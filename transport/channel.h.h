#ifndef MYUTIL_CHANNEL_H_
#define MYUTIL_CHANNEL_H_

#include <unistd.h>

#include <memory>
#include <utility>
#include <limits>

#include "queue.h"
#include "spin_queue.h"

namespace myutil {

template <typename ValueType>
class Channel {
 public:
  Channel(int readfd, int writefd)
      : readfd_(readfd),
        writefd_(writefd) {}

  ~Channel() {
    close(writefd_);
    close(readfd_);
  }

  Channel(const Channel&)            = delete;
  Channel& operator=(const Channel&) = delete;
  Channel(Channel&&)                 = delete;
  Channel& operator=(Channel&&)      = delete;

  int ReadFD() { return readfd_; }

  std::unique_ptr<Queue<ValueType>> Read(
      size_t max = std::numeric_limits<size_t>::max()) {
    std::unique_ptr<Queue<ValueType>> queue = queue_.BatchPop(max);

    char buf[1024];
    size_t remain = queue->Size();
    while (remain > 0) {
      int count = read(readfd_, buf, remain < 1024 ? remain : 1024);
      if (-1 == count) {
        if (EINTR != errno) {
          //LOGFATAL();
        }
      } else {
        remain -= count;
      }
    }

    return std::move(queue);
  }

  void Write(const ValueType& value) {
    queue_.Push(value);
    while (1 != (int ret = write(writefd_, " ", 1))) {
      if (-1 == ret) {
        if (EINTR != errno) {
          //LOGFATAL
        }
      }
    }
  }

  void Write(ValueType&& value) {
    queue_.Push(std::move(value));
    while (1 != (int ret = write(writefd_, " ", 1))) {
      if (-1 == ret) {
        if (EINTR != errno) {
          //LOGFATAL
        }
      }
    }
  }

 private:
  int readfd_;
  int writefd_;
  SpinQueue<ValueType> queue_;
}; // class Channel

template <typename ValueType>
std::unique_ptr<Channel<ValueType>> MakeChannel() {
  int pipefd[2];
  if (0 != pipe(pipefd)) {
    return nullptr;
  }

  return make_unique<Channel<ValueType>>(pipefd[0], pipefd[1]);
}

} // namespace myutil

#endif // MYUTIL_CHANNEL_H_
