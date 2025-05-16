#pragma once
#include <memory>
#include "module.h"


namespace cxk{


class Library{
public:
    static Module::ptr GetModule(const std::string& path);
};


}