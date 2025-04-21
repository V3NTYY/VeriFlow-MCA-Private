#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <mutex>
#include <sstream>

class Log {
public:
    // Singleton instance
    static Log& getInstance() {
        static Log instance;
        return instance;
    }
    
    // Overload the `<<` operator for stream-like logging
    template <typename T>
    Log& operator<<(const T& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << message;
        return *this;
    }

    // Overload the `<<` operator for manipulators like std::endl
    Log& operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << manip; // Apply the manipulator (e.g., std::endl)
        return *this;
    }

    // Function-style logging for normal messages
    template <typename T>
    void logMessage(const T& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << message;
    }

    // Function-style logging for error messages
    template <typename T>
    void logErrorMessage(const T& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << message;
    }

private:
    // Private constructor and destructor
    Log() = default;
    ~Log() = default;

    // Disable copy and assignment
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    // Mutex for thread safety
    std::mutex logMutex;
};

// Macros for logging
#define loggy (Log::getInstance()) // Stream-like logging
#define loggyMsg(message) Log::getInstance().logMessage(message) // Function-style logging
#define loggyErr(message) Log::getInstance().logErrorMessage(message) // Function-style error logging

#endif