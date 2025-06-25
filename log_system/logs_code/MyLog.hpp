#pragma once
#include "Manager.hpp"
namespace mylog
{
    // 用户获取日志器
    AsyncLogger::ptr GetLogger(const std::string &name)
    {
        return LoggerManager::GetInstance().GetLogger(name);
    }
    // 用户获取默认日志器
    AsyncLogger::ptr DefaultLogger() { return LoggerManager::GetInstance().DefaultLogger(); }

// 简化用户使用，宏函数默认填上文件名+行号
#define Debug(fmt, ...) Debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Info(fmt, ...) Info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Warn(fmt, ...) Warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Error(fmt, ...) Error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Fatal(fmt, ...) Fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 默认日志器宏
#define LOGDEBUGDEFAULT(fmt, ...) mylog::DefaultLogger()->Debug(fmt, ##__VA_ARGS__)
#define LOGINFODEFAULT(fmt, ...) mylog::DefaultLogger()->Info(fmt, ##__VA_ARGS__)
#define LOGWARNDEFAULT(fmt, ...) mylog::DefaultLogger()->Warn(fmt, ##__VA_ARGS__)
#define LOGERRORDEFAULT(fmt, ...) mylog::DefaultLogger()->Error(fmt, ##__VA_ARGS__)
#define LOGFATALDEFAULT(fmt, ...) mylog::DefaultLogger()->Fatal(fmt, ##__VA_ARGS__)

// 注册自定义日志器宏
#define REGISTER_LOGGER(name)                                  \
    namespace mylog                                            \
    {                                                          \
        AsyncLogger::ptr GetLogger_##name()                    \
        {                                                      \
            static AsyncLogger::ptr logger = GetLogger(#name); \
            return logger;                                     \
        }                                                      \
    }

// 内部宏：实际展开逻辑
#define _LOGDEBUG(logger, ...) mylog::GetLogger_##logger()->Debug(__FILE__, __LINE__, __VA_ARGS__)
#define _LOGINFO(logger, ...) mylog::GetLogger_##logger()->Info(__FILE__, __LINE__, __VA_ARGS__)
#define _LOGWARN(logger, ...) mylog::GetLogger_##logger()->Warn(__FILE__, __LINE__, __VA_ARGS__)
#define _LOGERROR(logger, ...) mylog::GetLogger_##logger()->Error(__FILE__, __LINE__, __VA_ARGS__)
#define _LOGFATAL(logger, ...) mylog::GetLogger_##logger()->Fatal(__FILE__, __LINE__, __VA_ARGS__)

// 外部宏：拼接名字后转发
#define LOGDEBUG(name, ...) _LOGDEBUG(name, __VA_ARGS__)
#define LOGINFO(name, ...) _LOGINFO(name, __VA_ARGS__)
#define LOGWARN(name, ...) _LOGWARN(name, __VA_ARGS__)
#define LOGERROR(name, ...) _LOGERROR(name, __VA_ARGS__)
#define LOGFATAL(name, ...) _LOGFATAL(name, __VA_ARGS__)

} // namespace mylog