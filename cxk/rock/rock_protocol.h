#pragma once


#include "cxk/protocol.h"
#include "google/protobuf/message.h"


namespace cxk{


class RockBody{
public:
    using ptr = std::shared_ptr<RockBody>;
    virtual ~RockBody() {}

    void setBody(const std::string& body){ m_body = body; }
    const std::string& getBody() const { return m_body; }

    virtual bool serializeToByteArray(ByteArray::ptr bytearray);
    virtual bool parseFromByteArray(ByteArray::ptr bytearray);


    /**
     * @brief    将body转换为Protocol Buffers消息类型，如果转换失败则返回nullptr。
     */
    template<class T>
    std::shared_ptr<T> getAsPB() const{
        try{
            std::shared_ptr<T> data(new T);
            if(data->ParseFromString(m_body)){
                return data;
            }
        } catch(...){
            return nullptr;
        }
    }


    template <class T>
    bool setAsPB(const T& v) const{
        try{
            return v.SerializeToString(&m_body);
        } catch(...){

        }
        return false;
    }

protected:
    std::string m_body;
};


class RockResponse;
class RockRequest : public Request, public RockBody{
public:
    using ptr = std::shared_ptr<RockRequest>;

    std::shared_ptr<RockResponse> createResponse();

    virtual std::string toString() const override;
    virtual const std::string& getName() const override;
    virtual int32_t getType() const override;

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
};

class RockResponse : public Response, public RockBody{
public:
    using ptr = std::shared_ptr<RockResponse>;

    virtual std::string toString() const override;
    virtual const std::string& getName() const override;
    virtual int32_t getType() const override;

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
};


class RockNotify : public Notify, public RockBody{
public:
    using ptr = std::shared_ptr<RockNotify>;

    virtual std::string toString() const override;
    virtual const std::string& getName() const override;
    virtual int32_t getType() const override;

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
};


struct RockMsgHeader{
    RockMsgHeader();
    uint8_t magic[2];
    uint8_t version;
    uint8_t flag;       // 是否压缩
    int32_t length;
};


class RockMessageDecoder : public MessageDecoder{
public:
    using ptr = std::shared_ptr<RockMessageDecoder>;

    virtual Message::ptr parseFrom(Stream::ptr stream) override;
    virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) override;
};

}