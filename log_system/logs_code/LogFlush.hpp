#include <cassert>
#include <fstream>
#include <memory>
#include <unistd.h>
#include "Util.hpp"
#include <filesystem>

extern mylog::Util::JsonData* g_conf_data;
namespace mylog{
    class LogFlush
    {
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() {}
        virtual void Flush(const char *data, size_t len) = 0;//不同的写文件方式Flush的实现不同
    };

    // 标准输出
    class StdoutFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char *data, size_t len) override{
            cout.write(data, len);
        }
    };

    // 文件输出
    class FileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<FileFlush>;

        // 构造,打开文件
        FileFlush(const std::string &filename) : filename_(filename)
        {
            // 创建所给目录
            Util::File::CreateDirectory(Util::File::Path(filename));
            // 打开文件
            fs_ = fopen(filename.c_str(), "ab");
            if(fs_==NULL){
                std::cout <<__FILE__<<__LINE__<<"open log file failed"<< std::endl;
                perror(NULL);
            }
        }
        void Flush(const char *data, size_t len) override{
            // 每个数据项大小是1字节
            fwrite(data,1,len,fs_);
            // 检查文件流是否有错误
            if(ferror(fs_)){
                std::cout <<__FILE__<<__LINE__<<"write log file failed"<< std::endl;
                perror(NULL);
            }
            // 写入文件缓冲区
            if(g_conf_data->flush_log == 1){
                if(fflush(fs_)==EOF){
                    std::cout <<__FILE__<<__LINE__<<"fflush file failed"<< std::endl;
                    perror(NULL);
                }
            }else if(g_conf_data->flush_log == 2){
                // 强制写入硬盘
                if (fflush(fs_) == EOF){
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
                fsync(fileno(fs_));
            }
        }

    private:
        std::string filename_;
        FILE* fs_ = NULL; 
    };

    // 滚动文件输出
    class RollFileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<RollFileFlush>;
        RollFileFlush(const std::string &filename, size_t max_size, int retention_days = 7)
            : max_size_(max_size), basename_(filename), retention_days_(retention_days)
        {
            Util::File::CreateDirectory(Util::File::Path(filename));
            StartCleanupThread();
        }

        void Flush(const char *data, size_t len) override
        {
            // 确认文件大小不满足滚动需求
            InitLogFile();
            // 向文件写入内容
            fwrite(data, 1, len, fs_);
            if(ferror(fs_)){
                std::cout <<__FILE__<<__LINE__<<"write log file failed"<< std::endl;
                perror(NULL);
            }
            cur_size_ += len;
            if(g_conf_data->flush_log == 1){
                if(fflush(fs_)){
                    std::cout <<__FILE__<<__LINE__<<"fflush file failed"<< std::endl;
                    perror(NULL);
                }
            }else if(g_conf_data->flush_log == 2){
                if (fflush(fs_)){
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
                fsync(fileno(fs_));
            }
        }

    private:
        // 当前文件已经超过一定大小, 需要创建新的文件
        void InitLogFile()
        {
            if (fs_==NULL || cur_size_ >= max_size_)
            {
                if(fs_!=NULL){
                    fclose(fs_);
                    fs_=NULL;
                }   

                std::string filename = CreateFilename();

                // 打开新建文件
                fs_=fopen(filename.c_str(), "ab");
                if(fs_==NULL){
                    std::cout <<__FILE__<<__LINE__<<"open file failed"<< std::endl;
                    perror(NULL);
                }
                // 当前大小置0
                cur_size_ = 0;
            }
        }

        // 构建落地的滚动日志文件名称
        std::string CreateFilename()
        {
            time_t time_ = Util::Date::Now();
            struct tm t;
            localtime_r(&time_, &t);
            std::string filename = basename_;
            filename += std::to_string(t.tm_year + 1900);
            filename += std::to_string(t.tm_mon + 1);
            filename += std::to_string(t.tm_mday);
            filename += std::to_string(t.tm_hour + 1);
            filename += std::to_string(t.tm_min + 1);
            filename += std::to_string(t.tm_sec + 1) + '-' +
                        std::to_string(cnt_++) + ".log";
            return filename;
        }

        // 启动一个线程，定时清理过期日志
        void StartCleanupThread()
        {
            cleanup_thread_ = std::thread([this]{
                while (!stop_cleanup_) {
                    CleanupOldLogs();
                    std::this_thread::sleep_for(std::chrono::hours(24)); // Run daily
                } });
        }

        // 实际清理文件的函数
        void CleanupOldLogs()
        {
            namespace fs = std::filesystem;
            auto dir = fs::path(basename_).parent_path();
            if (!fs::exists(dir))
                return;

            time_t now = Util::Date::Now();
            time_t retention_time = now - retention_days_ * 24 * 60 * 60; // 保留天数转换为秒

            for (const auto &entry : fs::directory_iterator(dir))
            {
                // 普通文件且后缀为.log的文件
                if (entry.is_regular_file() && entry.path().extension() == ".log")
                {
                    auto last_write = fs::last_write_time(entry);
                    auto last_write_time = std::chrono::time_point_cast<std::chrono::seconds>(
                                               last_write - fs::file_time_type::clock::now() + std::chrono::system_clock::now())
                                               .time_since_epoch()
                                               .count();
                    if (last_write_time < retention_time)
                    {
                        fs::remove(entry.path());
                    }
                }
            }
        }

    private:
        size_t cnt_ = 1;
        size_t cur_size_ = 0;
        size_t max_size_;
        std::string basename_;
        // std::ofstream ofs_;
        FILE* fs_ = NULL;
        int retention_days_;
        std::atomic<bool> stop_cleanup_{false};
        std::thread cleanup_thread_;
    };

    // 工厂类，用于创建日志输出类
    // 这里使用了模板方法，传入不同的FlushType类型和参数，
    class LogFlushFactory
    {
    public:
        using ptr = std::shared_ptr<LogFlushFactory>;
        template <typename FlushType, typename... Args>
        static std::shared_ptr<LogFlush> CreateLog(Args &&...args)
        {
            return std::make_shared<FlushType>(std::forward<Args>(args)...);
        }
    };
} // namespace mylog