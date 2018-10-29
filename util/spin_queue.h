#ifndef MYUTIL_SPIN_QUEUE_H_
#define MYUTIL_SPIN_QUEUE_H_

#include <deque>
#include <mutex>
#include <utility>
#include <limits>

#include "queue.h"
#include "spin_lock.h"
#include "unsafe_queue.h"

namespace myutil {

template <typename ValueType>
class SpinQueue : public Queue<ValueType> {
 public:
  SpinQueue() = default;
  virtual ~SpinQueue() = default;

  SpinQueue(const SpinQueue&) = delete;
  SpinQueue& operator=(const SpinQueue&) = delete;
  SpinQueue(SpinQueue&&) = delete;
  SpinQueue& operator=(SpinQueue&&) = delete;

  virtual bool Push(const ValueType& value) override {
    std::lock_guard<SpinLock> guard(spin_lock_);
    queue_.push_back(value);
    return 1 == queue_.size();
  }

  virtual ValueType Pop() override {
    std::lock_guard<SpinLock> guard(spin_lock_);
    ValueType value = std::move(queue_.front());
    queue_.pop_front();
    return std::move(value);
  }

  virtual std::unique_ptr<Queue<ValueType>> BatchPop(
      size_t max = std::numeric_limits<size_t>::max()) override {
    auto result = make_unique<UnsafeQueue<ValueType>>();

    {
      std::lock_guard<SpinLock> guard(spin_lock_);
      if (queue_.size() <= max) {
        using std::swap;
        swap(result->queue_, queue_);
      } else {
        for (size_t i = 0; i < max && !queue_.empty(); i++) {
          result->queue_.push_back(std::move(queue_.front()));
          queue_.pop_front();
        }
      }
    }

    return std::move(result);
  }

  virtual size_t Size() override {
    std::lock_guard<SpinLock> guard(spin_lock_);
    return queue_.size();
  }

  virtual bool Empty() override {
    std::lock_guard<SpinLock> guard(spin_lock_);
    return queue_.empty();
  }

 private:
  SpinLock spin_lock_;
  std::deque<ValueType> queue_;
}; // class SpinQueue

template<typename ValueType>
SpinQueue<ValueType>* NewSpinQueue() {
  return new SpinQueue<ValueType>();
}

} // namespace myutil

#endif // MYUTIL_SPIN_QUEUE_H_
