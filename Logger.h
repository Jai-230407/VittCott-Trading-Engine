#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstddef>
#include <ctime>

class Logger {
private:
    std::string filename;

    std::vector<std::string> ringBuffer;
    std::size_t capacity;
    std::size_t head;
    std::size_t tail;
    std::size_t count;

    std::mutex mutex_;
    std::condition_variable cvNotEmpty;
    std::condition_variable cvEmpty;

    std::thread workerThread;

    bool stopRequested;
    std::atomic<long long> droppedMessages;

    std::time_t cachedSecond;
    std::string cachedTimeString;

    std::string getCurrentTime();
    void workerLoop();

    bool enqueueMessage(std::string message);

public:
    Logger(
        const std::string& filename = "system.log",
        std::size_t capacity = 1048576
    );

    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const std::string& level, const std::string& message);

    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    void flush();

    long long getDroppedMessages() const;
};

#endif