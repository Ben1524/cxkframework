#include "logger.h"
#include "config.h"
namespace cxk
{

    class ThreadNameFormatItem : public LogFormat::FormatItem{
    public:
        ThreadNameFormatItem(const std::string &fmt = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
            os << event->get_threadName();
        }
    };


    class MessageFormatItem : public LogFormat::FormatItem
    {
    public:
        MessageFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_content();
        }
    };

    class LevelFormatItem : public LogFormat::FormatItem
    {
    public:
        LevelFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormat::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_elapse();
        }
    };

    class NameFormatItem : public LogFormat::FormatItem
    {
    public:
        NameFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            // os << logger->getName();
            os << event->get_logger()->getName();
        }
    };

    class ThreadIDFormatItem : public LogFormat::FormatItem
    {
    public:
        ThreadIDFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_threadID();
        }
    };

    class FiberIDFormatItem : public LogFormat::FormatItem
    {
    public:
        FiberIDFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_fiberID();
        }
    };

    class TimeFormatItem : public LogFormat::FormatItem
    {
    public:
        TimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            struct tm tm;
            time_t time = event->get_time();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FileNameFormatItem : public LogFormat::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_file();
        }
    };

    class LineFormatItem : public LogFormat::FormatItem
    {
    public:
        LineFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << event->get_line();
        }
    };

    class NewLineFormatItem : public LogFormat::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &fmt) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormat::FormatItem
    {
    public:
        StringFormatItem(const std::string &string) : m_string(string)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormat::FormatItem
    {
    public:
        TabFormatItem(const std::string &string)
        {
        }

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
        {
            os << "\t";
        }
    };

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t m_line,
                       uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string& thread_name)
        : m_file(file), m_line(m_line), m_threadID(thread_id), m_fiberID(fiber_id), m_elapse(elapse), m_time(time),
          m_level(level), m_threadName(thread_name), m_logger(logger){
    }

    LogEvent::~LogEvent()
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    LogEvent::ptr LogEventWrap::getEvent()
    {
        return m_event;
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        m_event->get_logger()->log(m_event->get_level(), m_event); // 这里保持怀疑？是否写错了
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG)
    {
        m_format.reset(new LogFormat("%d%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m\n"));
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            MutexType::Lock lock(m_mutex);

            if (!m_appenders.empty()){
                for (auto &appender : m_appenders){
                    appender->log(self, level, event);
                }
            }
            else if (m_root){
                m_root->log(level, event);
            }
        }
    }

    // 添加一个appender
    void Logger::addAppender(LogAppender::ptr appender){
        MutexType::Lock lock(m_mutex);
        if (!appender->getFormat())
        {
            MutexType::Lock ll(appender->m_mutex);
            appender->m_format = m_format;
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (appender == *it)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    void FileAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
        if (level >= m_level){
            uint64_t now = time(0);
            if(now != m_lastTime){
                reOpen();
                m_lastTime = now;
            }
            MutexType::Lock lock(m_mutex);
            if(!(m_filestream << m_format->format(logger, level, event))) {
                // std::cout << "is open: " << m_filestream.is_open() << std::endl;
                std::cout << "log errror"  << std::endl;
            }
        }
    }

    void StdOutAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level){
            MutexType::Lock lock(m_mutex);
            std::cout << m_format->format(logger, level, event);
        }
    }

    bool FileAppender::reOpen(){
        MutexType::Lock lock(m_mutex);
        if (m_filestream){
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios::app);
        // std::cout << "open file: " << m_filename << " " << m_filestream.is_open() << std::endl;

        return !!m_filestream;
    }

    FileAppender::FileAppender(const std::string &filename) : m_filename(filename)
    {
        int ret = reOpen();
    }

    LogFormat::LogFormat(const std::string &parttern) : m_pattern(parttern)
    {
        init();
    }

    void LogFormat::init()
    {
        /*
        %m 消息
        %p 日志级别
        %c 日志器名称
        %d 日期时间，后面可跟一对括号指定时间格式，比如%d{%Y-%m-%d %H:%M:%S}，这里的格式字符与C语言strftime一致
        %r 该日志器创建后的累计运行毫秒数
        %f 文件名
        %l 行号
        %t 线程id
        %F 协程id
        %N 线程名称
        %% 百分号
        %T 制表符
        %n 换行
        */


        // string format type
        std::vector<std::tuple<std::string, std::string, int>> vec;

        std::string nstr;   //用来临时存储非格式字符串
        for (size_t i = 0; i < m_pattern.size(); ++i) // 解析格式
        {
            if (m_pattern[i] != '%') // 如果不是%， 则直接写入nstr
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%') // 连续两个%， 则将%视为一个普通字符
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0; // 用来记录当前状态， 0表示没有解析到{}， 1表示解析到了{， 2表示已经解析到了}
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size())
            {
                if (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}') // 既不是字母也不是'{'或 '}'
                {
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i - 1); // 将%到｛之间的字符串写入str字符串
                        fmt_status = 1;                           // 标记从｛开始解析
                        fmt_begin = n;                            // 记录从{解析的索引
                        ++n;
                        continue;
                    }
                }
                if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1); // 将{}之间的字符串写入fmt字符串
                        fmt_status = 2;
                        break;
                    }
                }
                ++n;
            }

            if (fmt_status == 0) // 没有找到{},将nstr作为一个字符串格式添加到vec
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }

                str = m_pattern.substr(i + 1, n - i - 1);    // 从i到下一组%之间的字符串写入str字符串
                vec.push_back(std::make_tuple(str, fmt, 1)); // fmt其实为空， 只是为了占位
                i = n - 1;
            }
            else if (fmt_status == 1) // 找到{， 但是没有找到}，格式出错
            {
                std::cout << "pattern error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
                m_error = true;
            }
            else if (fmt_status == 2) // 找到{}，将{}之间的内容都添加到vec中
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
        }

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items =
            {

#define XX(str, c)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new c(fmt)); } \
    }

                XX(m, MessageFormatItem),
                XX(p, LevelFormatItem),
                XX(r, ElapseFormatItem),
                XX(c, NameFormatItem),
                XX(t, ThreadIDFormatItem),
                XX(n, NewLineFormatItem),
                XX(d, TimeFormatItem),
                XX(f, FileNameFormatItem),
                XX(l, LineFormatItem),
                XX(T, TabFormatItem),
                XX(F, FiberIDFormatItem),
                XX(S, StringFormatItem),
                XX(N, ThreadNameFormatItem)
#undef XX
            };
        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
    }

    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {

#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKNOWN";
        }

        return "UNKNOWN";
    }

    std::string LogFormat::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        // std::cout << "format: " << ss.str() << std::endl;
        return ss.str();
    }

    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger());

        m_root->addAppender(LogAppender::ptr(new StdOutAppender));

        m_logger[m_root->m_name] = m_root;

        init();
    }

    struct LogAppenderDefine
    {
        int type = 0; // 1:file, 2:stdout
        LogLevel::Level level = LogLevel::UNKONWING;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &rhs) const
        {
            return type == rhs.type && level == rhs.level && formatter == rhs.formatter && file == rhs.file;
        }
    };


    //用于定义和配置logger
    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKONWING;
        std::string formatter;

        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &rhs) const
        {
            return name == rhs.name && level == rhs.level && formatter == rhs.formatter && appenders == rhs.appenders;
        }

        bool operator<(const LogDefine &rhs) const
        {
            return name < rhs.name;
        }
    };


    void Logger::clearAppenders()
    {
        m_appenders.clear();
    }


    void Logger::setFormatter(LogFormat::ptr format)
    {
        MutexType::Lock lock(m_mutex);
        m_format = format;

        for(auto& i : m_appenders){
            MutexType::Lock ll(i->m_mutex);
            if(!i->m_hasFormat){
                i->m_format = m_format;
            }
        }
    }


    void Logger::setFormatter(const std::string& val)
    {
        cxk::LogFormat::ptr new_val(new cxk::LogFormat(val));
        if(new_val->isError()){
            std::cout << "Logger setFormatter name = " << val << " error" << std::endl;
            return;
        }
        //m_format = new_val;
        setFormatter(new_val);
    }

    LogLevel::Level LogLevel::FromString(const std::string& str)
    {
        #define XX(level, v)   \
            if(str == #v){ return LogLevel::level; \
            }

        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);
        return LogLevel::UNKONWING;
        #undef XX
    }



    LogFormat::ptr Logger::getFormatter()
    {
        return m_format;
    }





    cxk::ConfigVar<std::set<LogDefine>>::ptr g_log_define =
        cxk::Config::Lookup("logs", std::set<LogDefine>{}, "logs config");

    
    //偏特化, 字符串转类
    template <>
    class LexicalCast<std::string, std::set<LogDefine>>
    {
    public:
        std::set<LogDefine> operator()(const std::string &v){
            YAML::Node node = YAML::Load(v);
            std::set<LogDefine> vec;

            for(size_t i = 0; i < node.size(); ++i){
                auto n = node[i];
                if(!n["name"].IsDefined()){
                    std::cout << "logConfig name error " << std::endl;
                    continue;
                }

                LogDefine ld;
                ld.name = n["name"].as<std::string>();
                ld.level = LogLevel::FromString( n["level"].IsDefined() ? n["level"].as<std::string>() : "");
                if(n["formatter"].IsDefined()) {
                    ld.formatter = n["formatter"].as<std::string>();
                }

                if(n["appenders"].IsDefined()){
                    for(size_t x = 0; x < n["appenders"].size(); ++x){
                        auto a = n["appenders"][x];
                        if(!a["type"].IsDefined()){
                            std::cout << "logConfig type error name:"<< ld.name << std::endl;
                            continue;
                        }
                        std::string type = a["type"].as<std::string>();
                        LogAppenderDefine lad;
                        if(type == "FileLogAppender"){
                            lad.type = 1;
                            if(!a["file"].IsDefined()) {
                                std::cout << "logConfig file error" << std::endl;
                                continue;
                            }
                            lad.file = a["file"].as<std::string>();
                            if(a["formatter"].IsDefined()){
                                lad.formatter = a["formatter"].as<std::string>();
                            }
                        }else if(type == "StdoutLogAppender"){
                            lad.type = 2;
                        }else {
                            std::cout << "logConfig type error" << std::endl;
                            continue;
                        }
                        ld.appenders.push_back(lad);
                    }
                }
                vec.insert(ld);
            }
            return vec;
        }
    };

    //偏特化，把类转为string
    template <>
    class LexicalCast<std::set<LogDefine>, std::string>
    {
    public:
        std::string operator()(const std::set<LogDefine> &v)
        {
            YAML::Node node;

            for(auto& i : v){
                YAML::Node n;
                n["name"] = i.name;
                if(i.level != LogLevel::UNKONWING){
                    n["level"] = LogLevel::ToString(i.level);
                }

                if(i.formatter.empty()){
                    n["formatter"] = i.formatter;
                }

                for(auto& a : i.appenders){
                    YAML::Node na;
                    if(a.type == 1){
                        na["type"] = "FileLogAppender";
                        na["file"] = a.file;
                    }else if(a.type == 2){
                        na["type"] = "StdoutLogAppender";
                    }

                    if(a.level != LogLevel::UNKONWING){
                        na["level"] = LogLevel::ToString(a.level);
                    }

                    if(!a.formatter.empty()){
                        na["formatter"] = a.formatter;
                    }

                    n["appenders"].push_back(na);
                }

                node.push_back(n);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    //设置日志格式
    void LogAppender::setFormat(LogFormat::ptr format) {
        MutexType::Lock lock(m_mutex);
        m_format = format;
        if(m_format){
            m_hasFormat = true;
        } else {
            m_hasFormat = false;
        }
    }

    LogFormat::ptr LogAppender::getFormat(){
        MutexType::Lock lock(m_mutex);
        return m_format;
    } 



    //把logger的配置转为string类型
    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        if(m_level != LogLevel::UNKONWING){
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_format){
            node["formatter"] = m_format->getPattern();
        }

        for(auto& i : m_appenders){
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }


    std::string StdOutAppender::toYamlString(){
        MutexType::Lock lock(m_mutex);

        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if(m_level != LogLevel::UNKONWING){
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_format && m_hasFormat){
            node["formatter"] = m_format->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    std::string LoggerManager::toYamlString(){
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for(auto& i : m_logger){
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }


    std::string FileAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;

        if(m_level != LogLevel::UNKONWING){
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_format && m_hasFormat){
            node["formatter"] = m_format->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }




    //用于监听日志配置文件的更改,对日志按照配置进行增加修改和添加
    struct LogIniter
    {
        LogIniter()
        {
            g_log_define->addListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value){
                CXK_LOG_INFO(CXK_LOG_ROOT()) << "on_logger_config_change";
                for (auto &i : new_value){
                    auto it = old_value.find(i);
                    cxk::Logger::ptr logger;
                    if(it == old_value.end()){
                        // 新增logger
                        //logger.reset(new cxk::Logger(i.name));
                        logger = CXK_LOG_NAME(i.name);
                    }else {
                        if(!(i == *it)){
                            // 修改的日志
                            logger = CXK_LOG_NAME(i.name);
                        } else {
                            continue;
                        }
                    }

                    logger->setLogLevel(i.level);
                    if(!i.formatter.empty()){
                        logger->setFormatter(i.formatter);
                    }
                    logger->clearAppenders();
                    for(auto& a : i.appenders){
                        cxk::LogAppender::ptr ap;
                        if(a.type == 1){
                            ap.reset(new cxk::FileAppender(a.file));
                        }else if(a.type == 2){
                            ap.reset(new cxk::StdOutAppender);
                        }
                        ap->setLogLevel(a.level);
                        if(!a.formatter.empty()){
                            LogFormat::ptr fmt(new LogFormat(a.formatter));
                            if(!fmt->isError()){
                                ap->setFormat(fmt);
                            }else {
                                std::cout << "LogFormat error" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for(auto& i : old_value){
                    auto it = new_value.find(i);
                    if(it == new_value.end()){
                        // 删除的日志
                        auto logger = CXK_LOG_NAME(i.name);
                        logger->setLogLevel((LogLevel::Level)100);  // 设置为无效级别，防止日志输出(即删除)
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

    static LogIniter __log_init;

    void LoggerManager::init()
    {
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name){
        MutexType::Lock lock(m_mutex);
        auto it = m_logger.find(name);
        if (it != m_logger.end()){
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_logger[name] = logger;
        return logger;
    }
}
