#include <iostream>
#include <queue>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <functional>

using namespace std;

// 定义任务结构体
struct task_t {
    void (*fun)(void *);//函数
    void *arg; // 参数
};

// 线程池类
class ThreadPool {
private:
    int m_thread_num; // 线程池中的线程数量
    int m_task_num; // 任务队列的最大容量
    pthread_t *m_threads; // 线程数组
    queue<task_t> m_task_queue; // 任务队列
    bool m_shutdown; // 线程池是否关闭
    int m_task_count; // 当前未完成的任务数

    mutex m_mutex; // 互斥锁
    condition_variable m_cond; // 条件变量
    condition_variable m_cond_wait; // 等待条件变量

    static void *thread_func(void *arg) {
        ThreadPool *pool = (ThreadPool *) arg;

        while (true) {
            unique_lock<mutex> lock(pool->m_mutex);
            while (pool->m_task_queue.empty() && !pool->m_shutdown) {
                pool->m_cond.wait(lock);
            }

            if (pool->m_shutdown) {
                break;
            }

            task_t task = pool->m_task_queue.front();
            pool->m_task_queue.pop();
            pool->m_task_count--;
            lock.unlock();

            // 处理任务
            process_task(task);

            lock.lock();
            pool->m_cond_wait.notify_one();
        }

        return NULL;
    }

    static void process_task(task_t task) {
        auto fun = task.fun;
        fun(task.arg);
    }

public:
    ThreadPool(int thread_num = 5, int task_num = 10) : m_thread_num(thread_num), m_task_num(task_num),
                                                        m_shutdown(false), m_task_count(0) {
        m_threads = new pthread_t[m_thread_num];
    }

    ~ThreadPool() {
        delete[]m_threads;
    }

    void start() {
        for (int i = 0; i < m_thread_num; ++i) {
            pthread_create(&m_threads[i], NULL, thread_func, this);
        }
    }

    void waitStop() {
        {
            unique_lock<mutex> lock(m_mutex);
            m_shutdown = true;
            m_cond.notify_all();
        }

        for (int i = 0; i < m_thread_num; ++i) {
            pthread_join(m_threads[i], NULL);
        }
    }

    void add_task(task_t task) {
        unique_lock<mutex> lock(m_mutex);
        while (m_task_queue.size() >= m_task_num) {
            m_cond_wait.wait(lock);
        }

        m_task_queue.push(task);
        m_task_count++;
        m_cond.notify_one();
    }

    int get_task_count() const {
        return m_task_count;
    }


};