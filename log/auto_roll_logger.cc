#include "auto_roll_logger.h"

#include "posix_logger.h"
#include "sql_logger.h"

Logger* AutoRollLogger::NewLogger(
    const std::string& log_dir,
    size_t max_log_file_size,
    size_t log_file_time_to_roll,
    uint64_t log_save_time,
    const InfoLogLevel log_level) {
  CreateDirIfMissing(log_dir);

  AutoRollLogger* result = new AutoRollLogger(
      log_dir, max_log_file_size, log_file_time_to_roll, log_save_time, log_level);
  bool status = result->GetStatus();
  if (!status) {
    delete result;
    result = NULL;
  }

  return result;
}

bool AutoRollLogger::ResetLogger() {
  if (GetInfoLogLevel() == InfoLogLevel::SQL_LEVEL) {
    status_ = SQLLogger::NewLogger(log_fname_, GetInfoLogLevel(), &logger_);
  } else {
    status_ = PosixLogger::NewLogger(log_fname_, GetInfoLogLevel(), &logger_);
  }

  if (!status_) {
    return status_;
  }

  cached_now_ = static_cast<uint64_t>(NowMicros() * 1e-6);
  ctime_ = cached_now_;
  cached_now_access_count_ = 0;

  return status_;
}

void AutoRollLogger::RollLogFile() {
  std::string old_fname = OldInfoLogFileName(ctime_, log_dir_);

  if (FileExists(old_fname)) {
    uint64_t index = 0;
    std::string temp_fname;
    do {
      index++;
      temp_fname = old_fname + "." + NumberToString(index);
    } while (FileExists(temp_fname));
    old_fname = temp_fname;
  }

  if (RenameFile(log_fname_, old_fname)) {
    log_files_.push_back(LogFile(ctime_, old_fname));
  }
}

void AutoRollLogger::Logv(const char* format, va_list ap) {
  std::shared_ptr<Logger> logger;
  {
    MutexLock l(&mutex_);
    if (!logger_ ||
        (log_file_time_to_roll_ > 0 && LogExpired()) ||
        (max_log_file_size_ > 0 && logger_->GetLogFileSize() >= max_log_file_size_)) {
      RollLogFile();
      ResetLogger();
      ClearOldLogFile();
    }

    logger = logger_;
  }

  if (logger) {
    logger->Logv(format, ap);
  }
}

bool AutoRollLogger::LogExpired() {
  if (cached_now_access_count_ >= call_nowmicros_every_n_records_) {
    cached_now_ = static_cast<uint64_t>(NowMicros() * 1e-6);
    cached_now_access_count_ = 0;
  }

  ++cached_now_access_count_;
  return cached_now_ >= ctime_ + log_file_time_to_roll_;
}


