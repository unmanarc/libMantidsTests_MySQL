#include "globals.h"

Mantids::Application::Logs::AppLog * Globals::applog = nullptr;
Mantids::Application::Logs::RPCLog * Globals::rpclog = nullptr;
Mantids::RPC::Fast::FastRPC * Globals::fastRPC = nullptr;

LoginRPCClientImpl Globals::loginRPCClient;
Mantids::RPC::Web::WebServer * Globals::webServer = nullptr;


Globals::Globals()
{
}

Mantids::Application::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(Mantids::Application::Logs::AppLog *value)
{
    applog = value;
}

Mantids::RPC::Fast::FastRPC *Globals::getFastRPC()
{
    return fastRPC;
}

void Globals::setFastRPC(Mantids::RPC::Fast::FastRPC *value)
{
    fastRPC = value;
}

Mantids::Application::Logs::RPCLog *Globals::getRPCLog()
{
    return rpclog;
}

void Globals::setRPCLog(Mantids::Application::Logs::RPCLog *value)
{
    rpclog = value;
}

LoginRPCClientImpl *Globals::getLoginRPCClient()
{
    return &loginRPCClient;
}

Mantids::RPC::Web::WebServer *Globals::getWebServer()
{
    return webServer;
}

void Globals::setWebServer(Mantids::RPC::Web::WebServer *value)
{
    webServer = value;
}
