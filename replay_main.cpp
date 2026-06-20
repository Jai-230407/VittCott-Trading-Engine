#include "MatchingEngine.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

std::string trim(const std::string& text) {
    int left = 0;
    int right = (int)text.size() - 1;

    while (left <= right && std::isspace((unsigned char)text[left])) {
        left++;
    }

    while (right >= left && std::isspace((unsigned char)text[right])) {
        right--;
    }

    if (left > right) {
        return "";
    }

    return text.substr(left, right - left + 1);
}

std::string toUpper(std::string text) {
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        }
    );

    return text;
}

std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string cell;

    while (std::getline(ss, cell, ',')) {
        result.push_back(trim(cell));
    }

    return result;
}

Side parseSide(const std::string& value) {
    std::string side = toUpper(value);

    if (side == "BUY") {
        return Side::BUY;
    }

    return Side::SELL;
}

OrderType parseOrderType(const std::string& value) {
    std::string type = toUpper(value);

    if (type == "MARKET") {
        return OrderType::MARKET;
    }

    return OrderType::LIMIT;
}

TimeInForce parseTIF(const std::string& value) {
    std::string tif = toUpper(value);

    if (tif == "IOC") {
        return TimeInForce::IOC;
    }

    if (tif == "FOK") {
        return TimeInForce::FOK;
    }

    return TimeInForce::GTC;
}

double parseDoubleSafe(const std::string& value) {
    if (value.empty()) {
        return 0.0;
    }

    return std::stod(value);
}

int parseIntSafe(const std::string& value) {
    if (value.empty()) {
        return 0;
    }

    return std::stoi(value);
}

void processReplayRow(
    MatchingEngine& engine,
    const std::vector<std::string>& row,
    int lineNumber
) {
    if (row.size() < 11) {
        std::cout << "Skipping line " << lineNumber
                  << ": invalid column count\n";
        return;
    }

    std::string action = toUpper(row[0]);
    std::string symbol = toUpper(row[1]);

    Side side = parseSide(row[2]);
    OrderType orderType = parseOrderType(row[3]);
    TimeInForce tif = parseTIF(row[4]);

    double price = parseDoubleSafe(row[5]);
    int quantity = parseIntSafe(row[6]);
    int orderId = parseIntSafe(row[7]);
    double newPrice = parseDoubleSafe(row[8]);
    int newQuantity = parseIntSafe(row[9]);
    int levels = parseIntSafe(row[10]);

    std::cout << "\n-------------------------------------\n";
    std::cout << "Replay line " << lineNumber << ": " << action << "\n";

    if (action == "NEW") {
        if (orderType == OrderType::LIMIT) {
            engine.placeLimitOrder(
                symbol,
                side,
                tif,
                price,
                quantity
            );
        } else {
            engine.placeMarketOrder(
                symbol,
                side,
                tif,
                quantity
            );
        }
    } else if (action == "CANCEL") {
        engine.cancelOrder(
            symbol,
            orderId
        );
    } else if (action == "MODIFY") {
        engine.modifyOrder(
            symbol,
            orderId,
            newPrice,
            newQuantity
        );
    } else if (action == "DEPTH") {
        if (levels <= 0) {
            levels = 5;
        }

        engine.showMarketDepth(
            symbol,
            levels
        );
    } else if (action == "BOOK") {
        engine.printBook(symbol);
    } else if (action == "STATUS") {
        engine.showOrderStatus(orderId);
    } else {
        std::cout << "Unknown action on line "
                  << lineNumber << ": " << action << "\n";
    }
}

int main() {
    std::string filename = "replay_orders.csv";

    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        std::cout << "Could not open replay file: " << filename << "\n";
        return 1;
    }

    MatchingEngine engine(
        true,   // console output ON
        true,   // audit logging ON
        true    // trade CSV persistence ON
    );

    std::cout << "=====================================\n";
    std::cout << " VittCott Trading Engine - Replay\n";
    std::cout << "=====================================\n";
    std::cout << "Reading orders from: " << filename << "\n";

    std::string line;
    int lineNumber = 0;

    // Skip header
    if (std::getline(inputFile, line)) {
        lineNumber++;
    }

    while (std::getline(inputFile, line)) {
        lineNumber++;

        if (trim(line).empty()) {
            continue;
        }

        std::vector<std::string> row = splitCSVLine(line);

        try {
            processReplayRow(
                engine,
                row,
                lineNumber
            );
        } catch (const std::exception& error) {
            std::cout << "Error processing line "
                      << lineNumber
                      << ": "
                      << error.what()
                      << "\n";
        }
    }

    inputFile.close();

    std::cout << "\n=====================================\n";
    std::cout << " Replay completed\n";
    std::cout << "=====================================\n";

    std::cout << "Orders submitted: "
              << engine.getTotalOrdersSubmitted()
              << "\n";

    std::cout << "Orders rejected: "
              << engine.getTotalOrdersRejected()
              << "\n";

    std::cout << "Trades executed: "
              << engine.getTotalTradesExecuted()
              << "\n";

    std::cout << "=====================================\n";

    return 0;
}