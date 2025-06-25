#include<unordered_map>
#include"AsyncLogger.hpp"

namespace mylog{
    // 通过单例对象对日志器进行管理，懒汉模式
    class LoggerManager
    {
    public:
        static LoggerManager &GetInstance()
        {
            static LoggerManager eton;
            return eton;
        }

        // 该logger是否存在
        bool LoggerExist(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
                return false;
            return true;
        }

        // 添加logger
        void AddLogger(const AsyncLogger::ptr &&AsyncLogger)
        {
            if (LoggerExist(AsyncLogger->Name()))
                return;
            std::unique_lock<std::mutex> lock(mtx_);
            loggers_.insert(std::make_pair(AsyncLogger->Name(), AsyncLogger));
        }

        AsyncLogger::ptr GetLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
            {
                std::cerr << "Logger not found: " << name << std::endl;
                return AsyncLogger::ptr();
            }
            return it->second;
        }

        AsyncLogger::ptr DefaultLogger() { return default_logger_; }

    private:
        // logger有一个默认的日志器，默认日志器的名字为"default"
        LoggerManager()
        {
            std::unique_ptr<LoggerBuilder> builder(new LoggerBuilder());
            builder->BuildLoggerName("default");
            default_logger_ = builder->Build();
            loggers_.insert(std::make_pair("default", default_logger_));
        }

    private:
        std::mutex mtx_;
        AsyncLogger::ptr default_logger_;                              // 默认日志器
        std::unordered_map<std::string, AsyncLogger::ptr> loggers_; // 存放日志器
    };
}