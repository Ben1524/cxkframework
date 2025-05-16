cmake_minimum_required(VERSION 3.15) # vcpkg需要3.15+
set(CMAKE_TOOLCHAIN_FILE "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/opt/vcpkg/installed/x64-linux/share")
# set(CMAKE_TOOLCHAIN_FILE "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")
# if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
#     set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
#         CACHE STRING "Vcpkg toolchain file")
#     message(STATUS "Using vcpkg toolchain file: $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
# endif()

PROJECT(CXK_SERVER)
# 设置vcpkg工具链（如果未通过命令行设置）


cmake_policy(SET CMP0167 NEW) # 使用新的Boost查找方式

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 调试配置
set(CMAKE_CXX_FLAGS_DEBUG "-rdynamic -O0 -ggdb -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined")

option(TEST "ON for complile test" ON)

# 使用vcpkg方式查找包
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(protobuf CONFIG REQUIRED) 
find_package(ZLIB REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
# find_package(MySQL REQUIRED)
#  if(MYSQL_FOUND)
#     MESSAGE("MYSQL_FOUND =================================================== ")
#     include_directories(${MYSQL_INCLUDE_DIR})
#  endif()
set(MYSQL_CONNECTOR_CXX_INCLUDE_DIR "/usr/include/mysql-cppconn-8" CACHE PATH "MySQL Connector/C++ 头文件路径")
set(MYSQL_CONNECTOR_CXX_LIBRARIES "/usr/lib/x86_64-linux-gnu/libmysqlcppconn.so" CACHE FILEPATH "MySQL Connector/C++ 库文件路径")
include_directories(${MYSQL_CONNECTOR_CXX_INCLUDE_DIR})
 
# 打印找到的protobuf信息
message(STATUS "protobuf version: ${protobuf_VERSION}")
message(STATUS "protobuf libraries: ${protobuf_LIBRARIES}")
message(STATUS "protobuf include dirs: ${protobuf_INCLUDE_DIRS}")
include_directories(.)

set(LIB_SRC 
    cxk/config.cpp
    cxk/fiber.cpp
    cxk/iomanager.cpp
    cxk/logger.cpp
    cxk/scheduler.cpp
    cxk/Thread.cpp
    cxk/timer.cpp
    cxk/util.cpp
    cxk/hook.cpp
    cxk/address.cpp
    cxk/socket.cpp
    cxk/bytearray.cpp
    cxk/fd_manager.cpp
    cxk/tcp_server.cpp
    cxk/stream.cpp
    cxk/uri.rl.cpp
    cxk/daemon.cpp
    cxk/env.cpp
    cxk/application.cpp
    cxk/work.cpp
    cxk/mutex.cpp
    cxk/library.cpp
    cxk/module.cpp
    cxk/protocol.cpp
    cxk/http/http.cpp
    cxk/http/http_parser.cpp
    cxk/http/http11_parser.rl.cpp
    cxk/http/httpclient_parser.rl.cpp
    cxk/http/http_session.cpp
    cxk/http/http_server.cpp
    cxk/http/servlet.cpp
    cxk/http/http_connection.cpp
    cxk/http/ws_connection.cpp
    cxk/http/ws_server.cpp
    cxk/http/ws_servlet.cpp
    cxk/http/ws_session.cpp
    cxk/util/hash_util.cpp
    cxk/util/json_util.cpp
    cxk/util/crypto_util.cpp
    cxk/stream/socket_stream.cpp
    cxk/stream/zlib_stream.cpp
    cxk/stream/async_socket_stream.cpp
    cxk/stream/load_balance.cpp
    cxk/rock/rock_stream.cpp
    cxk/rock/rock_protocol.cpp
    cxk/rock/rock_server.cpp
    cxk/db/mysql.cpp
    cxk/db/sqlite3.cpp
    )

ragelmaker(cxk/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/cxk/http)
ragelmaker(cxk/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/cxk/http)
ragelmaker(cxk/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/cxk)

add_library(cxk SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(cxk)
# 使用现代CMake方式链接库
target_link_libraries(cxk PUBLIC
    Boost::system
    Boost::thread
    OpenSSL::Crypto
    OpenSSL::SSL
    protobuf::libprotoc
    protobuf::libprotobuf
    protobuf::libprotobuf-lite
    ZLIB::ZLIB
    yaml-cpp
    libmysqlclient.so
    JsonCpp::JsonCpp
    sqlite3
    pthread
    asio::asio
    dl
)
# find_library(YAMLCPP yaml-cpp)
# find_library(PTHREAD pthread)


# find_library(JSONCPP_LIBRARY NAMES jsoncpp)


# set(LIBS cxk pthread yaml-cpp mysqlclient sqlite3 ${JSONCPP_LIBRARY} dl z ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${protobuf_LIBRARIES})
# message("***" , ${LIBS})

set(LIBS cxk 
    Boost::system
    Boost::thread
    OpenSSL::Crypto
    OpenSSL::SSL
    protobuf::libprotoc
    protobuf::libprotobuf
    protobuf::libprotobuf-lite
    ZLIB::ZLIB
    yaml-cpp
    libmysqlclient.so
    JsonCpp::JsonCpp
    sqlite3
    pthread
    asio::asio
    dl)

if(TRUE)

add_executable(main_config test/test_config.cpp)
add_dependencies(main_config cxk)
force_redefine_file_macro_for_sources(main_config)
target_link_libraries(main_config ${LIBS})



add_executable(main_hook test/test_hook.cpp)
add_dependencies(main_hook cxk)
force_redefine_file_macro_for_sources(main_hook)
target_link_libraries(main_hook ${LIBS})

add_executable(main_address test/test_address.cpp)
add_dependencies(main_address cxk)
force_redefine_file_macro_for_sources(main_address)
target_link_libraries(main_address ${LIBS})


add_executable(main_socket test/test_socket.cpp)
add_dependencies(main_socket cxk)
force_redefine_file_macro_for_sources(main_socket)
target_link_libraries(main_socket ${LIBS})


add_executable(main_byteArray test/test_byteArray.cpp)
add_dependencies(main_byteArray cxk)
force_redefine_file_macro_for_sources(main_byteArray)
target_link_libraries(main_byteArray ${LIBS})


add_executable(main_http test/test_http.cpp)
add_dependencies(main_http cxk)
force_redefine_file_macro_for_sources(main_http)
target_link_libraries(main_http ${LIBS})


add_executable(main_parser test/test_parser.cpp)
add_dependencies(main_parser cxk)
force_redefine_file_macro_for_sources(main_parser)
target_link_libraries(main_parser ${LIBS})


add_executable(main_tcp_server test/test_tcp_server.cpp)
add_dependencies(main_tcp_server cxk)
force_redefine_file_macro_for_sources(main_tcp_server)
target_link_libraries(main_tcp_server ${LIBS})

add_executable(main_http_server test/test_http_server.cpp)
add_dependencies(main_http_server cxk)
force_redefine_file_macro_for_sources(main_http_server)
target_link_libraries(main_http_server ${LIBS})

add_executable(main_http_connection test/test_http_connection.cpp)
add_dependencies(main_http_connection cxk)
force_redefine_file_macro_for_sources(main_http_connection)
target_link_libraries(main_http_connection ${LIBS})


add_executable(main_echo examples/echo_server.cpp)
add_dependencies(main_echo cxk)
force_redefine_file_macro_for_sources(main_echo)
target_link_libraries(main_echo ${LIBS})

add_executable(http_server examples/http_server.cpp)
add_dependencies(http_server cxk)
force_redefine_file_macro_for_sources(http_server)
target_link_libraries(http_server ${LIBS})



add_executable(main_uri test/test_uri.cpp)
add_dependencies(main_uri cxk)
force_redefine_file_macro_for_sources(main_uri)
target_link_libraries(main_uri ${LIBS})


add_executable(main_daemon test/test_daemon.cpp)
add_dependencies(main_daemon cxk)
force_redefine_file_macro_for_sources(main_daemon)
target_link_libraries(main_daemon ${LIBS})


add_executable(main_env test/test_env.cpp)
add_dependencies(main_env cxk)
force_redefine_file_macro_for_sources(main_env)
target_link_libraries(main_env ${LIBS})


add_executable(main_sql test/test_mysql.cpp)
add_dependencies(main_sql cxk)
force_redefine_file_macro_for_sources(main_sql)
target_link_libraries(main_sql ${LIBS})



add_executable(main_application test/test_application.cpp)
add_dependencies(main_application cxk)
force_redefine_file_macro_for_sources(main_application)
target_link_libraries(main_application ${LIBS})

add_executable(main_ws_server test/test_ws_server.cpp)
add_dependencies(main_ws_server cxk)
force_redefine_file_macro_for_sources(main_ws_server)
target_link_libraries(main_ws_server ${LIBS})


add_executable(main_ws_client test/test_ws_client.cpp)
add_dependencies(main_ws_client cxk)
force_redefine_file_macro_for_sources(main_ws_client)
target_link_libraries(main_ws_client ${LIBS})


add_executable(application examples/application.cpp)
add_dependencies(application cxk)
force_redefine_file_macro_for_sources(application)
target_link_libraries(application ${LIBS})


cxk_add_executable(test_lru "test/test_lru.cpp" cxk "${LIBS}")
cxk_add_executable(test_zlib_stream "test/test_zlib_stream.cpp" cxk "${LIBS}")
cxk_add_executable(test_json "test/test_json.cpp" cxk "${LIBS}")
cxk_add_executable(test_rock "test/test_rock.cpp" cxk "${LIBS}")
cxk_add_executable(test_sqlite3 "test/test_sqlite3.cpp" cxk "${LIBS}")

add_library(test_module SHARED test/test_module.cpp)

endif()

cxk_add_executable(bin_cxk "cxk/main.cpp" cxk "")
set_target_properties(bin_cxk PROPERTIES OUTPUT_NAME "cxk")


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


