#pragma once
#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

#define CXK_LITTLE_ENDIAN 1
#define CXK_BIG_ENDIAN 2

namespace cxk{

template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value){
    return (T) bswap_64((uint64_t)value);
}

template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value){
    return (T) bswap_32((uint32_t)value);
}

template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value){
    return (T) bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define CXK_BYTE_ORDER CXK_BIG_ENDIAN
#else
#define CXK_BYTE_ORDER CXK_LITTLE_ENDIAN
#endif

#if CXK_BYTE_ORDER == CXK_BIG_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t){
    return t;
}

template<class T>
T byteswapOnBigEndian(T t){
    return byteswap(t);
}

#else 

template<class T>
T byteswapOnLittleEndian(T t){
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t){
    return t;
}

#endif

}