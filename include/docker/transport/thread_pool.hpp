#ifndef DOCKER_TRANSPORT_THREAD_POOL_HPP
#define DOCKER_TRANSPORT_THREAD_POOL_HPP

#include "transport.hpp"
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace docker::transport {

class ThreadPool {
public:
  explicit ThreadPool(std::size_t thread_count, ITransport::Factory factory)
      : factory_(std::move(factory)), running_(true) {
    for (std::size_t i = 0; i < thread_count; i++) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      running_ = false;
    }
    cv_.notify_all(); // wake all workers so they can exit
    for (auto &t : workers_)
      t.join();
  }

  // no copy, no move
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // submit a task, get back a future for its result
  std::future<void> submit(std::function<void(ITransport &)> task) {
    auto promise = std::make_shared<std::promise<void>>();
    std::future<void> future = promise->get_future();

    {
      std::lock_guard<std::mutex> lock(mtx_);
      if (!running_)
        throw std::runtime_error("ThreadPool is shutting down");

      // wrap task + promise in a void() closure
      tasks_.push([promise, task](ITransport &sock) {
        try {
          task(sock);
          promise->set_value();
        } catch (...) {
          promise->set_exception(std::current_exception());
        }
      });
    }
    cv_.notify_one();
    return future;
  }

private:
  std::string socket_path_;
  std::vector<std::thread> workers_;
  std::queue<std::function<void(ITransport &)>> tasks_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool running_;

  ITransport::Factory factory_;

  void worker_loop() {
    // each wotker creates its own transport via the factory

    auto transport = factory_();

    while (true) {
      std::function<void(ITransport &)> task;
      {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });

        if (!running_ && tasks_.empty())
          return; // drain queue before exiting

        task = std::move(tasks_.front());
        tasks_.pop();
      } // unlock before exiting

      task(*transport);
    }
  }
}; // class ThreadPool
} // namespace docker::transport

#endif
