#ifndef RPC_H
#define RPC_H

#include <mdz_net_sockets/streamsocket.h>

#include <mdz_hlp_functions/json.h>


class RPCServerImpl
{
public:
    RPCServerImpl();
    static bool createRPCListener();

    static json getHelloWorldMessage(void *, const std::string &nodeName, const json &payload);
    static json _pingNotFound_(void *, const std::string &nodeName, const json &payload);

private:
    /**
     * @brief callbackOnRPCConnect
     * @param sock
     * @param remoteAddr
     * @param secure
     * @return
     */
    static bool callbackOnRPCConnect(void *, Mantids::Network::Streams::StreamSocket *sock, const char *remoteAddr, bool secure);
    /**
     * callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool _callbackOnInitFailed(void *, Mantids::Network::Streams::StreamSocket *, const char *, bool);
    /**
     * callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    static void _callbackOnTimeOut(void *, Mantids::Network::Streams::StreamSocket *, const char *, bool);


};


#endif // RPC_H
