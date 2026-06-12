#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
//    int num_threads;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

public:
    ThreadPool(int n) {
        for (int i = 0; i < n; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(mtx);
                    cv.wait(lock, [this] { return !tasks.empty() || stop; });
                    if (stop && tasks.empty()) return;

                    auto task = tasks.front();
                    tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    void submit(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push(task);
        cv.notify_one();
    }

    ~ThreadPool() {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
        lock.unlock();

        cv.notify_all();
        for (auto& w : workers) w.join();
    }
};
