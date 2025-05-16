cmake_minimum_required(VERSION 2.8)
project(project_name)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cxkframework)

include(cxkframework/cmake/utils.cmake)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")


include_directories(.)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cxkframework)


find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()


set(LIB_SRC
        template/my_module.cpp
    )


add_library(project_name SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(project_name)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)