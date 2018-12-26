#ifndef ALOG_POSIX_LOGGER_H_
#define ALOG_POSIX_LOGGER_H_

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#ifdef OS_LINUX
#ifndef FALLOC_FL_KEEP_SIZE
#include <linux/falloc.h>
#endif
#endif

#include <string>
#include <atomic>

#include "logger.h"

class PosixLogger : public Logger {
 public:
  PosixLogger(FILE* f, const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : Logger(log_level),
        file_(f),
        fd_(fileno(f)),
        log_size_(0),
        last_flush_micros_(0),
        flush_pending_(false) {}

  virtual ~PosixLogger() { Close(); }

  virtual void Flush() override {
    if (flush_pending_) {
      flush_pending_ = false;
      fflush(file_);
    }
    last_flush_micros_ = NowMicros();
  }

  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    const uint64_t thread_id = GetThreadID();

    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char* base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 655235;
        base = new char[bufsize];
      }
      char* p = base;
      char* limit = base + bufsize;

      struct timeval now_tv;
      gettimeofday(&now_tv, nullptr);
      const time_t seconds = now_tv.tv_sec;
      struct tm t;
      localtime_r(&seconds, &t);
      p += snprintf(p, limit - p,
                    "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
                    t.tm_year + 1900,
                    t.tm_mon + 1,
                    t.tm_mday,
                    t.tm_hour,
                    t.tm_min,
                    t.tm_sec,
                    static_cast<int>(now_tv.tv_usec),
                    static_cast<long long unsigned int>(thread_id));

      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

      if (p >= limit) {
        if (iter == 0) {
          continue;
        } else {
          p = limit - 1;
        }
      }

      if (p == base || p[-1] != '\n') {
        *p++ = '\n';
      }

      assert(p <= limit);
      const size_t write_size = p - base;

#ifdef FALLOCATE_PRESENT
      static const int kDebugLogChunkSize = 128 * 1024;
      const size_t log_size = log_size_;
      const size_t last_allocation_chunk =
        ((kDebugLogChunkSize - 1 + log_size) / kDebugLogChunkSize);
      const size_t desired_allocation_chunk =
        ((kDebugLogChunkSize - 1 + log_size + write_size) /
         kDebugLogChunkSize);
      if (last_allocation_chunk != desired_allocation_chunk) {
        fallocate(fd_, FALLOC_FL_KEEP_SIZE, 0,
                  static_cast<off_t>(desired_allocation_chunk * kDebugLogChunkSize));
      }
#endif

      size_t sz = fwrite(base, 1, write_size, file_);
      flush_pending_ = true;
      assert(sz == write_size);
      if (sz > 0) {
        log_size_ += write_size;
      }
      uint64_t now_micros = static_cast<uint64_t>(now_tv.tv_sec) * 1000000 +
        now_tv.tv_usec;
      if (now_micros - last_flush_micros_ >= flush_every_seconds_ * 1000000) {
        Flush();
      }
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }

  size_t GetLogFileSize() const override { return log_size_; }

  static bool NewLogger(const std::string& fname,
                        const InfoLogLevel log_level,
                        std::shared_ptr<Logger>* result) {
    FILE* f = fopen(fname.c_str(), "w");
    if (f == nullptr) {
      result->reset();
      return false;
    } else {
      int fd = fileno(f);
#ifdef FALLOCATE_PRESENT
      fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, 4 * 1024);
#endif
      fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
      result->reset(new PosixLogger(f, log_level));
      return true;
    }
  }

 private:
  static uint64_t GetThreadID() {
    pthread_t tid = pthread_self();
    uint64_t thread_id = 0;
    memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
    return thread_id;
  }

  static uint64_t NowMicros() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
  }

  virtual bool CloseImpl() override {
    if (fclose(file_)) {
      return false;
    }

    return true;
  }

  static const uint64_t flush_every_seconds_ = 5;

  FILE* file_;
  int fd_;
  std::atomic_size_t log_size_;
  std::atomic_uint_fast64_t last_flush_micros_;
  std::atomic<bool> flush_pending_;
};


#endif // ALOG_POSIX_LOGGER_H_
