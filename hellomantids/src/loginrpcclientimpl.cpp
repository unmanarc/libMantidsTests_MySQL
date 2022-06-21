#include "loginrpcclientimpl.h"
#include "globals.h"
#include <mdz_prg_logs/applog.h>
#include <inttypes.h>

using namespace Mantids;
using namespace Mantids::Application;

LoginRPCClientImpl::LoginRPCClientImpl()
{

}

LoginRPCClientImpl::~LoginRPCClientImpl()
{

}

void LoginRPCClientImpl::notifyTLSConnecting(Mantids::Network::TLS::Socket_TLS *, const std::string &host, const uint16_t &port)
{
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Connecting to RPC Login Server [%s:%" PRIu16 "]...", host.c_str(), port);
}

void LoginRPCClientImpl::notifyTLSDisconnected(Network::TLS::Socket_TLS *, const std::string &host, const uint16_t &port, int code)
{
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Disconnected from RPC Login Server [%s:%" PRIu16 "] with code %d...", host.c_str(), port, code);
}

void LoginRPCClientImpl::notifyAPIProcessingOK(Mantids::Network::TLS::Socket_TLS *tlsSock)
{
    char remotePair[INET6_ADDRSTRLEN+1];
    memset(remotePair,0,INET6_ADDRSTRLEN+1);
    tlsSock->getRemotePair(remotePair);

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Connected to RPC Login Server [%s:%" PRIu16 "] via TLS, authenticated.", remotePair, tlsSock->getRemotePort());
}

void LoginRPCClientImpl::notifyTLSConnectedOK(Mantids::Network::TLS::Socket_TLS *tlsSock)
{
    char remotePair[INET6_ADDRSTRLEN+1];
    memset(remotePair,0,INET6_ADDRSTRLEN+1);
    tlsSock->getRemotePair(remotePair);

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Connected to RPC Login Server [%s:%" PRIu16 "] via TLS, starting authentication...", remotePair, tlsSock->getRemotePort());
}

void LoginRPCClientImpl::notifyBadApiKey(Mantids::Network::TLS::Socket_TLS *tlsSock)
{
    char remotePair[INET6_ADDRSTRLEN+1];
    memset(remotePair,0,INET6_ADDRSTRLEN+1);
    tlsSock->getRemotePair(remotePair);

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Disconnected from RPC Login Server [%s:%" PRIu16 "] via TLS, bad api key.", remotePair, tlsSock->getRemotePort());
}

void LoginRPCClientImpl::notifyTLSErrorConnecting(Mantids::Network::TLS::Socket_TLS *tlsSock, const std::string &host, const uint16_t &port)
{
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Error connecting to RPC Login Server [%s:%" PRIu16 "] via TLS: %s", host.c_str(),port, tlsSock->getLastError().c_str());

    for (const auto & i :tlsSock->getTLSErrorsAndClear())
    {
        if (!strstr(i.c_str(),"certificate unknown"))
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "RPC Login TLS Protocol Initialization Error [%s:%" PRIu16 "]: %s",  host.c_str(),port,i.c_str());
    }
}
