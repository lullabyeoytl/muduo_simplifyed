#include "Logger.hpp"
#include <iostream>
#include "timestamp.hpp"

// 获取单例
Logger &Logger::instance() {
  static Logger logger;
  return logger;
}

// 设置级别
void Logger::setLogLevel(int level) { logLevel_ = level; }

// 写日志：格式 [级别] 时间 : msg
void Logger::log(std::string msg) {
  switch (logLevel_) {
  case INFO:
    std::cout << "[INFO]";
    break;
  case ERROR:
    std::cout << "[ERROR]";
    break;
  case FATAL:
    std::cout << "[FATAL]";
    break;
  case DEBUG:
    std::cout << "[DEBUG]";
    break;
  default:
    break;
  }

  // 打印时间 (此处欠缺 Timestamp 类，下节课补全)
  // std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
  std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}