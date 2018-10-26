#ifndef MYUTIL_SPIN_LOCK_H_
#define MYUTIL_SPIN_LOCK_H_

#include <pthread.h>

namespace myutil {

class SpinLock {
 public:
  SpinLock();
  ~SpinLock();

  SpinLock(const SpinLock&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;
  SpinLock(SpinLock&&) = delete;
  SpinLock& operator=(SpinLock&&) = delete;

  void lock();
  void unlock();

 private:
  pthread_spinlock_t spin_lock_;
}; // class SpinLock

} // namespace myutil

#endif // MYUTIL_SPIN_LOCK_H_
