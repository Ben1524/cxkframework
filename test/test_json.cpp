#include <iostream>
#include "cxk/util.h" // 假设 JsonUtil 的实现和声明都在这个头文件中
#include "cxk/util/json_util.h"

// 测试函数
void TestJsonUtil() {
    // 测试 NeedEscape 和 Escape 方法
    std::string needEscapeStr = R"(Hello\"World)";
    bool needEscapeResult = cxk::JsonUtil::NeedEscape(needEscapeStr);
    std::cout << "NeedEscape: " << (needEscapeResult ? "Passed" : "Failed") << std::endl;
    
    std::string escapedStr = cxk::JsonUtil::Escape(needEscapeStr);
    std::cout << "Escape: Original: " << needEscapeStr << ", Escaped: " << escapedStr << std::endl;

    // 测试 GetString 方法
    Json::Value json;
    json["test"] = "value";
    std::string getStringResult = cxk::JsonUtil::GetString(json, "test", "default");
    std::cout << "GetString: " << (getStringResult == "value" ? "Passed" : "Failed") << std::endl;

    // 测试 GetDouble, GetInt32, GetUint32, GetInt64, GetUint64 方法
    json["number"] = 10.5;
    double getDoubleResult = cxk::JsonUtil::GetDouble(json, "number", 0.0);
    std::cout << "GetDouble: " << (getDoubleResult == 10.5 ? "Passed" : "Failed") << std::endl;

    json["int32"] = 10;
    int32_t getInt32Result = cxk::JsonUtil::GetInt32(json, "int32", 0);
    std::cout << "GetInt32: " << (getInt32Result == 10 ? "Passed" : "Failed") << std::endl;

    // 类似的测试可以为 GetUint32, GetInt64, GetUint64 编写...

    // 测试 FromString 和 ToString 方法
    std::string jsonString = "{\"key\":\"value\"}";
    Json::Value parsedJson;
    bool fromStringResult = cxk::JsonUtil::FromString(parsedJson, jsonString);
    std::cout << "FromString: " << (fromStringResult ? "Passed" : "Failed") << std::endl;

    std::string toStringResult = cxk::JsonUtil::ToString(parsedJson);
    std::cout << "ToString: " << (toStringResult.find("\"key\":\"value\"") != std::string::npos ? "Passed" : "Failed") << std::endl;
}

int main() {
    TestJsonUtil();
    return 0;
}