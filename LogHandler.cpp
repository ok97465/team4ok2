#pragma hdrstop
#include "LogHandler.h"
#include <iostream>
#include <cstdarg>
#include <iomanip>
#include <sstream>

// Static 멤버 변수 초기화
LogHandler::LogLevel LogHandler::s_minLevel = LogHandler::LOG_INFO;
int LogHandler::s_enabledCategories = LogHandler::CAT_GENERAL;
bool LogHandler::s_enableConsole = true;
bool LogHandler::s_enableFile = false;
std::ofstream LogHandler::s_logFile;
std::mutex LogHandler::s_logMutex;
bool LogHandler::s_initialized = false;
std::unordered_map<std::string, int> LogHandler::s_counters;
std::unordered_map<std::string, std::chrono::steady_clock::time_point> LogHandler::s_lastLogTime;

void LogHandler::Initialize()
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    if (!s_initialized) {
        s_initialized = true;
        // 기본 설정: INFO 레벨, GENERAL 카테고리만 활성화, 콘솔 출력
        s_minLevel = LOG_INFO;
        s_enabledCategories = CAT_GENERAL;
        s_enableConsole = true;
        s_enableFile = false;
    }
}

void LogHandler::Shutdown()
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    if (s_logFile.is_open()) {
        s_logFile.close();
    }
    s_initialized = false;
}

void LogHandler::SetMinLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_minLevel = level;
}

void LogHandler::EnableCategory(LogCategory categoryMask)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_enabledCategories |= static_cast<int>(categoryMask);
}

void LogHandler::DisableCategory(LogCategory categoryMask)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_enabledCategories &= ~static_cast<int>(categoryMask);
}

void LogHandler::SetConsoleOutput(bool enable)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_enableConsole = enable;
}

void LogHandler::SetFileOutput(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    if (s_logFile.is_open()) {
        s_logFile.close();
    }
    
    if (!filename.empty()) {
        s_logFile.open(filename, std::ios::app);
        s_enableFile = s_logFile.is_open();
    } else {
        s_enableFile = false;
    }
}

std::string LogHandler::GetLevelString(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string LogHandler::GetCategoryString(LogCategory category)
{
    switch (category) {
        case CAT_GENERAL: return "GENERAL";
        case CAT_PURGE: return "PURGE";
        case CAT_PROXIMITY: return "PROXIMITY";
        case CAT_SBS: return "SBS";
        case CAT_PERFORMANCE: return "PERF";
        case CAT_UI: return "UI";
        case CAT_NETWORK: return "NETWORK";
        case CAT_MAP: return "MAP";
        default: return "UNKNOWN";
    }
}

std::string LogHandler::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void LogHandler::Log(LogLevel level, LogCategory category, const std::string& message)
{
    if (!s_initialized) {
        Initialize();
    }
    
    // 빠른 체크 (락 없이)
    if (level < s_minLevel || !(s_enabledCategories & category)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(s_logMutex);
    
    // 락 내에서 다시 체크 (thread-safe)
    if (level < s_minLevel || !(s_enabledCategories & category)) {
        return;
    }
    
    std::string timestamp = GetTimestamp();
    std::string levelStr = GetLevelString(level);
    std::string categoryStr = GetCategoryString(category);
    
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] [" + categoryStr + "] " + message;
    
    // 콘솔 출력
    if (s_enableConsole) {
        std::cout << formattedMessage << std::endl;
    }
    
    // 파일 출력
    if (s_enableFile && s_logFile.is_open()) {
        s_logFile << formattedMessage << std::endl;
        s_logFile.flush(); // 즉시 쓰기 (성능 vs 안정성 트레이드오프)
    }
}

void LogHandler::LogFormat(LogLevel level, LogCategory category, const char* format, ...)
{
    if (!s_initialized) {
        Initialize();
    }
    
    // 빠른 체크 (락 없이)
    if (level < s_minLevel || !(s_enabledCategories & category)) {
        return;
    }
    
    // 가변인수 처리
    va_list args;
    va_start(args, format);
    
    // 버퍼 크기 계산
    int size = vsnprintf(nullptr, 0, format, args) + 1;
    va_end(args);
    
    if (size <= 0) return;
    
    // 메시지 포맷팅
    std::vector<char> buffer(size);
    va_start(args, format);
    vsnprintf(buffer.data(), size, format, args);
    va_end(args);
    
    std::string message(buffer.data());
    Log(level, category, message);
}

bool LogHandler::ShouldLogEveryN(const std::string& key, int n)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_counters[key]++;
    return (s_counters[key] % n) == 0;
}

bool LogHandler::ShouldLogWithInterval(const std::string& key, std::chrono::milliseconds interval)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    auto now = std::chrono::steady_clock::now();
    auto& lastTime = s_lastLogTime[key];
    
    if (now - lastTime >= interval) {
        lastTime = now;
        return true;
    }
    return false;
}

bool LogHandler::IsEnabled(LogLevel level, LogCategory category)
{
    return (level >= s_minLevel) && (s_enabledCategories & category);
}

void LogHandler::FlushLogs()
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    if (s_enableFile && s_logFile.is_open()) {
        s_logFile.flush();
    }
}
