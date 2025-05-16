#pragma once


#include "cxk/stream/async_socket_stream.h"
#include "rock_protocol.h"
#include "cxk/stream/load_balance.h"

namespace cxk{


struct RockResult{
    using ptr = std::shared_ptr<RockResult>;
    RockResult(int32_t _result, RockResponse::ptr rsp) : result(_result), response(rsp){}
    int32_t result;
    RockResponse::ptr response;
};


class RockStream : public cxk::AsyncSocketStream{
public:
    using ptr = std::shared_ptr<RockStream>;
    using request_handler = std::function<bool(cxk::RockRequest::ptr, cxk::RockResponse::ptr, cxk::RockStream::ptr)>;
    using notify_handler = std::function<bool(cxk::RockNotify::ptr, cxk::RockStream::ptr)>;

    RockStream(Socket::ptr sock);
    ~RockStream();

    int32_t sendMessage(Message::ptr msg);
    RockResult::ptr request(RockRequest::ptr req, uint32_t timeout_ms);

    request_handler getRequestHandler() const { return m_requestHandler; }
    void setRequestHandler(request_handler handler) { m_requestHandler = handler; }

    notify_handler getNotifyHandler() const { return m_notifyHandler; }
    void setNotifyHandler(notify_handler handler) { m_notifyHandler = handler; }

protected:
    struct RockSendCtx : public SendCtx{
        using ptr = std::shared_ptr<RockSendCtx>;
        Message::ptr msg;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;
    };

    struct RockCtx : public Ctx{
        using ptr = std::shared_ptr<RockCtx>;
        RockRequest::ptr request;
        RockResponse::ptr  response;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;
    };

    virtual Ctx::ptr doRecv() override;

    void handleRequest(cxk::RockRequest::ptr req);
    void handleNotify(cxk::RockNotify::ptr nty);


private:
    RockMessageDecoder::ptr m_decoder;
    request_handler m_requestHandler;
    notify_handler m_notifyHandler;
};


class RockSession : public RockStream{
public:
    using ptr = std::shared_ptr<RockSession>;
    RockSession(Socket::ptr sock);
};


class RockConnection : public RockStream{
public:
    using ptr = std::shared_ptr<RockConnection>;
    RockConnection();

    bool connect(cxk::Address::ptr addr);
};


class RockFairLoadBalance : public FairLoadBalance{
public:
    using ptr = std::shared_ptr<RockFairLoadBalance>;

    RockResult::ptr request(RockRequest::ptr req, uint32_t timeout_ms);

private:
    uint64_t m_lastInitTime = 0;
};


}