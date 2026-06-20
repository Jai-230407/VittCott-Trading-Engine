#ifndef TRADE_H
#define TRADE_H

#include <string>
#include <chrono>

struct Trade {
    int tradeId;
    int buyOrderId;
    int sellOrderId;
    std::string symbol;
    double price;
    int quantity;
    long long timestamp;

    Trade(
        int tradeId,
        int buyOrderId,
        int sellOrderId,
        const std::string& symbol,
        double price,
        int quantity
    ) {
        this->tradeId = tradeId;
        this->buyOrderId = buyOrderId;
        this->sellOrderId = sellOrderId;
        this->symbol = symbol;
        this->price = price;
        this->quantity = quantity;

        this->timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

#endif