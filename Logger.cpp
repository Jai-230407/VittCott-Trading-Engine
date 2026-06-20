#include "Logger.h"

#include <fstream>
#include <iostream>
#include <utility>

Logger::Logger(
    const std::string& filename,
    std::size_t capacity
) {
    this->filename = filename;
    this->capacity = capacity;

    head = 0;
    tail = 0;
    count = 0;

    stopRequested = false;
    droppedMessages = 0;

    cachedSecond = 0;
    cachedTimeString = "";

    ringBuffer.resize(capacity);

    workerThread = std::thread(&Logger::workerLoop, this);
}

Logger::~Logger() {
    flush();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopRequested = true;
    }

    cvNotEmpty.notify_one();

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

std::string Logger::getCurrentTime() {
    std::time_t now = std::time(nullptr);

    if (now == cachedSecond && !cachedTimeString.empty()) {
        return cachedTimeString;
    }

    cachedSecond = now;

    std::tm localTimeData;

#ifdef _WIN32
    localtime_s(&localTimeData, &now);
#else
    localtime_r(&now, &localTimeData);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTimeData);

    cachedTimeString = std::string(buffer);
    return cachedTimeString;
}

bool Logger::enqueueMessage(std::string message) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (count == capacity) {
            droppedMessages++;
            return false;
        }

        ringBuffer[tail] = std::move(message);
        tail = (tail + 1) % capacity;
        count++;
    }

    cvNotEmpty.notify_one();
    return true;
}

void Logger::workerLoop() {
    std::ofstream outputFile(filename, std::ios::app);

    if (!outputFile.is_open()) {
        std::cerr << "Logger Error: Could not open log file.\n";
        return;
    }

    std::vector<std::string> localBatch;
    localBatch.reserve(8192);

    while (true) {
        localBatch.clear();

        {
            std::unique_lock<std::mutex> lock(mutex_);

            cvNotEmpty.wait(lock, [this]() {
                return stopRequested || count > 0;
            });

            while (count > 0 && localBatch.size() < 8192) {
                localBatch.push_back(std::move(ringBuffer[head]));
                head = (head + 1) % capacity;
                count--;
            }

            if (count == 0) {
                cvEmpty.notify_all();
            }

            if (localBatch.empty() && stopRequested) {
                break;
            }
        }

        for (const std::string& message : localBatch) {
            outputFile << message << '\n';
        }
    }

    long long dropped = droppedMessages.load();

    if (dropped > 0) {
        outputFile << "[LOGGER] dropped_messages=" << dropped << '\n';
    }

    outputFile.flush();
    outputFile.close();
}

void Logger::log(const std::string& level, const std::string& message) {
    std::string finalMessage;

    finalMessage.reserve(
        cachedTimeString.size()
        + level.size()
        + message.size()
        + 8
    );

    finalMessage += "[";
    finalMessage += getCurrentTime();
    finalMessage += "] [";
    finalMessage += level;
    finalMessage += "] ";
    finalMessage += message;

    enqueueMessage(std::move(finalMessage));
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}

void Logger::warning(const std::string& message) {
    log("WARNING", message);
}

void Logger::error(const std::string& message) {
    log("ERROR", message);
}

void Logger::flush() {
    std::unique_lock<std::mutex> lock(mutex_);

    cvEmpty.wait(lock, [this]() {
        return count == 0;
    });
}

long long Logger::getDroppedMessages() const {
    return droppedMessages.load();
}