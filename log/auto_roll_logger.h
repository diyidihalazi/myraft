#ifndef ALOG_AUTO_ROLL_LOGGER_H_
#define ALOG_AUTO_ROLL_LOGGER_H_

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <deque>
#include <memory>

#include "logger.h"
#include "mutexlock.h"

class AutoRollLogger : public Logger {
 public:
  AutoRollLogger(const std::string& log_dir,
                 size_t max_log_file_size,
                 size_t log_file_time_to_roll,
                 uint64_t log_save_time,
                 const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : Logger(log_level),
        log_dir_(log_dir),
        status_(true),
        max_log_file_size_(max_log_file_size),
        log_file_time_to_roll_(log_file_time_to_roll),
        cached_now_(static_cast<uint64_t>(NowMicros() * 1e-6)),
        ctime_(cached_now_),
        cached_now_access_count_(0),
        call_nowmicros_every_n_records_(100),
        log_save_time_(log_save_time),
        mutex_() {
    log_fname_ = InfoLogFileName(log_dir_);
    RollLogFile();
    ResetLogger();
  }

  virtual ~AutoRollLogger() {}

  using Logger::Logv;
  void Logv(const char* format, va_list ap) override;

  bool GetStatus() { return status_; }

  virtual size_t GetLogFileSize() const override {
    std::shared_ptr<Logger> logger;
    {
      MutexLock l(&mutex_);
      logger = logger_;
    }

    return logger->GetLogFileSize();
  }

  virtual void Flush() override {
    std::shared_ptr<Logger> logger;
    {
      MutexLock l(&mutex_);
      logger = logger_;
    }
    logger->Flush();
  }

  void SetCallNowMicrosEveryNRecords(uint64_t call_nowmicros_every_n_records) {
    call_nowmicros_every_n_records_ = call_nowmicros_every_n_records;
  }

  std::string LogFileName() const { return log_fname_; }

  uint64_t CTime() const { return ctime_; }

  //max_log_file_size (B)
  //log_file_time_to_roll (s)
  //log_save_time (s)
  static Logger* NewLogger(const std::string& log_dir,
                           size_t max_log_file_size,
                           size_t log_file_time_to_roll,
                           uint64_t log_save_time,
                           const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL);

 private:
  void ClearOldLogFile() {
    while (!log_files_.empty() && cached_now_ - log_files_.front().create_time_s_ > log_save_time_) {
      unlink(log_files_.front().fname_.c_str());
      log_files_.pop_front();
    }
  }

  static bool DirExists(const std::string& dname) {
    struct stat statbuf;
    if (stat(dname.c_str(), &statbuf) == 0) {
      return S_ISDIR(statbuf.st_mode);
    }

    return false;
  }

  static bool CreateDirIfMissing(const std::string& name) {
    if (mkdir(name.c_str(), 0755) != 0) {
      if (errno != EEXIST) {
        return false;
      } else if (!DirExists(name)) {
        return false;
      }
    }

    return true;
  }

  static uint64_t NowMicros() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
  }

  static bool FileExists(const std::string& fname) {
    if (access(fname.c_str(), F_OK) == 0) {
      return true;
    } else {
      return false;
    }
  }

  static bool RenameFile(const std::string& src,
                         const std::string& target) {
    if (rename(src.c_str(), target.c_str()) != 0) {
      return false;
    } else {
      return true;
    }
  }

  static std::string InfoLogFileName(const std::string& log_dir) {
    return log_dir + "/LOG";
  }

  static std::string OldInfoLogFileName(uint64_t unix_time, const std::string& log_dir) {
    char buf[50] = {0};
    struct tm t;
    const time_t seconds = static_cast<time_t>(unix_time);
    localtime_r(&seconds, &t);
    snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d-%02d:%02d",
             t.tm_year + 1900,
             t.tm_mon + 1,
             t.tm_mday,
             t.tm_hour,
             t.tm_min);

    return log_dir + "/LOG.old." + buf;
  }

  static std::string NumberToString(uint64_t num) {
    char buf[50] = {0};
    snprintf(buf, sizeof(buf) - 1, "%llu", static_cast<unsigned long long>(num));

    return buf;
  }

  bool LogExpired();
  bool ResetLogger();
  void RollLogFile();

  virtual bool CloseImpl() override {
    if (logger_) {
      return logger_->Close();
    } else {
      return true;
    }
  }

  std::string log_fname_;
  std::string log_dir_;
  std::shared_ptr<Logger> logger_;
  bool status_;

  size_t max_log_file_size_;
  size_t log_file_time_to_roll_;

  uint64_t cached_now_;
  uint64_t ctime_;
  uint64_t cached_now_access_count_;
  uint64_t call_nowmicros_every_n_records_;

  struct LogFile {
    uint64_t create_time_s_;
    std::string fname_;

    LogFile(time_t create_time_s, const std::string& fname)
        : create_time_s_(create_time_s),
          fname_(fname) {}
  };

  uint64_t log_save_time_;
  std::deque<LogFile> log_files_;

  mutable Mutex mutex_;
};



#endif // ALOG_AUTO_ROLL_LOGGER_H_
