#include "logger.h"

Logger::~Logger() { Close(); }

bool Logger::Close() {
  if (!closed_) {
    closed_ = true;
    return CloseImpl();
  }

  return true;
}

bool Logger::CloseImpl() { return true; }

void Logger::Logv(const InfoLogLevel log_level, const char* format, va_list ap) {
  static const char* kInfoLogLevelNames[5] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  if (log_level < log_level_) {
    return;
  }

  if (log_level != InfoLogLevel::SQL_LEVEL) {
    char new_format[500];
    snprintf(new_format, sizeof(new_format) - 1, "[%s] %s",
        kInfoLogLevelNames[log_level], format);
    Logv(new_format, ap);
  } else {
    Logv(format, ap);
  }
}
