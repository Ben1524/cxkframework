#pragma once


#include "cxk/stream.h"
#include <sys/uio.h>
#include <zlib.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <iostream>


namespace cxk{


class ZlibStream : public Stream{
public:
    using ptr = std::shared_ptr<ZlibStream>;

    // 压缩类型
    enum Type{
        ZLIB,
        DEFLATE,
        GZIP
    };

    // 压缩策略
    enum Strategy{
        DEFAULT = Z_DEFAULT_STRATEGY,
        FILTERED = Z_FILTERED,
        HUFFMAN = Z_HUFFMAN_ONLY,
        FIXED = Z_FIXED,
        RLE = Z_RLE,
    };


    // 压缩级别--> 无压缩，最佳速度，最佳压缩，默认压缩
    enum CompressLevel{
        NO_COMPRESSION = Z_NO_COMPRESSION,
        BEST_SPEED = Z_BEST_SPEED,
        BEST_COMPRESSION = Z_BEST_COMPRESSION,
        DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION
    };

    static ZlibStream::ptr CreateGzip(bool encode, std::uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateDeflate(bool encode, std::uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateZlib(bool encode, std::uint32_t buff_size = 4096);
    static ZlibStream::ptr Create(bool encode, std::uint32_t buff_size = 4096, Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15, 
        int memlevel = 8, Strategy strategy = DEFAULT);

    ZlibStream(bool encode, std::uint32_t buff_size = 4096);
    ~ZlibStream();


    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    int flush();

    bool isFree() const { return m_free; }
    void setFree(bool v) { m_free = v; }


    bool isEncode() const { return m_encode; }
    void setEncode(bool v) { m_encode = v; }

    std::vector<iovec>& getBUffers() { return m_buffs; }
    std::string getResult() const;

    cxk::ByteArray::ptr getByteArray();

private:
    int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION
        ,int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

    int encode(const iovec* v, const uint64_t& size, bool finish);
    int decode(const iovec* v, const uint64_t& size, bool finish);

private:
    z_stream m_zstream;
    std::uint32_t m_buffsize;
    bool m_encode;
    bool m_free;
    std::vector<iovec> m_buffs;
};




}