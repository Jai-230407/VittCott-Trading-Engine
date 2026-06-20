#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <chrono>

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET
};

enum class TimeInForce {
    GTC,   // Good Till Cancelled
    IOC,   // Immediate Or Cancel
    FOK    // Fill Or Kill
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

inline std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string orderTypeToString(OrderType type) {
    return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

inline std::string tifToString(TimeInForce tif) {
    if (tif == TimeInForce::GTC) return "GTC";
    if (tif == TimeInForce::IOC) return "IOC";
    return "FOK";
}

inline std::string statusToString(OrderStatus status) {
    if (status == OrderStatus::NEW) return "NEW";
    if (status == OrderStatus::PARTIALLY_FILLED) return "PARTIALLY_FILLED";
    if (status == OrderStatus::FILLED) return "FILLED";
    if (status == OrderStatus::CANCELLED) return "CANCELLED";
    return "REJECTED";
}

struct Order {
    int orderId;
    std::string symbol;
    Side side;
    OrderType type;
    TimeInForce timeInForce;

    double price;
    int quantity;
    int remainingQuantity;

    OrderStatus status;
    long long timestamp;

    Order(
        int orderId,
        const std::string& symbol,
        Side side,
        OrderType type,
        TimeInForce timeInForce,
        double price,
        int quantity
    ) {
        this->orderId = orderId;
        this->symbol = symbol;
        this->side = side;
        this->type = type;
        this->timeInForce = timeInForce;
        this->price = price;
        this->quantity = quantity;
        this->remainingQuantity = quantity;
        this->status = OrderStatus::NEW;

        this->timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    int filledQuantity() const {
        return quantity - remainingQuantity;
    }
};

#endif