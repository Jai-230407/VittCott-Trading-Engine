#ifndef TRADELOGGER_H
#define TRADELOGGER_H

#include "Trade.h"

#include <string>
#include <vector>

class TradeLogger {
private:
    std::string filename;

    void initializeFile();

public:
    TradeLogger(const std::string& filename = "trades.csv");

    void logTrade(const Trade& trade);
    void logTrades(const std::vector<Trade>& trades);
};

#endif