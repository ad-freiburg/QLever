// Copyright 2021, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Kalmbach <johannes.kalmbach@gmail.com>

#ifndef QLEVER_WORKQUEUE_H
#define QLEVER_WORKQUEUE_H

#include <deque>
#include <condition_variable>
#include <type_traits>
#include <queue>
#include "Synchronized.h"

namespace ad_utility {

inline auto constantZeroDummy = [](auto&&...) {return 0ull;};
template <typename T, bool isOrdered = false, typename GetIndex = decltype(constantZeroDummy)>
class WorkQueue {
 private:
  static constexpr size_t unlimited = size_t(-2);

  // we need the biggest index first, because priority_queu is a MAX heap
  constexpr static auto comparator = [](const auto& a, const auto& b) {
    return GetIndex{}(a) > GetIndex{}(b);
  };

  using Container = std::conditional_t<isOrdered, std::priority_queue<T, std::vector<T>, decltype(comparator)>, std::deque<T>>;
  Container _deque;
  std::mutex _mutex;
  std::condition_variable _conditionVariableMayPush;
  std::condition_variable _conditionVariableMayPop;
  size_t _maxSize;
  size_t _numberOfElementsPopped = 0;
  bool _isFinished = false;

  void pushInternal(T value) {
    if constexpr (isOrdered) {
      _deque.push(std::move(value));
    } else {
      _deque.push_back(std::move(value));
    }
  }

  decltype(auto) getFirst() {
    if constexpr (isOrdered) {
      return _deque.top();
    } else {
      return _deque.front();
    }
  }

  void removeFirst() {
    if constexpr (isOrdered) {
      _deque.pop();
    } else {
      _deque.pop_front();
    }
  }


 public:

  WorkQueue(size_t maxSize = unlimited) : _maxSize{maxSize} {}

  void push(T value) {
    std::unique_lock lock{_mutex};
    AD_CHECK(!_isFinished);
    _conditionVariableMayPush.wait(lock,
                                   [&] { return _deque.size() < _maxSize; });
    pushInternal(std::move(value));
    lock.unlock();
    _conditionVariableMayPop.notify_one();
  }

  /// std::nullopt signals, that the queue has been finished
  std::optional<T> pop() {
    std::unique_lock lock{_mutex};
    if constexpr (isOrdered) {
      _conditionVariableMayPop.wait(
          lock, [&] { return (!_deque.empty() && GetIndex{}(_deque.top()) == _numberOfElementsPopped) || _isFinished; });

    } else {
      _conditionVariableMayPop.wait(
          lock, [&] { return !_deque.empty() || _isFinished; });
    }
    if (_deque.empty()) {
      return std::nullopt;
    }

    if constexpr (isOrdered) {
      AD_CHECK(GetIndex{}(_deque.top()) == _numberOfElementsPopped);
    }
    std::optional<T> result{std::move(getFirst())};
    removeFirst();
    _numberOfElementsPopped++;
    lock.unlock();
    _conditionVariableMayPush.notify_one();
    return result;
  }

  /// After calling this function, no more calls to push are allowed
  void finish() {
    std::unique_lock lock{_mutex};
    _isFinished = true;
    lock.unlock();
    _conditionVariableMayPop.notify_one();
  }
};
} // namespace ad_utility

#endif  // QLEVER_WORKQUEUE_H
