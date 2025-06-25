/*日志缓冲区类设计*/
#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "Util.hpp"

extern mylog::Util::JsonData* g_conf_data;

namespace mylog{
    class Buffer{
    public:
        Buffer() : write_pos_(0), read_pos_(0) {
            buffer_.resize(g_conf_data->buffer_size);
        }

        void Push(const char *data, size_t len)
        {
            ToBeEnough(len); // 确保容量足够
            // 开始写入
            std::copy(data, data + len, &buffer_[write_pos_]);
            write_pos_ += len;
        }
        char *ReadBegin(int len)
        {
            // 读的时候需要保证读的长度不超过可读空间, assert可能需要更改
            assert(len <= ReadableSize());
            return &buffer_[read_pos_];
        }
        bool IsEmpty() { return write_pos_ == read_pos_; }

        // 换成传入的buf
        void Swap(Buffer &buf)
        {
            buffer_.swap(buf.buffer_);
            std::swap(read_pos_, buf.read_pos_);
            std::swap(write_pos_, buf.write_pos_);
        }
        size_t WriteableSize()
        { // 写空间剩余容量
            return buffer_.size() - write_pos_;
        }
        size_t ReadableSize()
        { // 读空间剩余容量
            return write_pos_ - read_pos_;
        }
        // 读指针的首地址
        const char *Begin() { return &buffer_[read_pos_]; }



        void MoveWritePos(int len)
        {
            assert(len <= WriteableSize());
            write_pos_ += len;
        }
        void MoveReadPos(int len)
        {
            assert(len <= ReadableSize());
            read_pos_ += len;
        }

        
        void Reset()
        { // 重置缓冲区
            write_pos_ = 0;
            read_pos_ = 0;
        }

    protected:
        // 缓冲区是否足够
        void ToBeEnough(size_t len)
        {
            int buffersize = buffer_.size();
            if (len >= WriteableSize())
            {
                if (buffer_.size() < g_conf_data->threshold)
                {
                    buffer_.resize(2 * buffer_.size() + buffersize);
                }
                else
                {
                    buffer_.resize(g_conf_data->linear_growth + buffersize);
                }
            }
        }

    protected:
        std::vector<char> buffer_; // 缓冲区
        size_t write_pos_;         // 生产者此时的位置
        size_t read_pos_;          // 消费者此时的位置
    };
} // namespace mylog