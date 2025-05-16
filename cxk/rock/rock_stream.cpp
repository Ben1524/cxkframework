#pragma once
#include "rock_stream.h"
#include "cxk/logger.h"


namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


RockStream::RockStream(Socket::ptr sock) : AsyncSocketStream(sock, true), m_decoder(new RockMessageDecoder) { 
    CXK_LOG_INFO(g_logger) << "RockStream::RockStream " << this << " " << (sock ? sock->toString() : "");
}

RockStream::~RockStream(){
    CXK_LOG_INFO(g_logger) << "RockStream::~RockStream " << this << " " << (m_socket ? m_socket->toString() : "");
}

int32_t RockStream::sendMessage(Message::ptr msg){
    if(isConnected()){
        RockSendCtx::ptr ctx(new RockSendCtx);
        ctx->msg = msg;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}


RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms){
    if(isConnected()){
        RockCtx::ptr ctx(new RockCtx);
        ctx->request = req;
        ctx->sn = req->getSn();
        ctx->timeout = timeout_ms;
        ctx->scheduler = cxk::Scheduler::GetThis();
        ctx->fiber = cxk::Fiber::GetThis();
        addCtx(ctx);
        
        ctx->timer = cxk::IOManager::GetThis()->addTimer(timeout_ms, std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));

        enqueue(ctx);
        cxk::Fiber::YieldToHold();
        return std::make_shared<RockResult>(ctx->result, ctx->response);
    } else {
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, nullptr);
    }
    
}


bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream){
    return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder->serializeTo(stream, msg) > 0;
}


bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream){
    return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder->serializeTo(stream, request) > 0;
}



AsyncSocketStream::Ctx::ptr RockStream::doRecv(){
    auto msg = m_decoder->parseFrom(shared_from_this());
    if(!msg){
        innerClose();
        return nullptr;
    }

    int type = msg->getType();
    if(type == Message::RESPONSE){
        auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
        if(!rsp){
            CXK_LOG_WARN(g_logger) << "RockStream:: do  Recv  RockResponse not response" << msg->toString();
            return nullptr;
        }
        RockCtx::ptr ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
        ctx->result = rsp->getResult();
        ctx->response = rsp;
        return ctx;
    } else if(type == Message::REQUEST){
        auto req = std::dynamic_pointer_cast<RockRequest>(msg);
        if(!req){
            CXK_LOG_WARN(g_logger) << "RockStream:: do  Recv  RockRequest not request" << msg->toString();
            return nullptr;
        }
        if(m_requestHandler){
            m_iomanager->schedule(std::bind(&RockStream::handleRequest, std::dynamic_pointer_cast<RockStream>(shared_from_this()), req));
        }
    } else if(type == Message::NOTIFY){
        auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
        if(!nty){
            CXK_LOG_WARN(g_logger) << "RockStream doRecv notify not RockNotify: " << msg->toString();
            return nullptr ;
        }

        if(m_notifyHandler){
            m_iomanager->schedule(std::bind(&RockStream::handleNotify, std::dynamic_pointer_cast<RockStream>(shared_from_this()), nty));
        }
    } else {
        CXK_LOG_WARN(g_logger) << "RockStream:: do  Recv  unknow message type: " << msg->toString();
    }
    return nullptr;
}


void RockStream::handleRequest(cxk::RockRequest::ptr req){
    cxk::RockResponse::ptr rsp = req->createResponse();
    if(!m_requestHandler(req, rsp, std::dynamic_pointer_cast<RockStream>(shared_from_this()))){
        sendMessage(rsp);
        innerClose();
    } else {
        sendMessage(rsp);
    }
}


void RockStream::handleNotify(cxk::RockNotify::ptr nty){
    if(!m_notifyHandler(nty, std::dynamic_pointer_cast<RockStream>(shared_from_this()))){
        innerClose();
    }
}


RockSession::RockSession(Socket::ptr sock) : RockStream(sock) {
    m_autoConnect = false;
}


RockConnection::RockConnection() : RockStream(nullptr) {
    m_autoConnect = true;
}

bool RockConnection::connect(cxk::Address::ptr addr){
    m_socket = cxk::Socket::CreateTCP(addr);
    return m_socket->connect(addr);
}



RockResult::ptr RockFairLoadBalance::request(RockRequest::ptr req, uint32_t timeout_ms){
    uint64_t ts = cxk::GetCurrentMS();
    {
        if((ts - m_lastInitTime) > 500){
            RWMutexType::WriteLock lock(m_mutex);
            initWeight();
            m_lastInitTime = ts;
        }
    }

    auto conn = getAsFair();
    if(!conn){
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, nullptr);
    }

    auto& stats = conn->get(ts / 1000);
    stats.incDoing(1);
    stats.incTotal(1);

    auto r = conn->getStreamAs<RockStream>()->request(req, timeout_ms);
    uint64_t ts2 = cxk::GetCurrentMS();
    if(r->result == 0){
        stats.incOks(1);
        stats.incUsedTime(ts2 - ts);
    } else if(r->result == AsyncSocketStream::TIMEOUT){
        stats.incTimeouts(1);
    } else if(r->result < 0){
        stats.incErrs(1);
    }
    stats.decDoing(1);
    return r;
}



}