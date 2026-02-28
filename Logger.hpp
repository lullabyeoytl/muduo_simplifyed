#pragma once

#include <string>
#include "noncopyable.hpp"

// 定义日志级别
enum LogLevel {
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // 崩溃信息
    DEBUG, // 调试信息
};

// 日志类：单例模式，禁止拷贝
class Logger : noncopyable {
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_;
    Logger() {} // 构造函数私有化
};

// --- 定义用户调用的便捷宏 ---
// 使用 do-while(0) 是为了防止宏在各种 if-else 语句中产生逻辑错误

#define LOG_INFO(logMsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_ERROR(logMsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_FATAL(logMsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

// DEBUG 级别通过宏开关控制，默认不开启以减轻 IO 负担
#ifdef MU_DEBUG
#define LOG_DEBUG(logMsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
#define LOG_DEBUG(logMsgFormat, ...)
#endif