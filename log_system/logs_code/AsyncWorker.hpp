#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "AsyncBuffer.hpp"

namespace mylog {
    enum class AsyncType { ASYNC_SAFE, ASYNC_UNSAFE };

    // 接收Buffer作为参数的回调函数类型
    using functor = std::function<void(Buffer&)>;
    
    class AsyncWorker {
    public:
        using ptr = std::shared_ptr<AsyncWorker>;

        // 默认线程安全
        AsyncWorker(const functor& cb, AsyncType async_type = AsyncType::ASYNC_SAFE)
            : async_type_(async_type),
            callback_(cb),
            stop_(false),
            // 非静态成员函数, 所以需要传入this指针
            thread_(std::thread(&AsyncWorker::ThreadEntry, this)) {
                if (!callback_)
                {
                    std::cerr << "Callback is null in AsyncWorker constructor" << std::endl;
                    throw std::invalid_argument("Null callback");
                }
            }
        
        ~AsyncWorker() { Stop(); }
        
        void Push(const char* data, size_t len) {
            // 如果生产者队列不足以写下len长度数据，并且缓冲区是固定大小，那么阻塞
            std::unique_lock<std::mutex> lock(mtx_);

            // 阻塞到能够写入
            // 这里可能有len的长度隐患
            if (AsyncType::ASYNC_SAFE == async_type_){
                cond_productor_.wait(lock, [&]()
                    { return len <= buffer_productor_.WriteableSize(); });
            }
                

            buffer_productor_.Push(data, len);
            cond_consumer_.notify_one();
        }

        void Stop() {
            stop_ = true;
            cond_consumer_.notify_all();  // 所有线程把缓冲区内数据处理完就结束了
            thread_.join();
        }

    private:
        // 线程入口函数, 循环处理写入数据
        void ThreadEntry() {
            while (1) {
                {  // 缓冲区交换完就解锁，让productor继续写入数据
                    std::unique_lock<std::mutex> lock(mtx_);

                    if (buffer_productor_.IsEmpty() &&
                        stop_)  // 有数据则交换，无数据就阻塞
                        cond_consumer_.wait(lock, [&]() {
                            return stop_ || !buffer_productor_.IsEmpty();
                        });

                    buffer_productor_.Swap(buffer_consumer_);
                    // 固定容量的缓冲区才需要唤醒
                    if (async_type_ == AsyncType::ASYNC_SAFE)
                        cond_productor_.notify_one();
                }
                if (!callback_)
                {
                    std::cerr << "Error: callback_ is null" << std::endl;
                    return;
                }
                callback_(buffer_consumer_);  // 调用回调函数对缓冲区中数据进行处理
                buffer_consumer_.Reset();
                if (stop_ && buffer_productor_.IsEmpty()) return;
            }
        }

    private:
        AsyncType async_type_;
        std::atomic<bool> stop_;  // 用于控制异步工作器的启动
        std::mutex mtx_;
        mylog::Buffer buffer_productor_;
        mylog::Buffer buffer_consumer_;
        std::condition_variable cond_productor_;
        std::condition_variable cond_consumer_;
        std::thread thread_;

        functor callback_;  // 回调函数，用来告知工作器如何落地
    };
}  // namespace mylog