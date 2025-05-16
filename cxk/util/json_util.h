#pragma once


#include <string>
#include <iostream>
#include <json/json.h>


namespace cxk{

class JsonUtil{
public:
    /**
     * @brief           判断字符串是否需要转义，例如：包含双引号、反斜杠等特殊字符时，则需要转义。
     * 
     * @param v         是否需要转义
     * @return true     如果需要转义，则返回true；
     * @return false    如果不需要转义，则返回false。
     */
    static bool NeedEscape(const std::string& v);
    static std::string Escape(const std::string& v);
    static std::string GetString(const Json::Value& json, const std::string& name, const std::string& default_value = "");

    static double GetDouble(const Json::Value& json, const std::string& name, double default_value = 0);
    static int32_t GetInt32(const Json::Value& json, const std::string& name, int32_t default_value = 0);
    static uint32_t GetUint32(const Json::Value& json, const std::string& name, uint32_t default_value = 0);
    static int64_t GetInt64(const Json::Value& json, const std::string& name, int64_t default_value = 0);
    static uint64_t GetUint64(const Json::Value& json, const std::string& name, uint64_t default_value = 0);

    static bool FromString(Json::Value& json, const std::string& v);
    static std::string ToString(const Json::Value& json);
};




}