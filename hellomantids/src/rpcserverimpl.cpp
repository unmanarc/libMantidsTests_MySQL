#include "rpcserverimpl.h"
#include "globals.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <mdz_net_sockets/socket_tls.h>
#include <mdz_net_sockets/socket_acceptor_multithreaded.h>
#include <mdz_xrpc_fast/fastrpc.h>
#include <mdz_prg_logs/applog.h>
#include <mdz_net_sockets/cryptostream.h>

#include <string>
#include <inttypes.h>

using namespace Mantids;
using namespace Application;
using namespace RPC;

using namespace std;

RPCServerImpl::RPCServerImpl()
{
}

bool RPCServerImpl::callbackOnRPCConnect(void *, Network::Streams::StreamSocket *sock, const char * remoteAddr, bool secure)
{
    string sComponent = sock->readStringEx<uint8_t>();
    Network::TLS::Socket_TLS * tlsSock = (Network::TLS::Socket_TLS *)sock;
    string peerCNName = tlsSock->getTLSPeerCN();

    int ret=-144;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Incoming %sRPC connection from (IP=%s,CN=%s,COMPONENT=%s)", secure?"secure ":"", remoteAddr, tlsSock->getTLSPeerCN().c_str(), sComponent.c_str());

    Mantids::Network::Streams::CryptoStream cstreams(sock);
    if (cstreams.mutualChallengeResponseSHA256Auth(Globals::getLC_RPCServer_ApiKey(),false) == make_pair(true,true))
    {
        if ((ret=Globals::getFastRPC()->processConnection(sock, "AGENT." + peerCNName ))==-2)
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "RPC Already Connected, giving up from %s for CN=%s", remoteAddr, peerCNName.c_str());
        }
    }
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Invalid API Key from RPC connector %s, AGENT.%s", remoteAddr, peerCNName.c_str());
    }

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "RPC connection from %s (CN=%s, COMPONENT=%s) Terminated with %d.", remoteAddr, tlsSock->getTLSPeerCN().c_str(), sComponent.c_str(), ret);

    return true;
}

bool RPCServerImpl::createRPCListener()
{
    Network::TLS::Socket_TLS * sockRPCListen = new Network::TLS::Socket_TLS;

    uint16_t listenPort = Globals::getLC_RPCServer_ListenPort();
    string listenAddr = Globals::getLC_RPCServer_ListenAddr();

    if (!sockRPCListen->setTLSCertificateAuthorityPath(Globals::getLC_RPCServer_TLSCAFilePath().c_str()))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting RPC Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, "Bad TLS RPC Server Public Key");
        return false;
    }
    if (!sockRPCListen->setTLSPublicKeyPath( Globals::getLC_RPCServer_TLSCertFilePath().c_str() ))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting RPC Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, "Bad TLS RPC Server Public Key");
        return false;
    }
    if (!sockRPCListen->setTLSPrivateKeyPath( Globals::getLC_RPCServer_TLSKeyFilePath().c_str() ))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting RPC Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, "Bad TLS RPC Server Private Key");
        return false;
    }

    Globals::setFastRPC(new RPC::Fast::FastRPC);

    Globals::getFastRPC()->addMethod("getHelloWorldMessage",{&getHelloWorldMessage,nullptr});
    Globals::getFastRPC()->addMethod("_pingNotFound_",{&_pingNotFound_,nullptr});

    Network::Sockets::Acceptors::Socket_Acceptor_MultiThreaded * multiThreadedAcceptor = new Network::Sockets::Acceptors::Socket_Acceptor_MultiThreaded;
    multiThreadedAcceptor->setMaxConcurrentClients( Globals::getLC_RPCServer_MaxClients() );
    multiThreadedAcceptor->setCallbackOnConnect(callbackOnRPCConnect,nullptr);
    multiThreadedAcceptor->setCallbackOnInitFail(_callbackOnInitFailed,nullptr);
    multiThreadedAcceptor->setCallbackOnTimedOut(_callbackOnTimeOut,nullptr);

    if (sockRPCListen->listenOn(listenPort ,listenAddr.c_str(), !Globals::getLC_RPCServer_UseIPv6() ))
    {
        multiThreadedAcceptor->setAcceptorSocket(sockRPCListen);
        multiThreadedAcceptor->startThreaded();
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  "Accepting RPC clients @%s:%" PRIu16 " via TLS", listenAddr.c_str(), listenPort);
        return true;
    }
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error starting RPC Server @%s:%" PRIu16 ": %s", listenAddr.c_str(), listenPort, sockRPCListen->getLastError().c_str());
        return false;
    }
}

json RPCServerImpl::getHelloWorldMessage(void *, const std::string &nodeName, const json &payload)
{
    json r;
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR,  "Node '%s' requesting hello world with payload=%s...", nodeName.c_str(), payload.toStyledString().c_str());

    r = "Hello World";

    return r;
}

bool RPCServerImpl::_callbackOnInitFailed(void * obj, Network::Streams::StreamSocket * s, const char * cUserIP, bool isSecure)
{
    Globals::getAppLog()->log1(__func__,cUserIP,Logs::LEVEL_ERR,  "Incoming RPC Client TLS initialization Failed...");

    for (const auto & i :((Network::TLS::Socket_TLS *)s)->getTLSErrorsAndClear())
    {
        //        if (!strstr(i.c_str(),"certificate unknown"))
        Globals::getAppLog()->log1(__func__,cUserIP,Logs::LEVEL_ERR, "RPC Login TLS Protocol Initialization Error: %s",  i.c_str());
    }

    return true;
}

void RPCServerImpl::_callbackOnTimeOut(void * obj, Network::Streams::StreamSocket *s, const char * cUserIP, bool isSecure)
{
    Globals::getAppLog()->log1(__func__,cUserIP,Logs::LEVEL_ERR,  "Client CN=%s timed out. Not enough threads...", ((Network::TLS::Socket_TLS *)s)->getTLSPeerCN().c_str());
    return;
}

json RPCServerImpl::_pingNotFound_(void *, const std::string &nodeName, const json &payload)
{
    //Globals::getAppLog()->log0(__func__, Logs::LEVEL_INFO, "Received ping from node '%s'.",nodeName.c_str());
    return true;
}

