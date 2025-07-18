#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <jsoncpp/json/json.h>

#include <ctime>
#include <fstream>
#include <iostream>
using std::cout;
using std::endl;
namespace mylog
{
    namespace Util
    {
        // 时间类
        class Date
        {
        public:
            static time_t Now() { return time(nullptr); }
        };
        class File
        {
        public:
            static bool Exists(const std::string &filename)
            {
                // 所有文件元数据信息
                struct stat st;
                return (0 == stat(filename.c_str(), &st));
            }
            static std::string Path(const std::string &filename)
            {
                if (filename.empty())
                    return "";
                // 目录的最后一个文件
                int pos = filename.find_last_of("/\\");
                if (pos != std::string::npos)
                    return filename.substr(0, pos + 1);
                return "";
            }

            // 将路径创建到最后一级
            static void CreateDirectory(const std::string &pathname)
            {
                if (pathname.empty())
                    perror("文件所给路径为空：");
                // 文件不存在再创建
                if (!Exists(pathname)){
                    size_t pos, index = 0;
                    size_t size = pathname.size();
                    while (index < size){
                        // find_first_of是查找"/\\"中的任意一个字符
                        pos = pathname.find_first_of("/\\", index);
                        // 循环创建文件夹
                        if (pos == std::string::npos)
                        {
                            mkdir(pathname.c_str(), 0755);
                            return;
                        }
                        if (pos == index)
                        {
                            index = pos + 1;
                            continue;
                        }

                        std::string sub_path = pathname.substr(0, pos);
                        // 如果是"."或".."，则跳过
                        if (sub_path == "." || sub_path == "..")
                        {
                            index = pos + 1;
                            continue;
                        }
                        if (Exists(sub_path))
                        {
                            index = pos + 1;
                            continue;
                        }

                        mkdir(sub_path.c_str(), 0755);
                        index = pos + 1;
                    }
                }
            }

            // 取得文件大小,通过stat
            int64_t FileSize(std::string filename)
            {
                struct stat s;
                auto ret = stat(filename.c_str(), &s);
                if (ret == -1)
                {
                    perror("Get file size failed");
                    return -1;
                }
                return s.st_size;
            }
            // 获取文件内容
            bool GetContent(std::string *content, std::string filename)
            {
                // 打开文件
                std::ifstream ifs;
                ifs.open(filename.c_str(), std::ios::binary);
                if (ifs.is_open() == false)
                {
                    std::cout << "file open error" << std::endl;
                    return false;
                }

                // 读入content
                ifs.seekg(0, std::ios::beg); // 更改文件指针的偏移量
                size_t len = FileSize(filename);
                content->resize(len);
                ifs.read(&(*content)[0], len);
                if (!ifs.good())
                {
                    std::cout << __FILE__ << __LINE__ << "-"
                              << "read file content error" << std::endl;
                    ifs.close();
                    return false;
                }
                ifs.close();

                return true;
            }
        }; // class file

        class JsonUtil
        {
        public:
            // 将json对象转换成字符串
            static bool Serialize(const Json::Value &val, std::string *str)
            {
                // 建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入str
                Json::StreamWriterBuilder swb;
                // 默认配置的建造者实例
                // usw, StreamWriter是实际执行 JSON 序列化写入操作的类
                std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
                std::stringstream ss;
                // val是序列化对象, ss是序列化结果的输出流
                if (usw->write(val, &ss) != 0)
                {
                    std::cout << "serialize error" << std::endl;
                    return false;
                }
                *str = ss.str();
                return true;
            }
            // 将字符串转成json对象
            static bool UnSerialize(const std::string &str, Json::Value *val)
            {
                // 操作方法类似序列化
                // 构建CharReader的建造者
                Json::CharReaderBuilder crb;
                std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
                std::string err;
                if (ucr->parse(str.c_str(), str.c_str() + str.size(), val, &err) == false)
                {
                    std::cout <<__FILE__<<__LINE__<<"parse error" << err<<std::endl;
                    return false;
                }
                return true;
            }
        };
        // 构造的时候就已经获取到config的信息
        struct JsonData{
            static JsonData* GetJsonData(){
               static JsonData* json_data = new JsonData;
               return json_data;
            }
            private:
            JsonData(){
                std::string content;
                mylog::Util::File file;
                // 相对路径读取配置文件
                if (file.GetContent(&content, "../../log_system/logs_code/config.conf") == false){
                    std::cout << __FILE__ << __LINE__ << "open config.conf failed" << std::endl;
                    perror(NULL);
                }
                Json::Value root;
                mylog::Util::JsonUtil::UnSerialize(content, &root); // 反序列化，把内容转成jaon value格式
                buffer_size = root["buffer_size"].asInt64();
                threshold = root["threshold"].asInt64();
                linear_growth = root["linear_growth"].asInt64();
                flush_log = root["flush_log"].asInt64();
                backup_addr = root["backup_addr"].asString();
                backup_port = root["backup_port"].asInt();
                thread_count = root["thread_count"].asInt();
            }
            public:
                size_t buffer_size;//缓冲区基础容量
                size_t threshold;// 倍数扩容阈值
                size_t linear_growth;// 线性增长容量
                size_t flush_log;//控制日志同步到磁盘的时机，默认为0,1调用fflush，2调用fsync
                std::string backup_addr;
                uint16_t backup_port;
                size_t thread_count;
        };
    } // namespace Util
} // namespace mylog
