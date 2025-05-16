#include "cxk/module.h"

namespace name_space{

class MyModule : public cxk::Module{
public:
    using ptr = std::shared_ptr<MyModule>;

    MyModule();

    bool onLoad() override;
    bool onUnload() override;

    bool onServerReady() override;
    bool onServerUp() override;

};


}