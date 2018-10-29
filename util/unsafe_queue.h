#ifndef MYUTIL_UNSAFE_QUEUE_H_
#define MYUTIL_UNSAFE_QUEUE_H_

#include <deque>
#include <utility>
#include <limits>

#include "queue.h"
#include "make_unique.h"

namespace myutil {

template<typename ValueType>
class SpinQueue;

template <typename ValueType>
class UnsafeQueue : public Queue<ValueType> {
 public:
  UnsafeQueue() = default;
  virtual ~UnsafeQueue() = default;

  UnsafeQueue(const UnsafeQueue&) = default;
  UnsafeQueue& operator=(const UnsafeQueue&) = default;
  UnsafeQueue(UnsafeQueue&&) = default;
  UnsafeQueue& operator=(UnsafeQueue&&) = default;

  virtual bool Push(const ValueType& value) override {
    queue_.push_back(value);
    return 1 == queue_.size();
  }

  virtual ValueType Pop() override {
    ValueType value = std::move(queue_.front());
    queue_.pop_front();
    return std::move(value);
  }

  virtual std::unique_ptr<Queue<ValueType>> BatchPop(
      size_t max = std::numeric_limits<size_t>::max()) override {
    auto result = make_unique<UnsafeQueue<ValueType>>();

    if (queue_.size() <= max) {
      using std::swap;
      swap(result->queue_, queue_);
    } else {
      for (size_t i = 0; i < max && !queue_.empty(); i++) {
        result->queue_.push_back(std::move(queue_.front()));
        queue_.pop_front();
      }
    }

    return std::move(result);
  }

  virtual size_t Size() override {
    return queue_.size();
  }

  virtual bool Empty() override {
    return queue_.empty();
  }

 private:
  friend class SpinQueue<ValueType>;

  std::deque<ValueType> queue_;
}; // class UnsafeQueue

template <typename ValueType>
UnsafeQueue<ValueType>* NewUnsafeQueue() {
  return new UnsafeQueue<ValueType>();
}

} // namespace myutil

#endif // MYUTIL_UNSAFE_QUEUE_H_
