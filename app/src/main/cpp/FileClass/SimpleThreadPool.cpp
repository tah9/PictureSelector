#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <unistd.h>
#include <future>
#include "../JNI_LOG.h"

class fixed_thread_pool {
private:
    struct data {
        std::mutex *mtx_;
        std::condition_variable cond_;
        std::condition_variable main_cond_;
        bool is_shutdown_ = false;
        std::queue<std::function<void()>> tasks_;
        short int unDoWorks = 0;
//        std::atomic<int> unDoWorks = {0};
    public:
        data() : mtx_(new std::mutex) {}

        ~data() {
            LOGI("~data");
        }
    };

public:
    std::shared_ptr<data> data_;
    //获取计算机支持的并发线程数量
    short int num_thread = (short int) std::thread::hardware_concurrency();

    explicit fixed_thread_pool() : data_(std::make_shared<data>()) {
        LOGI("num_thread %d", num_thread);
        for (size_t i = 0; i < num_thread; ++i) {
            std::thread([this] {

                std::unique_lock<std::mutex> lk(*data_->mtx_);
                while (true) {
                    if (!data_->tasks_.empty()) {
                        auto fun = std::move(data_->tasks_.front());
                        data_->tasks_.pop();
                        lk.unlock();

                        fun();

                        lk.lock();
                        if (--data_->unDoWorks == 0) {
                            data_->main_cond_.notify_one();
                        }
                    } else if (data_->is_shutdown_) {
                        break;
                    } else {
                        data_->cond_.wait(lk);
                    }
                }
                LOGI("线程关闭");
                if (--num_thread == 0) {
                    delete data_->mtx_;
                    LOGI("删除锁");
                }
            }).detach();
        }
    }

    fixed_thread_pool(fixed_thread_pool &&) = default;

    ~fixed_thread_pool() {
        {
            std::lock_guard<std::mutex> lk(*data_->mtx_);
            data_->is_shutdown_ = true;
        }
        data_->cond_.notify_all();
        LOGI("~fixed_thread_pool()");
    }

    void waitFinish() {
        //挂起主线程，等所有任务完成再回调。
        std::unique_lock<std::mutex> lk(*data_->mtx_);
        data_->main_cond_.wait(lk, [this] { return data_->unDoWorks == 0; });
    }

    template<class F, class... Args>
    auto execute(F &&fun, Args &&...args) -> std::future<decltype(fun(args...))> {
        std::function<decltype(fun(args...))()> b_fun = std::bind(std::forward<F>(fun),
                                                                  std::forward<Args>(args)...);
        auto share_task = std::make_shared<std::packaged_task<decltype(fun(args...))()>>(b_fun);

        {
            std::lock_guard<std::mutex> lk(*data_->mtx_);
            ++data_->unDoWorks;
            data_->tasks_.emplace([share_task] { (*share_task)(); });
        }
        data_->cond_.notify_one();

        return share_task->get_future();
    }
};