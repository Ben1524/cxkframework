#include "cxk/application.h"


int main(int argc, char** argv){
    cxk::Application app;
    if(app.init(argc, argv)){
        return app.run();
    }

    return 0;
}