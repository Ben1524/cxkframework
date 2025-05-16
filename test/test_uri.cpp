#include "cxk/uri.h"
#include <iostream>

int main(){
    cxk::Uri::ptr uri = cxk::Uri::Create("http://www.sylar.top:8080/test/uri?id=100&name=sylar#frg");
    std::cout << uri->toString() << std::endl;

    auto addr = uri->createAddress();

    std::cout << addr->toString() << std::endl;

    return 0;
}
