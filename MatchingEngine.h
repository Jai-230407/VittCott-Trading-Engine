#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "OrderBook.h"
#include "RiskManager.h"
#include "TradeLogger.h"
#include "Logger.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstddef>

class MatchingEngine {
private:
    std::unordered_map<std::string, OrderBook> books;
    std::unordered_map<int, std::shared_ptr<Order>> orderHistory;

    int orderIdCounter;
    int tradeIdCounter;

    RiskManager riskManager;
    TradeLogger tradeLogger;
    Logger auditLogger;

    bool consoleOutputEnabled;
    bool auditLoggingEnabled;
    bool tradePersistenceEnabled;

    long long totalOrdersSubmitted;
    long long totalOrdersRejected;
    long long totalTradesExecuted;

    void handleTrades(const std::vector<Trade>& trades);

public:
    MatchingEngine(
        bool enableConsoleOutput = true,
        bool enableAuditLogging = true,
        bool enableTradePersistence = true
    );

    void reserveCapacity(std::size_t expectedOrders);

    void placeLimitOrder(
        const std::string& symbol,
        Side side,
        TimeInForce timeInForce,
        double price,
        int quantity
    );

    void placeMarketOrder(
        const std::string& symbol,
        Side side,
        TimeInForce timeInForce,
        int quantity
    );

    bool cancelOrder(
        const std::string& symbol,
        int orderId
    );

    bool modifyOrder(
        const std::string& symbol,
        int orderId,
        double newPrice,
        int newQuantity
    );

    void printBook(const std::string& symbol);

    void showOrderStatus(int orderId);

    void showMarketDepth(
        const std::string& symbol,
        int levels = 5
    );

    long long getTotalOrdersSubmitted() const;
    long long getTotalOrdersRejected() const;
    long long getTotalTradesExecuted() const;
};

#endif