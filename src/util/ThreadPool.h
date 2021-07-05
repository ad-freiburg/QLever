//
// Created by johannes on 28.06.21.
//

#ifndef QLEVER_THREADPOOL_H
#define QLEVER_THREADPOOL_H

#include <thread>

class ThreadPool {
 public:
  template <typename Function>
  ThreadPool(size_t numThreads, Function function)
      : _threads(numThreads, {function}) {}

 private:
  std::vector<std::jthread> _threads;
};

#endif  // QLEVER_THREADPOOL_H
