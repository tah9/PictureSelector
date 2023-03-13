#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <jni.h>

class fixed_thread_pool {
private:
    struct data {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool is_shutdown_ = false;
        std::queue<std::function<void()>> tasks_;
        short int unDoWorks = 0;
    };
    std::shared_ptr<data> data_;
public:
    explicit fixed_thread_pool(size_t thread_count)
            : data_(std::make_shared<data>()) {
        for (size_t i = 0; i < thread_count; ++i) {
            std::thread([=] {//原写法data=data_
                std::thread::id this_id = std::this_thread::get_id();
                unsigned int t = *(unsigned int *) &this_id;

//                std::cout << "create - thread-" << t << std::endl << std::flush;
                //初次上锁
                std::unique_lock<std::mutex> lk(data_->mtx_);
//                std::cout << "lock-thread-" << t << std::endl << std::flush;

                while (true) {
//                    std::cout << "run-thread-" << t << std::endl << std::flush;
                    if (!data_->tasks_.empty()) {
                        auto current = std::move(data_->tasks_.front());
                        data_->tasks_.pop();

                        lk.unlock();//解锁，队列为空

                        current();//执行，还没添加上去，此时队列为空，结束时会添加上去新的任务

                        lk.lock();//该次任务执行结束
                        data_->unDoWorks--;

                        if (data_->unDoWorks == 0) {
                            data_->cond_.notify_one();
                        }
                    } else if (data_->is_shutdown_) {
                        break;
                    } else {
//                        std::cout << "挂起thread-" << t << std::flush;
                        data_->cond_.wait(lk);
                    }
                }
            }).detach();
        }
    }

    fixed_thread_pool() = default;

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

    void waitFinish() {
        
        while(){

        }
        usleep(1000 * 1000);
//        std::unique_lock<std::mutex> ul(data_->mtx_);
//        std::cout<<"主线程阻塞"<<std::endl<<std::flush;
//        data_->cond_.wait(ul, [&] { return data_->unDoWorks == 0; });
//        std::cout<<"主线程结束阻塞"<<std::endl<<std::flush;
    }

    template<class F>
    void execute(F &&task) {
        {
            std::lock_guard<std::mutex> lk(data_->mtx_);
            data_->unDoWorks++;
            data_->tasks_.emplace(std::forward<F>(task));
//            LOGI("execute");
        }
        data_->cond_.notify_one();
//        LOGI("execute notify_one");
    }

};