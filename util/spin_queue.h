#ifndef MYUTIL_SPIN_QUEUE_H_
#define MYUTIL_SPIN_QUEUE_H_

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
    queue_.pop();
    return std::move(value);
  }

  virtual std::unique_ptr<Queue<ValueType>> BatchPop(
      size_t max = std::numeric_limits<size_t>::max()) override {

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
