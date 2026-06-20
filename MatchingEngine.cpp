#include "MatchingEngine.h"

#include <iostream>

MatchingEngine::MatchingEngine(
    bool enableConsoleOutput,
    bool enableAuditLogging,
    bool enableTradePersistence
)
    : auditLogger("system.log")
{
    orderIdCounter = 1;
    tradeIdCounter = 1;

    consoleOutputEnabled = enableConsoleOutput;
    auditLoggingEnabled = enableAuditLogging;
    tradePersistenceEnabled = enableTradePersistence;

    totalOrdersSubmitted = 0;
    totalOrdersRejected = 0;
    totalTradesExecuted = 0;
}

void MatchingEngine::reserveCapacity(std::size_t expectedOrders) {
    orderHistory.reserve(expectedOrders);
    books.reserve(64);
}

void MatchingEngine::handleTrades(const std::vector<Trade>& trades) {
    totalTradesExecuted += trades.size();

    for (const auto& trade : trades) {
        if (consoleOutputEnabled) {
            std::cout << "\nTRADE EXECUTED\n";
            std::cout << "Trade ID: " << trade.tradeId << "\n";
            std::cout << "Symbol: " << trade.symbol << "\n";
            std::cout << "Buy Order ID: " << trade.buyOrderId << "\n";
            std::cout << "Sell Order ID: " << trade.sellOrderId << "\n";
            std::cout << "Price: " << trade.price << "\n";
            std::cout << "Quantity: " << trade.quantity << "\n";
        }

        if (tradePersistenceEnabled) {
            tradeLogger.logTrade(trade);
        }

        if (auditLoggingEnabled) {
            auditLogger.info(
                "TRADE_EXECUTED tradeId=" + std::to_string(trade.tradeId) +
                " symbol=" + trade.symbol +
                " buyOrderId=" + std::to_string(trade.buyOrderId) +
                " sellOrderId=" + std::to_string(trade.sellOrderId) +
                " price=" + std::to_string(trade.price) +
                " quantity=" + std::to_string(trade.quantity)
            );
        }
    }
}

void MatchingEngine::placeLimitOrder(
    const std::string& symbol,
    Side side,
    TimeInForce timeInForce,
    double price,
    int quantity
) {
    totalOrdersSubmitted++;

    ValidationResult result = riskManager.validateLimitOrder(
        symbol,
        price,
        quantity
    );

    if (!result.accepted) {
        totalOrdersRejected++;

        if (consoleOutputEnabled) {
            std::cout << "Order rejected: " << result.reason << "\n";
        }

        if (auditLoggingEnabled) {
            auditLogger.warning(
                "LIMIT_ORDER_REJECTED symbol=" + symbol +
                " reason=" + result.reason
            );
        }

        return;
    }

    int orderId = orderIdCounter++;

    auto order = std::make_shared<Order>(
        orderId,
        symbol,
        side,
        OrderType::LIMIT,
        timeInForce,
        price,
        quantity
    );

    orderHistory[orderId] = order;

    if (auditLoggingEnabled) {
        auditLogger.info(
            "LIMIT_ORDER_RECEIVED orderId=" + std::to_string(orderId) +
            " symbol=" + symbol +
            " side=" + sideToString(side) +
            " tif=" + tifToString(timeInForce) +
            " price=" + std::to_string(price) +
            " quantity=" + std::to_string(quantity)
        );
    }

    std::vector<Trade> trades = books[symbol].addOrder(
        order,
        tradeIdCounter
    );

    handleTrades(trades);

    if (consoleOutputEnabled) {
        std::cout << "Order ID: " << orderId << "\n";
        std::cout << "Status: " << statusToString(order->status) << "\n";
        std::cout << "Filled Quantity: " << order->filledQuantity() << "\n";
        std::cout << "Remaining Quantity: " << order->remainingQuantity << "\n";
    }
}

void MatchingEngine::placeMarketOrder(
    const std::string& symbol,
    Side side,
    TimeInForce timeInForce,
    int quantity
) {
    totalOrdersSubmitted++;

    ValidationResult result = riskManager.validateMarketOrder(
        symbol,
        quantity
    );

    if (!result.accepted) {
        totalOrdersRejected++;

        if (consoleOutputEnabled) {
            std::cout << "Order rejected: " << result.reason << "\n";
        }

        if (auditLoggingEnabled) {
            auditLogger.warning(
                "MARKET_ORDER_REJECTED symbol=" + symbol +
                " reason=" + result.reason
            );
        }

        return;
    }

    if (timeInForce == TimeInForce::GTC) {
        timeInForce = TimeInForce::IOC;
    }

    int orderId = orderIdCounter++;

    auto order = std::make_shared<Order>(
        orderId,
        symbol,
        side,
        OrderType::MARKET,
        timeInForce,
        0.0,
        quantity
    );

    orderHistory[orderId] = order;

    if (auditLoggingEnabled) {
        auditLogger.info(
            "MARKET_ORDER_RECEIVED orderId=" + std::to_string(orderId) +
            " symbol=" + symbol +
            " side=" + sideToString(side) +
            " quantity=" + std::to_string(quantity)
        );
    }

    std::vector<Trade> trades = books[symbol].addOrder(
        order,
        tradeIdCounter
    );

    handleTrades(trades);

    if (consoleOutputEnabled) {
        std::cout << "Order ID: " << orderId << "\n";
        std::cout << "Status: " << statusToString(order->status) << "\n";
        std::cout << "Filled Quantity: " << order->filledQuantity() << "\n";
        std::cout << "Remaining Quantity: " << order->remainingQuantity << "\n";
    }
}

bool MatchingEngine::cancelOrder(
    const std::string& symbol,
    int orderId
) {
    auto bookIt = books.find(symbol);

    if (bookIt == books.end()) {
        if (consoleOutputEnabled) {
            std::cout << "No order book found for symbol: " << symbol << "\n";
        }

        return false;
    }

    bool cancelled = bookIt->second.cancelOrder(orderId);

    if (cancelled) {
        if (consoleOutputEnabled) {
            std::cout << "Order cancelled successfully.\n";
        }

        if (auditLoggingEnabled) {
            auditLogger.info(
                "ORDER_CANCELLED orderId=" + std::to_string(orderId) +
                " symbol=" + symbol
            );
        }
    } else {
        if (consoleOutputEnabled) {
            std::cout << "Cancel failed. Order not found or already inactive.\n";
        }

        if (auditLoggingEnabled) {
            auditLogger.warning(
                "ORDER_CANCEL_FAILED orderId=" + std::to_string(orderId) +
                " symbol=" + symbol
            );
        }
    }

    return cancelled;
}

bool MatchingEngine::modifyOrder(
    const std::string& symbol,
    int orderId,
    double newPrice,
    int newQuantity
) {
    auto it = orderHistory.find(orderId);

    if (it == orderHistory.end()) {
        if (consoleOutputEnabled) {
            std::cout << "Order not found.\n";
        }

        return false;
    }

    auto oldOrder = it->second;

    bool cancelled = cancelOrder(symbol, orderId);

    if (!cancelled) {
        if (consoleOutputEnabled) {
            std::cout << "Modify failed because old order could not be cancelled.\n";
        }

        return false;
    }

    placeLimitOrder(
        symbol,
        oldOrder->side,
        oldOrder->timeInForce,
        newPrice,
        newQuantity
    );

    if (auditLoggingEnabled) {
        auditLogger.info(
            "ORDER_MODIFIED oldOrderId=" + std::to_string(orderId) +
            " symbol=" + symbol +
            " newPrice=" + std::to_string(newPrice) +
            " newQuantity=" + std::to_string(newQuantity)
        );
    }

    return true;
}

void MatchingEngine::printBook(const std::string& symbol) {
    auto it = books.find(symbol);

    if (it == books.end()) {
        std::cout << "No order book found for symbol: " << symbol << "\n";
        return;
    }

    std::cout << "\nSymbol: " << symbol << "\n";
    it->second.printOrderBook();
}

void MatchingEngine::showOrderStatus(int orderId) {
    auto it = orderHistory.find(orderId);

    if (it == orderHistory.end()) {
        std::cout << "Order not found.\n";
        return;
    }

    auto order = it->second;

    std::cout << "\n========== ORDER STATUS ==========\n";
    std::cout << "Order ID: " << order->orderId << "\n";
    std::cout << "Symbol: " << order->symbol << "\n";
    std::cout << "Side: " << sideToString(order->side) << "\n";
    std::cout << "Type: " << orderTypeToString(order->type) << "\n";
    std::cout << "TIF: " << tifToString(order->timeInForce) << "\n";
    std::cout << "Price: " << order->price << "\n";
    std::cout << "Original Quantity: " << order->quantity << "\n";
    std::cout << "Filled Quantity: " << order->filledQuantity() << "\n";
    std::cout << "Remaining Quantity: " << order->remainingQuantity << "\n";
    std::cout << "Status: " << statusToString(order->status) << "\n";
    std::cout << "==================================\n";
}

void MatchingEngine::showMarketDepth(
    const std::string& symbol,
    int levels
) {
    auto it = books.find(symbol);

    if (it == books.end()) {
        std::cout << "No order book found for symbol: " << symbol << "\n";
        return;
    }

    std::cout << "\nSymbol: " << symbol << "\n";
    it->second.printMarketDepth(levels);
}

long long MatchingEngine::getTotalOrdersSubmitted() const {
    return totalOrdersSubmitted;
}

long long MatchingEngine::getTotalOrdersRejected() const {
    return totalOrdersRejected;
}

long long MatchingEngine::getTotalTradesExecuted() const {
    return totalTradesExecuted;
}