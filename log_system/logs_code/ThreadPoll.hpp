#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threads)
        : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
        {
            workers.emplace_back([this]
                                 {
                for (;;) {
                    std::function<void()> task;

                    {
                        std::unique_lock lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty())
                            return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                } });
        }
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        // future对象, 如果get会阻塞,直到获取结果
        std::future<return_type> res = task->get_future();
        {
            // 将任务放进队列
            std::unique_lock lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // 函数指针解引用然后再调用
            tasks.emplace([task]
                          { (*task)(); });
        }
        // 唤醒一个线程
        condition.notify_one();
        return res;
    }

    ~ThreadPool()
    {
        {
            std::unique_lock lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto &worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
