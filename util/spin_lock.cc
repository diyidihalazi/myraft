#include "spin_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace myutil {

static void PthreadCall(const char* lable, int result) {
  if (0 != result) {
    fprintf(stderr, "pthread %s: %s\n", lable, strerror(result));
    abort();
  }
}

SpinLock::SpinLock() {
  PthreadCall("init spin", pthread_spin_init(&spin_lock_, PTHREAD_PROCESS_PRIVATE));
}

SpinLock::~SpinLock() {
  PthreadCall("destory spin", pthread_spin_destroy(&spin_lock_));
}

void SpinLock::lock() {
  PthreadCall("spin lock", pthread_spin_lock(&spin_lock_));
}

void SpinLock::unlock() {
  PthreadCall("spin unlock", pthread_spin_unlock(&spin_lock_));
}

} // namespace myutil
