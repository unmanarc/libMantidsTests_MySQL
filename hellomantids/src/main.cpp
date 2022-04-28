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
        Globals::getAppLog()->log(__func__, "","", Logs::LEVEL_INFO, 2048, "Starting... (Build date %s %s), PID: %u",__DATE__, __TIME__, getpid());
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Using config dir: %s", configDir.c_str());

        Globals::getLoginRPCClient()->start();

        if (!RPCServerImpl::createRPCListener())
        {
            _exit(-2);
        }

        if (!WebServerImpl::createWebServer())
        {
            _exit(-1);
        }

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  "Looking for static login resources...");

        auto staticContent = Globals::getLoginRPCClient()->getRemoteAuthManager()->getStaticContent();

        for ( unsigned int i=0; i<staticContent.size(); i++ )
        {
            auto jContent =  staticContent[i];

            if (jContent.isMember("path") && jContent.isMember("content"))
            {
                std::string path = JSON_ASSTRING(jContent,"path","");
                std::string content = JSON_ASSTRING(jContent,"content","");
                Globals::getWebServer()->addInternalContentElement(path,content);
                Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  "Adding web login static resource '%s'", path.c_str());
            }
            else {
                Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN,  "Warning: bad static resource (%d)", i);
            }
        }

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  (globalArguments->getDaemonName() + " initialized with PID: %d").c_str(), getpid());

        return 0;
    }

    void _initvars(int , char *[], Arguments::GlobalArguments * globalArguments)
    {
        // init variables (pre-config):
        globalArguments->setInifiniteWaitAtEnd(true);

        globalArguments->setAuthor("Aarón Mizrachi <amizrachi@tekium.mx>, Enrique Vaamonde <evaamonde@tekium.mx>");
        //globalArguments->setEmail("amizrachi@tekium.mx");
        globalArguments->setVersion(atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PTOJECT_VER_PATCH), "");
        globalArguments->setDescription(PROJECT_DESCRIPTION);

        globalArguments->addCommandLineOption("Service Options", 'c', "config-dir" , "Configuration directory"  , "/etc/hellomantids", Mantids::Memory::Abstract::TYPE_STRING );
    }

    bool _config(int , char *argv[], Arguments::GlobalArguments * globalArguments)
    {
        // process config:
        unsigned int logMode = Logs::MODE_STANDARD;

        Mantids::Network::TLS::Socket_TLS::prepareTLS();

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
            boost::property_tree::ini_parser::read_ini((configDir + "/config.ini").c_str(),pRunningConfig);
        else
        {
            initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration: %s", (configDir + "/config.ini").c_str());
            return false;
        }

        Globals::setLocalInitConfig(pRunningConfig);

        if ( pRunningConfig.get<bool>("Logs.ToSyslog",true) ) logMode|=Logs::MODE_SYSLOG;
        Globals::setAppLog(new Logs::AppLog(logMode));
        Globals::getAppLog()->setPrintEmptyFields(true);
        Globals::getAppLog()->setUsingColors(Globals::getLC_LogsShowColors());
        Globals::getAppLog()->setUsingPrintDate(Globals::getLC_LogsShowDate());
        Globals::getAppLog()->setModuleAlignSize(26);
        Globals::getAppLog()->setUsingAttributeName(false);
        Globals::getAppLog()->setDebug(Globals::getLC_LogsDebug());

        Globals::setRPCLog(new Logs::RPCLog(logMode));
        Globals::getRPCLog()->setPrintEmptyFields(true);
        Globals::getRPCLog()->setUsingColors(Globals::getLC_LogsShowColors());
        Globals::getRPCLog()->setUsingPrintDate(Globals::getLC_LogsShowDate());
        Globals::getRPCLog()->setDisableDomain(true);
        Globals::getRPCLog()->setDisableModule(true);
        Globals::getRPCLog()->setModuleAlignSize(26);
        Globals::getRPCLog()->setUsingAttributeName(false);
        Globals::getRPCLog()->setStandardLogSeparator(",");
        Globals::getRPCLog()->setDebug(Globals::getLC_LogsDebug());

        // Initialize remote login rpc implementation here:
        Globals::getLoginRPCClient()->setApiKey(Globals::getLC_LoginRPC_ApiKey());
        Globals::getLoginRPCClient()->setAppName(Globals::getLC_LoginRPC_AppName());
        Globals::getLoginRPCClient()->setCaFile(Globals::getLC_LoginRPC_TLSCAFilePath());
        Globals::getLoginRPCClient()->setRemoteHost(Globals::getLC_LoginRPC_RemoteHost());
        Globals::getLoginRPCClient()->setRemotePort(Globals::getLC_LoginRPC_RemotePort());
        Globals::getLoginRPCClient()->setUseIPv6(Globals::getLC_LoginRPC_UseIPv6());

        // Create the DB / Get database status...
        if (!DB::start())
        {
            return false;
        }

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Configuration file loaded OK.");

        return true;
    }
};

int main(int argc, char *argv[])
{
    Main * main = new Main;
    return StartApplication(argc,argv,main);
}

