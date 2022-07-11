#include "webserverimpl.h"
#include "globals.h"
#include "config.h"

#include <mdz_db_sqlite3/sqlconnector_sqlite3.h>
#include <mdz_auth_db/manager_db.h>
#include <mdz_xrpc_webserver/webserver.h>
#include <mdz_net_sockets/socket_tls.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <sstream>
#include <ostream>
#include <inttypes.h>

using namespace Mantids::Application;
using namespace Mantids::Authentication;
using namespace Mantids::RPC::Web;
using namespace Mantids::RPC;
using namespace Mantids;


WebServerImpl::WebServerImpl()
{
}

json WebServerImpl::rmtCaller(const std::string &caller, Manager *auth, Session *sess, const json &jInput)
{
    std::string target = JSON_ASSTRING(jInput,"target","");
    std::string remoteMethod = JSON_ASSTRING(jInput,"remoteMethod","");
    json j,error;

    // stats/control Validation
    if (!boost::starts_with(remoteMethod, caller)) return j;

    if ( target == "DB" )
    {
        // Local DB.
        j = DB::runLocalRPCMethod(remoteMethod,auth,sess,jInput["payload"],&error);
    }
    else // ASUME THIS IS A REMOTE TARGET:
    {
        j = Globals::getFastRPC()->runRemoteRPCMethod(target, remoteMethod,jInput["payload"],&error);
    }

    if (JSON_ASBOOL(error,"succeed",false) == false)
    {
        LOG_APP->log2(__func__, sess->getUserID(),"", Logs::LEVEL_ERR, "Error (%d) Executing Remote RPC Method '%s' at '%s': '%s'",
                                   JSON_ASINT(error,"errorId",-1),
                                   remoteMethod.c_str(),
                                   JSON_ASCSTRING(jInput,"target",""),
                                   JSON_ASCSTRING(error,"errorMessage","")
                                   );
    }
    else
    {
        LOG_APP->log2(__func__, sess->getUserID(),"", Logs::LEVEL_INFO, "Remote RPC Method '%s' Executed at '%s'.",
                                   remoteMethod.c_str(),
                                   JSON_ASCSTRING(jInput,"target","")
                                   );
    }
    return j;
}
json WebServerImpl::controlMethods(void *, Manager *auth, Session *sess, const json & jInput)
{
    return rmtCaller( "control.", auth,sess,jInput );
}

json WebServerImpl::statMethods(void *, Manager *auth, Session *sess, const json &jInput)
{
    return rmtCaller( "stats.", auth,sess,jInput );
}

bool WebServerImpl::createWebServer()
{
    std::string sAppName = Globals::getLC_LoginRPC_AppName();
    Mantids::Network::Sockets::Socket_TLS * sockWebListen = new Mantids::Network::Sockets::Socket_TLS;

    uint16_t listenPort = Globals::getLC_WebServer_ListenPort();
    std::string listenAddr = Globals::getLC_WebServer_ListenAddr();

    if (!sockWebListen->setTLSPublicKeyPath(  Globals::getLC_WebServer_TLSCertFilePath().c_str()  ))
    {
        LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting Web Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, "Bad TLS WEB Server Public Key");
        return false;
    }
    if (!sockWebListen->setTLSPrivateKeyPath( Globals::getLC_WebServer_TLSKeyFilePath().c_str()  ))
    {
        LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting Web Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, "Bad TLS WEB Server Private Key");
        return false;
    }

    if (sockWebListen->listenOn(listenPort ,listenAddr.c_str(), !Globals::getLC_WebServer_UseIPv6() ))
    {
        Authentication::Domains * authDomains = new Authentication::Domains;

        ////////////////////////////////////////////////////////////////
        /// Authentication DB
        ///
        auto * authManager = Globals::getLoginRPCClient()->getRemoteAuthManager();
        Secret passDataStats, passDataControl;

        // Create attribute to applications:]
        if (!authManager->attribExist({sAppName,"control"}))
        {
            if (!authManager->attribAdd({sAppName,"control"},"Control Access"))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to create attribute in ther authentication server.");
                return false;
            }
            else
                LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Attrib 'control' initialized.");

        }
        if (!authManager->attribExist({sAppName,"stats"}))
        {
            if (!authManager->attribAdd({sAppName,"stats"},"Stats Access"))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to create attribute in ther authentication server.");
                return false;
            }
            else
                LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Attrib 'stats' initialized.");
        }
        // Add the default domain / auth:
        authDomains->addDomain("",authManager);
        ////////////////////////////////////////////////////////////////

        MethodsManager *methodsManagers = new MethodsManager(sAppName);
        // Add functions
        methodsManagers->addRPCMethod( "remote.stats",        {"stats"},   {&WebServerImpl::statMethods,nullptr} );
        methodsManagers->addRPCMethod( "remote.control",      {"control"}, {&WebServerImpl::controlMethods,nullptr} );

        WebServer * webServer = new WebServer;
        Globals::setWebServer(webServer);
        webServer->setRPCLog(LOG_RPC);

        std::string resourcesPath = Globals::getLC_WebServer_ResourcesPath();
        if (!webServer->setDocumentRootPath( resourcesPath ))
        {
            LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Error locating web server resources at %s",resourcesPath.c_str() );
            return false;
        }
        webServer->setAuthenticator(authDomains);
        webServer->setMethodManagers(methodsManagers);
        webServer->setSoftwareVersion(atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PROJECT_VER_PATCH), PROJECT_NAME);
        webServer->setExtCallBackOnInitFailed(WebServerImpl::protoInitFail);

        webServer->acceptPoolThreaded(sockWebListen);

        LOG_APP->log0(__func__,Logs::LEVEL_INFO,  "Web Server Listening @%s:%" PRIu16, listenAddr.c_str(), listenPort);
        return true;
    }
    else
    {
        LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting Web Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, sockWebListen->getLastError().c_str());
        return false;
    }
}

bool WebServerImpl::protoInitFail(void * , Network::Sockets::Socket_StreamBase * sock, const char * remoteIP, bool )
{
    Mantids::Network::Sockets::Socket_TLS * secSocket = (Mantids::Network::Sockets::Socket_TLS *)sock;

    for (const auto & i :secSocket->getTLSErrorsAndClear())
    {
        if (!strstr(i.c_str(),"certificate unknown"))
            LOG_APP->log1(__func__, remoteIP,Logs::LEVEL_ERR, "TLS Protocol Initialization: %s", i.c_str());
    }
    return true;
}
