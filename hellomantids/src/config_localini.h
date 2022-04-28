#ifndef CONFIG_LOCALINI_H
#define CONFIG_LOCALINI_H

#include <boost/property_tree/ini_parser.hpp>

#ifdef _WIN32
static std::string dirSlash =  "\\";
#else
static std::string dirSlash =  "/";
#endif

class Config_LocalIni
{
public:
    Config_LocalIni();

    static void setLocalInitConfig(const boost::property_tree::ptree &config);

    static bool getLC_LogsUsingSyslog()
    {
        return pLocalConfig.get<bool>("Logs.Syslog",true);
    }

    static bool getLC_LogsShowColors()
    {
         return pLocalConfig.get<bool>("Logs.ShowColors",true);
    }

    static bool getLC_LogsShowDate()
    {
        return pLocalConfig.get<bool>("Logs.ShowDate",true);
    }

    static bool getLC_LogsDebug()
    {
        return pLocalConfig.get<bool>("Logs.Debug",false);
    }

    static std::string getLC_LoginRPC_ApiKey()
    {
        return pLocalConfig.get<std::string>("LoginRPCClient.ApiKey","REPLACEME_XABCXAPIX_LOGIN");
    }

    static std::string getLC_LoginRPC_TLSCAFilePath()
    {
        return pLocalConfig.get<std::string>("LoginRPCClient.CAFile",   ".." + dirSlash + "keys" + dirSlash + "loginrpc_ca.crt");
    }

    static std::string getLC_LoginRPC_AppName()
    {
        return pLocalConfig.get<std::string>("LoginRPCClient.AppName","HELLOMANTIDS");
    }

    static std::string getLC_LoginRPC_RemoteHost()
    {
        return pLocalConfig.get<std::string>("LoginRPCClient.RemoteHost","127.0.0.1");
    }

    static uint16_t getLC_LoginRPC_RemotePort()
    {
        return pLocalConfig.get<uint16_t>("LoginRPCClient.RemotePort",30301);
    }

    static bool getLC_LoginRPC_UseIPv6()
    {
        return pLocalConfig.get<bool>("LoginRPCClient.ipv6",false);
    }

    static uint16_t getLC_RPCServer_ListenPort()
    {
        return pLocalConfig.get<uint16_t>("RPCServer.ListenPort",10444);
    }

    static uint16_t getLC_RPCServer_MaxClients()
    {
        return pLocalConfig.get<uint16_t>("RPCServer.MaxClients",512);
    }

    static std::string getLC_RPCServer_ListenAddr()
    {
        return pLocalConfig.get<std::string>("RPCServer.ListenAddr","0.0.0.0");
    }

    static std::string getLC_RPCServer_ApiKey()
    {
        return pLocalConfig.get<std::string>("RPCServer.ApiKey","REPLACEME_XABCXAPIX_HELLOMANTIDSWEB");
    }

    static bool getLC_RPCServer_UseIPv6()
    {
        return pLocalConfig.get<bool>("RPCServer.ipv6",false);
    }

    static std::string getLC_RPCServer_TLSCAFilePath()
    {
        return pLocalConfig.get<std::string>("RPCServer.CAFile",   ".." + dirSlash + "keys" + dirSlash + "ca.crt");
    }

    static std::string getLC_RPCServer_TLSCertFilePath()
    {
        return pLocalConfig.get<std::string>("RPCServer.CertFile", ".." + dirSlash + "keys" + dirSlash + "master.crt");
    }

    static std::string getLC_RPCServer_TLSKeyFilePath()
    {
        return pLocalConfig.get<std::string>("RPCServer.KeyFile",  ".." + dirSlash + "keys" + dirSlash + "master.key");
    }

    static std::string getLC_WebServer_TLSCertFilePath()
    {
        return pLocalConfig.get<std::string>("WebServer.CertFile", ".." + dirSlash + "keys" + dirSlash + "web_snakeoil.crt");
    }

    static std::string getLC_WebServer_TLSKeyFilePath()
    {
        return pLocalConfig.get<std::string>("WebServer.KeyFile",  ".." + dirSlash + "keys" + dirSlash + "web_snakeoil.key");
    }

    static uint16_t getLC_WebServer_ListenPort()
    {
        return pLocalConfig.get<uint16_t>("WebServer.ListenPort",10443);
    }

    static std::string getLC_WebServer_ListenAddr()
    {
        return pLocalConfig.get<std::string>("WebServer.ListenAddr","0.0.0.0");
    }
    static std::string getLC_WebServer_ResourcesPath()
    {
        return pLocalConfig.get<std::string>("WebServer.ResourcesPath","/var/www/hellomantids");
    }

    static bool getLC_WebServer_UseIPv6()
    {
        return pLocalConfig.get<bool>("WebServer.ipv6",false);
    }
    
    static uint16_t getLC_Database_Port()
    {
        return pLocalConfig.get<uint16_t>("Database.DbPort",3306);
    }

    static std::string getLC_Database_Host()
    {
        return pLocalConfig.get<std::string>("Database.DbHost");
    }
    static std::string getLC_Database_DbName()
    {
        return pLocalConfig.get<std::string>("Database.DbName","hellomantids");
    }
      static std::string getLC_Database_DbUser()
    {
        return pLocalConfig.get<std::string>("Database.DbUser");
    }
    static std::string getLC_Database_DbPass()
    {
        return pLocalConfig.get<std::string>("Database.DbPass");
    }

private:
    static boost::property_tree::ptree pLocalConfig;

};

#endif // CONFIG_LOCALINI_H
