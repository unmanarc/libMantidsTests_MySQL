#include <mdz_prg_service/application.h>
#include <mdz_net_sockets/socket_tls.h>

#include "rpcserverimpl.h"
#include "webserverimpl.h"

#include "globals.h"
#include "config.h"

#include <sys/types.h>

#ifndef WIN32
#include <pwd.h>
#endif
#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#include <inttypes.h>
#include <sys/stat.h>

using namespace Mantids::Application;

class Main : public Application
{
public:

    void _shutdown()
    {
    }

    int _start(int , char *[], Arguments::GlobalArguments *globalArguments)
    {
        std::string configDir = globalArguments->getCommandLineOptionValue("config-dir")->toString();

        // start program.
        LOG_APP->log(__func__, "","", Logs::LEVEL_INFO, 2048, "Starting... (Build date %s %s), PID: %" PRIi32,__DATE__, __TIME__, getpid());
        LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Using config dir: %s", configDir.c_str());

        Globals::getLoginRPCClient()->start();

        if (!RPCServerImpl::createRPCListener())
        {
            _exit(-2);
        }

        if (!WebServerImpl::createWebServer())
        {
            _exit(-1);
        }

        LOG_APP->log0(__func__,Logs::LEVEL_INFO,  "Looking for static login resources...");

        auto staticContent = Globals::getLoginRPCClient()->getRemoteAuthManager()->getStaticContent();

        for ( unsigned int i=0; i<staticContent.size(); i++ )
        {
            auto jContent =  staticContent[i];

            if (jContent.isMember("path") && jContent.isMember("content"))
            {
                std::string path = JSON_ASSTRING(jContent,"path","");
                std::string content = JSON_ASSTRING(jContent,"content","");
                Globals::getWebServer()->addInternalContentElement(path,content);
                LOG_APP->log0(__func__,Logs::LEVEL_INFO,  "Adding web login static resource '%s'", path.c_str());
            }
            else {
                LOG_APP->log0(__func__,Logs::LEVEL_WARN,  "Warning: bad static resource (%d)", i);
            }
        }

        LOG_APP->log0(__func__,Logs::LEVEL_INFO,  (globalArguments->getDaemonName() + " initialized with PID: %d").c_str(), getpid());

        return 0;
    }

    void _initvars(int , char *[], Arguments::GlobalArguments * globalArguments)
    {
        // init variables (pre-config):
        globalArguments->setInifiniteWaitAtEnd(true);

        globalArguments->setAuthor("Aarón Mizrachi <amizrachi@tekium.mx>, Enrique Vaamonde <evaamonde@tekium.mx>");
        //globalArguments->setEmail("amizrachi@tekium.mx");
        globalArguments->setVersion(atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PROJECT_VER_PATCH), "");
        globalArguments->setDescription(PROJECT_DESCRIPTION);

        globalArguments->addCommandLineOption("Service Options", 'c', "config-dir" , "Configuration directory"  , "/etc/hellomantids", Mantids::Memory::Abstract::Var::TYPE_STRING );
    }

    bool _config(int , char *argv[], Arguments::GlobalArguments * globalArguments)
    {
        // process config:
        unsigned int logMode = Logs::MODE_STANDARD;

        Mantids::Network::Sockets::Socket_TLS::prepareTLS();

        Logs::AppLog initLog(Logs::MODE_STANDARD);
        initLog.setPrintEmptyFields(true);
        initLog.setUsingAttributeName(false);
        initLog.setUserAlignSize(1);

        std::string configDir = globalArguments->getCommandLineOptionValue("config-dir")->toString();

        initLog.log0(__func__,Logs::LEVEL_INFO, "Loading configuration: %s", (configDir + "/config.ini").c_str());

        boost::property_tree::ptree pRunningConfig;

        if (access(configDir.c_str(),R_OK))
        {
            initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration dir: %s", configDir.c_str());
            return false;
        }

        chdir(configDir.c_str());

        if (!access((configDir + "/config.ini").c_str(),R_OK))
        {
            struct stat stats;
            // Check file properties...
            stat((configDir + "/config.ini").c_str(), &stats);

            int smode = stats.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

            if ( smode != 0600 )
            {
                initLog.log0(__func__,Logs::LEVEL_WARN, "config.ini file permissions are not 0600 and API key may be exposed, changing...");

                if (chmod((configDir + "/config.ini").c_str(),0600))
                {
                    initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Configuration file permissions can't be changed to 0600");
                    return false;
                }
                initLog.log0(__func__,Logs::LEVEL_INFO, "config.ini file permissions changed to 0600");
            }
            boost::property_tree::ini_parser::read_ini((configDir + "/config.ini").c_str(),pRunningConfig);
        }
        else
        {
            initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration: %s", (configDir + "/config.ini").c_str());
            return false;
        }

        Globals::setLocalInitConfig(pRunningConfig);

        if ( pRunningConfig.get<bool>("Logs.ToSyslog",true) ) logMode|=Logs::MODE_SYSLOG;
        Globals::setAppLog(new Logs::AppLog(logMode));
        LOG_APP->setPrintEmptyFields(true);
        LOG_APP->setUsingColors(Globals::getLC_LogsShowColors());
        LOG_APP->setUsingPrintDate(Globals::getLC_LogsShowDate());
        LOG_APP->setModuleAlignSize(26);
        LOG_APP->setUsingAttributeName(false);
        LOG_APP->setDebug(Globals::getLC_LogsDebug());

        Globals::setRPCLog(new Logs::RPCLog(logMode));
        LOG_RPC->setPrintEmptyFields(true);
        LOG_RPC->setUsingColors(Globals::getLC_LogsShowColors());
        LOG_RPC->setUsingPrintDate(Globals::getLC_LogsShowDate());
        LOG_RPC->setDisableDomain(true);
        LOG_RPC->setDisableModule(true);
        LOG_RPC->setModuleAlignSize(26);
        LOG_RPC->setUsingAttributeName(false);
        LOG_RPC->setStandardLogSeparator(",");
        LOG_RPC->setDebug(Globals::getLC_LogsDebug());

        // Initialize remote login rpc implementation here:
        Globals::getLoginRPCClient()->setApiKey(Globals::getLC_LoginRPC_ApiKey());
        Globals::getLoginRPCClient()->setAppName(Globals::getLC_LoginRPC_AppName());
        Globals::getLoginRPCClient()->setCaFile(Globals::getLC_LoginRPC_TLSCAFilePath());
        Globals::getLoginRPCClient()->setRemoteHost(Globals::getLC_LoginRPC_RemoteHost());
        Globals::getLoginRPCClient()->setRemotePort(Globals::getLC_LoginRPC_RemotePort());
        Globals::getLoginRPCClient()->setUseIPv6(Globals::getLC_LoginRPC_UseIPv6());
        Globals::getLoginRPCClient()->setUsingTLSPSK(Globals::getLC_LoginRPC_UsePSK());

        // Create the DB / Get database status...
        if (!DB::start())
        {
            return false;
        }

        LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Configuration file loaded OK.");

        return true;
    }
};

int main(int argc, char *argv[])
{
    Main * main = new Main;
    return StartApplication(argc,argv,main);
}

