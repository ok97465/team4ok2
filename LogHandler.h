#ifndef LogHandlerH
#define LogHandlerH

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <unordered_map>

class LogHandler {
public:
    enum LogLevel {
        LOG_DEBUG = 0,
        LOG_INFO = 1,
        LOG_WARNING = 2,
        LOG_ERROR = 3,
        LOG_CRITICAL = 4
    };
    
    enum LogCategory {
        CAT_GENERAL = 0x01,
        CAT_PURGE = 0x02,
        CAT_PROXIMITY = 0x04,
        CAT_SBS = 0x08,
        CAT_PERFORMANCE = 0x10,
        CAT_UI = 0x20,
        CAT_NETWORK = 0x40,
        CAT_MAP = 0x80
    };
    
private:
    static LogLevel s_minLevel;
    static int s_enabledCategories;
    static bool s_enableConsole;
    static bool s_enableFile;
    static std::ofstream s_logFile;
    static std::mutex s_logMutex;
    static bool s_initialized;
    
    // 조건부 로깅을 위한 카운터와 타이머
    static std::unordered_map<std::string, int> s_counters;
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> s_lastLogTime;
    
    static std::string GetLevelString(LogLevel level);
    static std::string GetCategoryString(LogCategory category);
    static std::string GetTimestamp();
    
public:
    // 초기화 및 설정
    static void Initialize();
    static void Shutdown();
    static void SetMinLevel(LogLevel level);
    static void EnableCategory(LogCategory categoryMask);
    static void DisableCategory(LogCategory categoryMask);
    static void SetConsoleOutput(bool enable);
    static void SetFileOutput(const std::string& filename);
    
    // 로깅 메서드
    static void Log(LogLevel level, LogCategory category, const std::string& message);
    static void LogFormat(LogLevel level, LogCategory category, const char* format, ...);
    
    // 조건부 로깅
    static bool ShouldLogEveryN(const std::string& key, int n);
    static bool ShouldLogWithInterval(const std::string& key, std::chrono::milliseconds interval);
    
    // 유틸리티
    static bool IsEnabled(LogLevel level, LogCategory category);
    static void FlushLogs();
};

// 편의 매크로 정의
#define LOG_DEBUG(cat, msg) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_DEBUG, cat)) { \
        LogHandler::Log(LogHandler::LOG_DEBUG, cat, msg); \
    } \
} while(0)

#define LOG_INFO(cat, msg) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_INFO, cat)) { \
        LogHandler::Log(LogHandler::LOG_INFO, cat, msg); \
    } \
} while(0)

#define LOG_WARNING(cat, msg) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_WARNING, cat)) { \
        LogHandler::Log(LogHandler::LOG_WARNING, cat, msg); \
    } \
} while(0)

#define LOG_ERROR(cat, msg) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_ERROR, cat)) { \
        LogHandler::Log(LogHandler::LOG_ERROR, cat, msg); \
    } \
} while(0)

#define LOG_CRITICAL(cat, msg) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_CRITICAL, cat)) { \
        LogHandler::Log(LogHandler::LOG_CRITICAL, cat, msg); \
    } \
} while(0)

// Printf 스타일 매크로
#define LOG_DEBUG_F(cat, fmt, ...) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_DEBUG, cat)) { \
        LogHandler::LogFormat(LogHandler::LOG_DEBUG, cat, fmt, __VA_ARGS__); \
    } \
} while(0)

#define LOG_INFO_F(cat, fmt, ...) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_INFO, cat)) { \
        LogHandler::LogFormat(LogHandler::LOG_INFO, cat, fmt, __VA_ARGS__); \
    } \
} while(0)

#define LOG_WARNING_F(cat, fmt, ...) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_WARNING, cat)) { \
        LogHandler::LogFormat(LogHandler::LOG_WARNING, cat, fmt, __VA_ARGS__); \
    } \
} while(0)

#define LOG_ERROR_F(cat, fmt, ...) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_ERROR, cat)) { \
        LogHandler::LogFormat(LogHandler::LOG_ERROR, cat, fmt, __VA_ARGS__); \
    } \
} while(0)

#define LOG_CRITICAL_F(cat, fmt, ...) do { \
    if (LogHandler::IsEnabled(LogHandler::LOG_CRITICAL, cat)) { \
        LogHandler::LogFormat(LogHandler::LOG_CRITICAL, cat, fmt, __VA_ARGS__); \
    } \
} while(0)

// 조건부 로깅 매크로
#define LOG_EVERY_N(level, cat, key, n, msg) do { \
    if (LogHandler::IsEnabled(level, cat) && LogHandler::ShouldLogEveryN(key, n)) { \
        LogHandler::Log(level, cat, msg); \
    } \
} while(0)

#define LOG_WITH_INTERVAL(level, cat, key, interval_ms, msg) do { \
    if (LogHandler::IsEnabled(level, cat) && LogHandler::ShouldLogWithInterval(key, std::chrono::milliseconds(interval_ms))) { \
        LogHandler::Log(level, cat, msg); \
    } \
} while(0)

// LogCategory에 대한 비트 연산자 지원 (클래스 외부에서 정의)
inline LogHandler::LogCategory operator|(LogHandler::LogCategory a, LogHandler::LogCategory b) {
    return static_cast<LogHandler::LogCategory>(static_cast<int>(a) | static_cast<int>(b));
}

inline LogHandler::LogCategory operator&(LogHandler::LogCategory a, LogHandler::LogCategory b) {
    return static_cast<LogHandler::LogCategory>(static_cast<int>(a) & static_cast<int>(b));
}

inline LogHandler::LogCategory& operator|=(LogHandler::LogCategory& a, LogHandler::LogCategory b) {
    a = static_cast<LogHandler::LogCategory>(static_cast<int>(a) | static_cast<int>(b));
    return a;
}

#endif
