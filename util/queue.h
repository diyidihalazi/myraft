#ifndef MYUTIL_QUEUE_H_
#define MYUTIL_QUEUE_H_

namespace myutil {

template <typename ValueType>
class Queue {
 public:
  Queue() = default;
  virtual ~Queue() = default;

  Queue(const Queue&) = default;
  Queue& operator=(const Queue&) = default;
  Queue(Queue&&) = default;
  Queue& operator=(Queue&&) = default;

  // return true iff the queue is empty before the element is pushed back.
  virtual bool Push(const ValueType& value) = 0;
  virtual ValueType Pop() = 0;
  virtual std::unique_ptr<Queue> BatchPop(size_t max = std::numeric_limits<size_t>::max()) = 0;
}; // class Queue

} // namespace myutil

#endif // MYUTIL_QUEUE_H_
