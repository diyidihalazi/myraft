#ifndef ALOG_LOGGER_H_
#define ALOG_LOGGER_H_

#include <stdio.h>
#include <stdarg.h>

#include <cstddef>
#include <limits>


enum InfoLogLevel : unsigned char {
  DEBUG_LEVEL = 0,
  INFO_LEVEL,
  WARN_LEVEL,
  ERROR_LEVEL,
  FATAL_LEVEL,
  HEADER_LEVEL,
  SQL_LEVEL,
  NUM_INFO_LOG_LEVELS
};

class Logger {
 public:
  explicit Logger(const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : closed_(false), log_level_(log_level) {}

  virtual ~Logger();

  virtual bool Close();

  virtual void LogHeader(const char* format, va_list ap) {
    Logv(format, ap);
  }

  virtual void Logv(const char* fromat, va_list ap) = 0;
  virtual void Logv(const InfoLogLevel log_level, const char* format, va_list ap);

  virtual size_t GetLogFileSize() const {
    static const size_t kDoNotSupportGetLogFileSize = (std::numeric_limits<size_t>::max)();
    return kDoNotSupportGetLogFileSize;
  }

  virtual void Flush() {}

  virtual InfoLogLevel GetInfoLogLevel() const { return log_level_; }
  virtual void SetInfoLogLevel(const InfoLogLevel log_level) { log_level_ = log_level; }

 private:
  Logger(const Logger&);
  void operator=(const Logger&);

  virtual bool CloseImpl();

  bool closed_;
  InfoLogLevel log_level_;
};


#endif // ALOG_LOGGER_H_
