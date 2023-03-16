#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <unistd.h>
#include <future>

class fixed_thread_pool {
private:
    struct data {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool is_shutdown_ = false;
        std::queue<std::function<void()>> tasks_;
    };
    std::shared_ptr<data> data_;
public:
    //获取计算机支持的并发线程数量
    short int num_thread = (short int) std::thread::hardware_concurrency();

    explicit fixed_thread_pool()
            : data_(std::make_shared<data>()) {
        for (size_t i = 0; i < num_thread; ++i) {
            std::thread([this] {

                std::unique_lock<std::mutex> lk(data_->mtx_);
                while (true) {
                    if (!data_->tasks_.empty()) {
                        auto fun = std::move(data_->tasks_.front());
                        data_->tasks_.pop();
                        lk.unlock();

                        fun();

                        lk.lock();
                    } else if (data_->is_shutdown_) {
                        break;
                    } else {
                        data_->cond_.wait(lk);
                    }
                }

            }).detach();
        }
    }

    fixed_thread_pool(fixed_thread_pool &&) = default;

    ~fixed_thread_pool() {
        if ((bool) data_) {
            {
                std::lock_guard<std::mutex> lk(data_->mtx_);
                data_->is_shutdown_ = true;
            }
            data_->cond_.notify_all();
        }
        printf("~fixed_thread_pool()");
    }


    template<class F, class... Args>
    auto execute(F &&fun, Args &&...args) -> std::future<decltype(fun(args...))> {
        std::unique_lock<std::mutex> lk(data_->mtx_);
        std::function<decltype(fun(args...))()> b_fun = std::bind(std::forward<F>(fun),
                                                                  std::forward<Args>(args)...);
        auto share_task = std::make_shared<std::packaged_task<decltype(fun(args...))()>>(b_fun);
        data_->tasks_.emplace([share_task] { (*share_task)(); });
        lk.unlock();

        data_->cond_.notify_one();
        return share_task->get_future();
    }
};