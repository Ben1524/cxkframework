#pragma once

#include <map>
#include <functional>
#include <iostream>
#include "Singleton.h"
#include <shared_mutex>
#include <fstream>
#include <list>
#include <string>
#include "util.h"
#include <memory>
#include <stdarg.h>
#include <tuple>
#include <ctime>
#include <vector>
#include <sstream>
#include "Thread.h"

// 使用流式方式将日志写入到logger
#define CXK_LOG_LEVEL(logger, level)                                                                         \
    if (logger->getLogLevel() <= level)                                                                      \
    cxk::LogEventWrap(cxk::LogEvent::ptr(new cxk::LogEvent(logger, level, __FILE__, __LINE__, 0,             \
                                                           cxk::getThreadId(), cxk::getFiberId(), time(0), cxk::Thread::GetName()))) \
        .getSS()

#define CXK_LOG_DEBUG(logger) CXK_LOG_LEVEL(logger, cxk::LogLevel::DEBUG)
#define CXK_LOG_INFO(logger) CXK_LOG_LEVEL(logger, cxk::LogLevel::INFO)
#define CXK_LOG_WARN(logger) CXK_LOG_LEVEL(logger, cxk::LogLevel::WARN)
#define CXK_LOG_ERROR(logger) CXK_LOG_LEVEL(logger, cxk::LogLevel::ERROR)
#define CXK_LOG_FATAL(logger) CXK_LOG_LEVEL(logger, cxk::LogLevel::FATAL)


// 使用格式化方式将日志写入到logger
#define CXK_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                                  \
    if (logger->getLogLevel() <= level)                                                                                             \
    cxk::LogEventWrap(cxk::LogEvent::ptr(new cxk::LogEvent(logger, level,                                                           \
                                 __FILE__, __LINE__, 0, cxk::getThreadId(), cxk::getFiberId(), time(0), cxk::Thread::GetName()))) \
        .getEvent()                                                                                                                 \
        ->format(fmt, __VA_ARGS__)

#define CXK_LOG_FMT_DEBUG(logger, fmt, ...) CXK_LOG_FMT_LEVEL(logger, cxk::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define CXK_LOG_FMT_INFO(logger, fmt, ...) CXK_LOG_FMT_LEVEL(logger, cxk::LogLevel::INFO, fmt, __VA_ARGS__)
#define CXK_LOG_FMT_WARN(logger, fmt, ...) CXK_LOG_FMT_LEVEL(logger, cxk::LogLevel::WARN, fmt, __VA_ARGS__)
#define CXK_LOG_FMT_ERROR(logger, fmt, ...) CXK_LOG_FMT_LEVEL(logger, cxk::LogLevel::ERROR, fmt, __VA_ARGS__)
#define CXK_LOG_FMT_FATAL(logger, fmt, ...) CXK_LOG_FMT_LEVEL(logger, cxk::LogLevel::FATAL, fmt, __VA_ARGS__)

#define CXK_LOG_ROOT() cxk::LoggerMar::GetInstance()->getRoot()
#define CXK_LOG_NAME(name) cxk::LoggerMar::GetInstance()->getLogger(name)

namespace cxk
{
    class Logger;

    class LogLevel
    {
    public:
        /// @brief 日志级别
        enum Level
        {
            UNKONWING = 0,  // 未知
            DEBUG = 1,      // 调试
            INFO = 2,       // 信息
            WARN = 3,       // 警告
            ERROR = 4,      // 错误
            FATAL = 5       // 致命
        };

        /// @brief 将日志转换为字符串
        /// @param level 日志级别
        /// @return 日志级别对应的字符串
        static const char *ToString(LogLevel::Level level);


        /// @brief 将文本转换为日志级别
        /// @param str 日志级别对应的字符串
        /// @return     日志级别
        static LogLevel::Level FromString(const std::string& str);
    };

    /*
    日志事件，用于记录日志现场，比如该日志的级别，文件名/行号，日志消息，线程/协程号，所属日志器名称等。
    */
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;

        /// @brief 构造函数
        /// @param logger 日志器
        /// @param level 日志级别
        /// @param file 文件名
        /// @param m_line 文件行号
        /// @param elapse 程序启动依赖的耗时
        /// @param thread_id 线程id
        /// @param fiber_id 协程id
        /// @param time     当前时间（秒）
        /// @param thread_name 线程名称
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                const char *file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                const std::string& thread_name);

        ~LogEvent();

        const char *get_file() const { return m_file; }
        int32_t get_line() const { return m_line; }
        uint32_t get_threadID() const { return m_threadID; }
        uint32_t get_fiberID() const { return m_fiberID; }
        std::string get_content() const { return m_ss.str(); }
        uint32_t get_elapse() const { return m_elapse; }
        uint64_t get_time() const { return m_time; }
        std::shared_ptr<Logger> get_logger() const { return m_logger; }
        LogLevel::Level get_level() const { return m_level; }
        std::string get_threadName() const { return m_threadName; }

        
        std::stringstream &getSS() { return m_ss; }

        /// @brief 格式化写入日志内容
        /// @param fmt 格式化字符串
        /// @param  ... 格式化参数列表
        void format(const char *fmt, ...); // 将fmt字符串格式化输出到m_ss中
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr; // 文件名
        int32_t m_line;               // 行号
        uint32_t m_threadID;          // 线程ID
        uint32_t m_fiberID;           // 协程
        uint32_t m_elapse;            // 程序启动到现在的毫秒数
        uint64_t m_time;              // 当前时间
        LogLevel::Level m_level;      // 日志级别
        std::string m_threadName;     // 线程名

        std::stringstream m_ss; // 日志内容

        std::shared_ptr<Logger> m_logger; // 容器类
    };


    /*
    日志事件包装类，在日志现场构造，包装了日志器和日志事件两个对象，
    在日志记录结束后，LogEventWrap析构时，调用日志器的log方法输出日志事件。
    */
    class LogEventWrap{
    public:
        /// @brief 构造函数
        /// @param e 日志事件
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();

        /// @brief 获取日志事件
        /// @return 日志事件
        LogEvent::ptr getEvent();

        /// @brief 获取日志内容流
        /// @return 
        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };

    // 日志格式类
    /*
    日志格式器，与log4cpp的PatternLayout对应，用于格式化一个日志事件。
    该类构建时可以指定pattern，表示如何进行格式化。提供format方法，用于将日志事件格式化成字符串。
    */
    class LogFormat
    {
    public:
        using ptr = std::shared_ptr<LogFormat>;
        
        /// @brief 日志内容格式化
        /// @param logger 日志器
        /// @param level    日志等级
        /// @param event    日志事件
        /// @return 
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);


        /// @brief 构造函数
        /// @param pattern 格式模板
        /*
            *  %m 消息
            *  %p 日志级别
            *  %r 累计毫秒数
            *  %c 日志名称
            *  %t 线程id
            *  %n 换行
            *  %d 时间
            *  %f 文件名
            *  %l 行号
            *  %T 制表符
            *  %F 协程id
            *  %N 线程名称
            *
            *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
        */
        LogFormat(const std::string &pattern);

        /// @brief 获取日志模板
        /// @return 日志模板
        const std::string getPattern() const {return m_pattern;}

    public:
        class FormatItem // 格式化项类
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        /// @brief 初始化，解析日志模板
        void init();

        bool isError() const { return m_error; }

    private:
        std::string m_pattern;
        std::list<FormatItem::ptr> m_items;
        bool m_error = false;
    };


    /*
    日志输出器，用于将一个日志事件输出到对应的输出地。
    该类内部包含一个LogFormatter成员和一个log方法，日志事件先经过LogFormatter格式化后再输出到对应的输出地。
    从这个类可以派生出不同的Appender类型，比如StdoutLogAppender和FileLogAppender，分别表示输出到终端和文件。        
    */
    class LogAppender
    {
    friend class Logger;
    public:
        using ptr = std::shared_ptr<LogAppender>;
        using MutexType = Spinlock;
        ~LogAppender() {}

        /// @brief 写入日志
        /// @param ptr 日志器
        /// @param level 日志级别
        /// @param event 日志事件
        virtual void log(std::shared_ptr<Logger> ptr, LogLevel::Level level, LogEvent::ptr event) = 0;


        /// @brief 将日志输出的目标配置成YAML String
        virtual std::string toYamlString() = 0;

        /// @brief 更改日志格式器
        /// @param format  日志格式器
        void setFormat(LogFormat::ptr format);
        LogFormat::ptr getFormat(); 

        LogLevel::Level getLogLevel() const { return m_level; }
        void setLogLevel(LogLevel::Level level) { m_level = level; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG; // 日志级别
        bool m_hasFormat = false;                  // 是否有格式化字符串
        LogFormat::ptr m_format;
        MutexType m_mutex;
    };

    /*
    日志器，负责进行日志输出。一个Logger包含多个LogAppender和一个日志级别，
    提供log方法，传入日志事件，判断该日志事件的级别高于日志器本身的级别之后调用LogAppender将日志进行输出，否则该日志被抛弃。
    */
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        using ptr = std::shared_ptr<Logger>;
        using MutexType = Spinlock;

        /// @brief 构造函数
        /// @param name 日志器名称
        Logger(const std::string &name = "root");

        /// @brief 写日志
        /// @param level 日志级别
        /// @param event 日志事件
        void log(LogLevel::Level level, LogEvent::ptr event);

        /// @brief 添加日志目标
        /// @param appender     日志目标
        void addAppender(LogAppender::ptr appender);

        /// @brief 删除日志目标
        /// @param appender 日志目标
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();
        LogLevel::Level getLogLevel() const { return m_level; }
        void setLogLevel(LogLevel::Level level) { m_level = level; }

        /// @brief 写debug级别的日志
        /// @param event    日志事件
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);
        
        /// @brief 设置日志格式器
        void setFormatter(LogFormat::ptr format);
        void setFormatter(const std::string& val);
        LogFormat::ptr getFormatter();

        const std::string &getName() const { return m_name; }

        /// @brief 获取日志格式器
        std::string toYamlString();

    private:
        std::string m_name;                      // 日志器名称
        LogLevel::Level m_level;                 // 日志级别
        std::list<LogAppender::ptr> m_appenders; // 日志appender列表
        LogFormat::ptr m_format;                 // 日志格式
        Logger::ptr m_root;
        MutexType m_mutex;
    };

    // 输出到控制台的appender
    class StdOutAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdOutAppender>;
        std::string toYamlString() override;
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    private:
    };

    // 输出到文件的appender
    class FileAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileAppender>;
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;


        FileAppender(const std::string &filename);


        /// @brief 重新打开日志文件
        /// @return 成功返回True
        bool reOpen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
        // 上次打开的时间
        uint64_t m_lastTime;    
    };


    /*
    日志器管理类，单例模式，用于统一管理所有的日志器，提供日志器的创建与获取方法。
    LogManager自带一个root Logger，用于为日志模块提供一个初始可用的日志器。
    */
    class LoggerManager{
    public:
        using MutexType = Spinlock;

        /// @brief 获取日志器
        /// @param name 日志器名称
        Logger::ptr getLogger(const std::string &name);
        LoggerManager();

        void init();

        /// @brief 返回主日志器
        /// @return 主日志器对象
        Logger::ptr getRoot() const { return m_root; }

        /// @brief 将所有的日志器配置转为YAML String
        /// @return 
        std::string toYamlString();

    private:
        // 日志器容器
        std::map<std::string, Logger::ptr> m_logger;
        Logger::ptr m_root;
        MutexType m_mutex;
    };

    /// @brief 日志器管理类单例模式
    typedef cxk::Singleton<LoggerManager> LoggerMar;
}
