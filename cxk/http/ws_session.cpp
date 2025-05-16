#include "ws_session.h"
#include "cxk/logger.h"
#include "cxk/util.h"
#include "cxk/util/hash_util.h"
#include "cxk/endian.h"
#include <string>


namespace cxk{
namespace http{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

cxk::ConfigVar<uint32_t>::ptr g_websocket_message_max_size = 
    cxk::Config::Lookup("websocket.message.max_size", (uint32_t) 1024 * 1024 * 32, "websocket message max size");


WSSession::WSSession(Socket::ptr sock, bool owner) : HttpSession(sock, owner){

}



HttpRequest::ptr WSSession::handleShake(){
    HttpRequest::ptr req;
    do{
        req = recvRequest();
        if(!req){
            CXK_LOG_INFO(g_logger) << "invalied http request";
            break;
        }

        if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")){
            CXK_LOG_INFO(g_logger) << "http header upgrade != websocket";
            break;
        }

        if(strcasecmp(req->getHeader("Connection").c_str(), "Upgrade")){
            CXK_LOG_INFO(g_logger) << "http header connection != upgrade";
            break;
        }


        if(req->getHeaderAs<int>("Sec-webSocket-Version") != 13){
            CXK_LOG_INFO(g_logger) << "invalied websocket version";
            break;
        }
        
        std::string key = req->getHeader("Sec-WebSocket-Key");
        if(key.empty()){
            CXK_LOG_INFO(g_logger) << "invalied websocket key";
            break;
        }

        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = cxk::base64decode(cxk::sha1sum(v));

        req->setWebsocket(true);

        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol Handshake");
        rsp->setHeader("Upgrade", "websocket");
        rsp->setHeader("Connection", "Upgrade");
        rsp->setHeader("Sec-WebSocket-Accept", v);

        sendResponse(rsp);
        CXK_LOG_DEBUG(g_logger) << *req;
        CXK_LOG_DEBUG(g_logger) << *rsp;
        return req;

    } while(false);

    if(req){
        CXK_LOG_INFO(g_logger) << *req;
    }
    return nullptr;
}


WSFrameMessage::WSFrameMessage(int opcode, const std::string& data):m_opcode(opcode), m_data(data){

}


std::string WSFrameHead::toString() const{
    std::stringstream ss;
    ss << "[WSFrameHead fin=" << fin
       << "rsv1=" << rsv1
       << "rsv2=" << rsv2
       << "rsv3=" << rsv3
       << "opcode=" << opcode
       << "mask=" << mask
       << "payloadn=" << payload
       << "]"; 
    return ss.str();
}


WSFrameMessage::ptr WSSession::recvMessage(){
    return WSRecvMessage(this, false);
}


int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin){
    return WSSendMessage(this, msg, false, fin);
}


int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, bool fin){
    return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
}


int32_t WSSession::ping(){
    return WSPing(this);
}


WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client){
    int opcode = 0;
    std::string data;
    int cur_len = 0;


    // 循环读取websocket帧
    do{
        // 读取帧头
        WSFrameHead ws_head;
        if(stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0){
            break;
        }
        CXK_LOG_DEBUG(g_logger) << "WSFrameHead " << ws_head.toString();

        // 处理PING帧
        if(ws_head.opcode == WSFrameHead::PING){
            CXK_LOG_INFO(g_logger) << "PING";
            if(WSPong(stream) <= 0){
                break;
            }
        } else if(ws_head.opcode == WSFrameHead::PONG){
        } else if(ws_head.opcode == WSFrameHead::CONTINUE
            || ws_head.opcode == WSFrameHead::TEXT_FRAME
            || ws_head.opcode == WSFrameHead::BIN_FRAME){
            // 检查掩码
            if(!client && !ws_head.mask){
                CXK_LOG_INFO(g_logger) << "WSFrameHead mask != 1";
                break;
            }

            // 根据帧头中的长度字段计算实际数据长度
            uint64_t length = 0;
            if(ws_head.payload == 126){
                uint16_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0){
                    break;
                }
                length = cxk::byteswapOnLittleEndian(len);
            } else if(ws_head.payload == 127){
                uint64_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0){
                    break;
                }
                length = cxk::byteswapOnLittleEndian(len);
            } else{
                length = ws_head.payload;
            }

            // 检查数据长度是否超过最大长度
            if((cur_len + length) >= g_websocket_message_max_size->getValue()) {
                CXK_LOG_WARN(g_logger) << "WSFrameMessage length > "
                    << g_websocket_message_max_size->getValue()
                    << " (" << (cur_len + length) << ")";
                break;
            }
            char mask[4] = {0};
            // 如果使用了掩码，从流中读取数据掩码
            if(ws_head.mask) {
                if(stream->readFixSize(mask, sizeof(mask)) <= 0) {
                    break;
                }
            }

            // 从流中读取数据
            data.resize(cur_len + length);
            if(stream->readFixSize(&data[cur_len], length) <= 0) {
                break;
            }

            // 解码数据
            if(ws_head.mask) {
                for(int i = 0; i < (int)length; ++i) {
                    data[cur_len + i] ^= mask[i % 4];
                }
            }
            cur_len += length;

            // 如果是第一个帧，记录opcode
            if(!opcode && ws_head.opcode != WSFrameHead::CONTINUE) {
                opcode = ws_head.opcode;
            }

            // 如果帧标记为结束，则返回数据
            if(ws_head.fin) {
                CXK_LOG_INFO(g_logger) << data;
                return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data)));
            }
        } else{
            CXK_LOG_DEBUG(g_logger) << "invalid epcode=" << ws_head.opcode;
        }
    } while(true);
    stream->close();
    return nullptr;
}


int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin){
    do{
        // 初始化websocket帧头
        WSFrameHead ws_head;
        memset(&ws_head, 0, sizeof(ws_head));
        ws_head.fin = fin;
        ws_head.opcode = msg->getOpcode();
        ws_head.mask = client;
        uint64_t size = msg->getData().size();
        
        // 计算并设置负载长度
        if(size < 126){
            ws_head.payload = size;
        } else if(size < 65536){
            ws_head.payload = 126;
        } else{
            ws_head.payload = 127;
        }

        // 写入帧头
        if(stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0){
            break;
        }

        // 写入扩展的负载长度
        if(ws_head.payload == 126){
            uint16_t len = size;
            len = cxk::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0){
                break;
            }
        } else if(ws_head.payload == 127){
            uint64_t len = cxk::byteswapOnLittleEndian(size);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0){
                break;
            }
        }


        // 如果为客户端，则对数据进行掩码
        if(client){
            char mask[4];
            uint32_t rand_value = rand();
            memcpy(mask, &rand_value, sizeof(mask));
            std::string& data = msg->getData();
            for(size_t i = 0; i < data.size(); ++i){
                data[i] ^= mask[i % 4];
            }

            if(stream->writeFixSize(mask, sizeof(mask)) <= 0){
                break;
            }
        }

        // 写入数据
        if(stream->writeFixSize(msg->getData().c_str(), size) <= 0){
            break;
        }

        // 返回数据总大小
        return size + sizeof(ws_head);
    }while(0);
    stream->close();
    return -1;
}


int32_t WSPing(Stream* stream){
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PING;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0){
        stream->close();
    }
    return v;
}


int32_t WSPong(Stream* stream){
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PONG;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0){
        stream->close();
    }
    return v;
}


int32_t WSSession::pong(){
    return WSPong(this);
}


bool WSSession::handleServerShake(){
    return true;
}


bool WSSession::handleClientShake(){
    return true;
}



}


}