#include "rock_protocol.h"
#include "cxk/logger.h"
#include "cxk/config.h"
#include "cxk/stream/zlib_stream.h"
#include "cxk/util.h"
#include "cxk/endian.h"

namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


static cxk::ConfigVar<uint32_t>::ptr g_rock_protocol_max_length = 
    cxk::Config::Lookup("rock.protocol.max_length", (uint32_t)(1024 * 1024 * 64), "rock protocol min length");

static cxk::ConfigVar<uint32_t>::ptr g_rock_protocol_gzip_min_length = 
    cxk::Config::Lookup("rock.protocol.gzip_min_length", (uint32_t)(1024 * 4), "rock protocol gzip min length");


bool RockBody::serializeToByteArray(ByteArray::ptr bytearray){
    bytearray->writeStringVint(m_body);
    return true;
}
    


bool RockBody::parseFromByteArray(ByteArray::ptr bytearray){
    m_body = bytearray->readStringVint();
    return true;
}



std::shared_ptr<RockResponse> RockRequest::createResponse(){
    RockResponse::ptr rt(new RockResponse);
    rt->setSn(m_sn);
    rt->setCmd(m_cmd);
    return rt;
}


std::string RockRequest::toString() const{
    std::stringstream ss;
    ss << "[RockRequest sn = " << m_sn 
        << " cmd = " << m_cmd
        << " body.length = " << m_body.size()
        << "]";
    return ss.str();
}


const std::string& RockRequest::getName() const{
    static const std::string& s_name = "RockRequest";
    return s_name;
}



int32_t RockRequest::getType() const{
    return Message::REQUEST;
}



bool RockRequest::serializeToByteArray(ByteArray::ptr bytearray){
    try{
        bool v = true;
        v &= Request::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "serializeToByteArray error";
    }
    return false;
}



bool RockRequest::parseFromByteArray(ByteArray::ptr bytearray){
    try{
        bool v = true;
        v &= Request::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "parseFromByteArray error";
    }
    return false;
}


std::string RockResponse::toString() const{
    std::stringstream ss;
    ss << "[RockResponse sn = " << m_sn 
    << " cmd = " << m_cmd
    << " result = " << m_result
    << " result_msg = " << m_resultStr
    << " body.length = " << m_body.size()
    << "]";
    return ss.str();
}


const std::string& RockResponse::getName() const {
    static const std::string& s_name = "RockResponse";
    return s_name;
}


int32_t RockResponse::getType() const{
    return Message::RESPONSE;
}


bool RockResponse::serializeToByteArray(ByteArray::ptr bytearray){
    try{
        bool v = true;
        v &= Response::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "serializeToByteArray error";
    }
    return false;
}



bool RockResponse::parseFromByteArray(ByteArray::ptr bytearray){
    try{
        bool v = true;
        v &= Response::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "parseFromByteArray error";
    }
    return false;
}



std::string RockNotify::toString() const{
    std::stringstream ss;
    ss << "[RockNotify notify = " << m_notify 
        << " body.length = " << m_body.size()
        << "]";
    return ss.str();  
}


const std::string& RockNotify::getName() const{
    static const std::string& s_name = "RockNotify";
    return s_name;
}



int32_t RockNotify::getType() const{
    return Message::NOTIFY;
}


bool RockNotify::serializeToByteArray(ByteArray::ptr bytearray) {
    try{
        bool v = true;
        v &= Notify::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "serializeToByteArray error";
    }
    return false;
}


bool RockNotify::parseFromByteArray(ByteArray::ptr bytearray){
    try{
        bool v = true;
        v &= Notify::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch(...){
        CXK_LOG_ERROR(g_logger) << "parseFromByteArray error";
    }
    return false;
}



RockMsgHeader::RockMsgHeader() : magic{0xab, 0xcd}, version(1), flag(0), length(0) {

}

static const uint8_t s_rock_magic[2] = {0xab, 0xcd};

Message::ptr RockMessageDecoder::parseFrom(Stream::ptr stream){
    try{
        RockMsgHeader header;
        if(stream->readFixSize(&header, sizeof(header)) <= 0){
            CXK_LOG_ERROR(g_logger) << "readFixSize error";
            return nullptr;
        }

        if(memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic))){
            CXK_LOG_ERROR(g_logger) << "magic error";
            return nullptr;
        }
        
        if(header.version != 0x1){
            CXK_LOG_ERROR(g_logger) << "version error";
            return nullptr;
        }

        header.length = cxk::byteswapOnLittleEndian(header.length);
        if((uint32_t)header.length >= g_rock_protocol_max_length->getValue()){
            CXK_LOG_ERROR(g_logger) << "length error";
            return nullptr;
        }

        cxk::ByteArray::ptr ba(new cxk::ByteArray);
        if(stream->readFixSize(ba, header.length) <= 0){
            CXK_LOG_ERROR(g_logger) << "readFixSize error";
            return nullptr;
        }

        ba->setPosition(0);
        if(header.flag & 0x1){
            auto zstream = cxk::ZlibStream::CreateGzip(false);
            if(zstream->write(ba, -1) != Z_OK){
                CXK_LOG_ERROR(g_logger) << "gzip error";
                return nullptr;
            }
            if(zstream->flush() != Z_OK){
                CXK_LOG_ERROR(g_logger) << "gzip error";
                return nullptr;
            }

            ba  = zstream->getByteArray();
        }

        uint8_t type = ba->readFuint8_t();
        Message::ptr msg;
        switch(type){
            case Message::REQUEST:
                msg.reset(new RockRequest);
                break;
            case Message::RESPONSE:
                msg.reset(new RockResponse);
                break;
            case Message::NOTIFY:
                msg.reset(new RockNotify);
                break;
            default:
                CXK_LOG_ERROR(g_logger) << "type error";
                return nullptr;
        }
        if(!msg->parseFromByteArray(ba)){
            CXK_LOG_ERROR(g_logger) << "parseFromByteArray error";
            return nullptr;
        }
        return msg;
    }catch(std::exception& e){
        CXK_LOG_ERROR(g_logger) << "parseFrom error: " << e.what();
    }catch(...){
        CXK_LOG_ERROR(g_logger) << "parseFrom error";
    }
    return nullptr;
}



int32_t RockMessageDecoder::serializeTo(Stream::ptr stream, Message::ptr msg){
    RockMsgHeader header;
    auto ba  = msg->toByteArray();
    ba->setPosition(0);

    header.length = ba->getSize();
    if((uint32_t)header.length >= g_rock_protocol_gzip_min_length->getValue()){
        auto zstream = cxk::ZlibStream::CreateGzip(true);
        if(zstream->write(ba, -1) != Z_OK){
            CXK_LOG_ERROR(g_logger) << "gzip error";
            return -1;
        }
        if(zstream->flush() != Z_OK){
            CXK_LOG_ERROR(g_logger) << "gzip error";
            return -1;
        }
        ba = zstream->getByteArray();
        header.flag |= 0x1;
        header.length = ba->getSize();
    }

    header.length = cxk::byteswapOnLittleEndian(header.length);
    if(stream->writeFixSize(&header, sizeof(header)) <= 0){
        CXK_LOG_ERROR(g_logger) << "writeFixSize error";
        return -1;
    }
    if(stream->writeFixSize(ba, ba->getReadSize()) <= 0){
        CXK_LOG_ERROR(g_logger) << "writeFixSize error";
        return -1;
    }
    return sizeof(header) + ba->getSize();
}


}