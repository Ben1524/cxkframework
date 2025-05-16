#include "cxk/bytearray.h"
#include "cxk/cxk.h"

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test(){
#define XX(type, len, write_fun, read_fun, base_len) \
{ \
    std::vector<type> vec;  \   
    for(int i = 0; i < len; ++i){ \
        vec.push_back(rand()); \
    }                   \
    cxk::ByteArray::ptr ba(new cxk::ByteArray(base_len)); \
    for(auto& i : vec){\
        ba->write_fun(i); \
    }                     \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i){ \
        type v = ba->read_fun(); \
        CXK_ASSERT(v == vec[i]);\
    }\
    CXK_ASSERT(ba->getReadSize() == 0);\
    CXK_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
        " (" #type ") len=" << len \
        << " base_len=" << base_len << " size= " << ba->getSize();\
}

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8_t, readFuint8_t, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16_t, readFuint16_t, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32_t, readFuint32_t, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX
}


int main(int argc, char *argv[]){
    test();

    return 0;
}