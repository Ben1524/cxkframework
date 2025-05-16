#include "cxk/logger.h"
#include "cxk/util.h"
#include <iostream>
#include <thread>
#include "cxk/config.h"

int main(int argc, char *argv[])
{
    cxk::Logger::ptr logger(new cxk::Logger);
    logger->addAppender(cxk::LogAppender::ptr(new cxk::StdOutAppender));

    CXK_LOG_DEBUG(logger) << "this is a debug message";
    CXK_LOG_INFO(logger) << "this is a info message";
    CXK_LOG_WARN(logger) << "this is a warn message";
    CXK_LOG_ERROR(logger) << "this is a error message";
    CXK_LOG_FATAL(logger) << "this is a fatal message";

    char buf[] = "hello world";
    CXK_LOG_FMT_DEBUG(logger, "this is a debug message with format: %s", buf);

    cxk::Logger::ptr logger2(new cxk::Logger);
    cxk::FileAppender::ptr fileAppender(new cxk::FileAppender("/home/cxk/mydir/VSC_Code/WebServe/bin/log.txt"));
    fileAppender->setFormat(cxk::LogFormat::ptr(new cxk::LogFormat("%d%T%p%T%m%n")));
    fileAppender->setLogLevel(cxk::LogLevel::ERROR);
    logger2->addAppender(fileAppender);

    CXK_LOG_FMT_DEBUG(logger2, "this is a debug message with format: %s", buf);
    CXK_LOG_FMT_ERROR(logger2, "this is a error message with format: %s", buf);
    CXK_LOG_FMT_WARN(logger2, "this is a warn message with format: %s", buf);
    CXK_LOG_FMT_INFO(logger2, "this is a info message with format: %s", buf);
    CXK_LOG_FMT_FATAL(logger2, "this is a fatal message with format: %s", buf);

    auto i = cxk::LoggerMar::GetInstance()->getLogger("xx");
    CXK_LOG_DEBUG(i) << "this is XX ? no !";

    return 0;
}