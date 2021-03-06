#include "util/file.hpp"
#include "util/logger.hpp"
#include "util/thread.hpp"

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <ostream>

namespace rubinius {
  namespace utilities {
    namespace logger {
      static thread::SpinLock lock_;
      static Logger* logger_ = 0;
      static logger_level loglevel_ = eWarn;

      void open(logger_type type, const char* identifier, logger_level level) {
        lock_.init();

        switch(type) {
        case eSyslog:
          logger_ = new Syslog(identifier);
          break;
        case eConsoleLogger:
          logger_ = new ConsoleLogger(identifier);
          break;
        case eFileLogger:
          logger_ = new FileLogger(identifier);
          break;
        }

        loglevel_ = level;
      }

      void close() {
        delete logger_;
      }

#define LOGGER_MSG_SIZE   1024

      static int append_newline(char* message) {
        char* end = (char*)memchr(message, '\0', LOGGER_MSG_SIZE);
        size_t bytes = end ? end - message : LOGGER_MSG_SIZE;

        if(bytes < LOGGER_MSG_SIZE) {
          if(message[bytes-1] != '\n') {
            message[bytes] = '\n';
            message[bytes += 1] = '\0';
          }
        } else {
          if(message[bytes-1] != '\n') message[bytes-1] = '\n';
        }

        return bytes;
      }

      void set_loglevel(logger_level level) {
        loglevel_ = level;
      }

      void write(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;

          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->write(buf, append_newline(buf));
        }
      }

      void fatal(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(loglevel_ < eFatal) return;

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;

          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->fatal(buf, append_newline(buf));
        }
      }

      void error(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(loglevel_ < eError) return;

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;
          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->error(buf, append_newline(buf));
        }
      }

      void warn(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(loglevel_ < eWarn) return;

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;
          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->warn(buf, append_newline(buf));
        }
      }

      void info(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(loglevel_ < eInfo) return;

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;
          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->info(buf, append_newline(buf));
        }
      }

      void debug(const char* message, ...) {
        thread::SpinLock::LockGuard guard(lock_);

        if(loglevel_ < eDebug) return;

        if(logger_) {
          char buf[LOGGER_MSG_SIZE];

          va_list args;
          va_start(args, message);
          (void) vsnprintf(buf, LOGGER_MSG_SIZE, message, args);
          va_end(args);

          logger_->debug(buf, append_newline(buf));
        }
      }

      char* Logger::timestamp() {
        time_t clock;

        time(&clock);
        strftime(formatted_time_, LOGGER_TIME_SIZE, "%b %e %H:%M:%S",
            localtime(&clock));
        return formatted_time_;
      }

      Syslog::Syslog(const char* identifier)
        : Logger()
      {
        openlog(identifier, LOG_CONS | LOG_PID, LOG_LOCAL7);

        int logmask = eWarn;

        switch(loglevel_) {
        case eFatal:
          logmask = LOG_EMERG;
          break;
        case eError:
          logmask = LOG_ERR;
          break;
        case eWarn:
          logmask = LOG_WARNING;
          break;
        case eInfo:
          logmask = LOG_NOTICE;
          break;
        case eDebug:
          logmask = LOG_DEBUG;
          break;
        }

        setlogmask(LOG_UPTO(logmask));
      }

      Syslog::~Syslog() {
        closelog();
      }

      // Syslog doesn't give us the ability to write a message to the log
      // independent of a priority. Bummer.
      void Syslog::write(const char* message, int size) {
        syslog(LOG_INFO, "%s", message);
      }

      void Syslog::fatal(const char* message, int size) {
        syslog(LOG_ERR, "%s", message);
        fprintf(stderr, "%s", message);
      }

      void Syslog::error(const char* message, int size) {
        syslog(LOG_ERR, "%s", message);
      }

      void Syslog::warn(const char* message, int size) {
        syslog(LOG_WARNING, "%s", message);
      }

      void Syslog::info(const char* message, int size) {
        syslog(LOG_INFO, "%s", message);
      }

      void Syslog::debug(const char* message, int size) {
        syslog(LOG_DEBUG, "%s", message);
      }

      ConsoleLogger::ConsoleLogger(const char* identifier)
        : Logger()
      {
        std::ostringstream str;
        str << identifier << "[" << getpid() << "]";

        identifier_ = new std::string(str.str());
      }

      ConsoleLogger::~ConsoleLogger() {
        delete identifier_;
      }

      void ConsoleLogger::write_log(const char* level, const char* message, int size) {
        fprintf(stderr, "%s %s %s %s", timestamp(), identifier_->c_str(), level, message);
      }

#define LOGGER_LEVEL_FATAL  "<Fatal>"
#define LOGGER_LEVEL_ERROR  "<Error>"
#define LOGGER_LEVEL_WARN   "<Warn>"
#define LOGGER_LEVEL_INFO   "<Info>"
#define LOGGER_LEVEL_DEBUG  "<Debug>"

      void ConsoleLogger::write(const char* message, int size) {
        fprintf(stderr, "%s %s %s", timestamp(), identifier_->c_str(), message);
      }

      void ConsoleLogger::fatal(const char* message, int size) {
        write_log(LOGGER_LEVEL_FATAL, message, size);
      }

      void ConsoleLogger::error(const char* message, int size) {
        write_log(LOGGER_LEVEL_ERROR, message, size);
      }

      void ConsoleLogger::warn(const char* message, int size) {
        write_log(LOGGER_LEVEL_WARN, message, size);
      }

      void ConsoleLogger::info(const char* message, int size) {
        write_log(LOGGER_LEVEL_INFO, message, size);
      }

      void ConsoleLogger::debug(const char* message, int size) {
        write_log(LOGGER_LEVEL_DEBUG, message, size);
      }

#define LOGGER_MAX_FILE     5242880
#define LOGGER_OPEN_FLAGS   (O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC)
#define LOGGER_OPEN_PERMS   0600

      FileLogger::FileLogger(const char* identifier)
        : Logger()
      {
        std::ostringstream str;
        str << " [" << getpid() << "] ";

        identifier_ = new std::string(str.str());

        logger_fd_ = ::open(identifier, LOGGER_OPEN_FLAGS, LOGGER_OPEN_PERMS);

        // Round robin if log file exceeds the limit
        if(lseek(logger_fd_, 0, SEEK_END) > LOGGER_MAX_FILE) {
          lseek(logger_fd_, 0, SEEK_SET);
        }
      }

      FileLogger::~FileLogger() {
        delete identifier_;
        ::close(logger_fd_);
      }

      void FileLogger::write_log(const char* level, const char* message, int size) {
        utilities::file::LockGuard guard(logger_fd_, LOCK_EX);

        const char* time = timestamp();
        write_status_ = ::write(logger_fd_, time, strlen(time));
        write_status_ = ::write(logger_fd_, identifier_->c_str(), identifier_->size());
        if(level) {
          write_status_ = ::write(logger_fd_, level, strlen(level));
          write_status_ = ::write(logger_fd_, " ", 1);
        }
        write_status_ = ::write(logger_fd_, message, size);
      }

      void FileLogger::write(const char* message, int size) {
        write_log(NULL, message, size);
      }

      void FileLogger::fatal(const char* message, int size) {
        write_log(LOGGER_LEVEL_FATAL, message, size);
        fprintf(stderr, "%s", message);
      }

      void FileLogger::error(const char* message, int size) {
        write_log(LOGGER_LEVEL_ERROR, message, size);
      }

      void FileLogger::warn(const char* message, int size) {
        write_log(LOGGER_LEVEL_WARN, message, size);
      }

      void FileLogger::info(const char* message, int size) {
        write_log(LOGGER_LEVEL_INFO, message, size);
      }

      void FileLogger::debug(const char* message, int size) {
        write_log(LOGGER_LEVEL_DEBUG, message, size);
      }
    }
  }
}
