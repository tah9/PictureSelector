#include <iostream>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(size_t threadCount)
            : m_stop(false), m_threads(threadCount)
    {
        for (size_t i = 0; i < threadCount; ++i) {
            m_threads[i] = std::thread([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                        if (m_stop && m_tasks.empty())
                            return;
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_condition.notify_all();
        for (auto& thread : m_threads) {
            thread.join();
        }
    }

    template<typename T>
    auto enqueue(T task) -> std::future<decltype(task())>
    {
        auto wrapper = std::make_shared<std::packaged_task<decltype(task()) ()>>(std::move(task));
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.emplace([wrapper] { (*wrapper)(); });
        }
        m_condition.notify_one();
        return wrapper->get_future();
    }

    void waitAll()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return m_tasks.empty(); });
    }

private:
    std::vector<std::thread> m_threads;
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;
};