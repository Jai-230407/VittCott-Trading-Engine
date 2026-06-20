#include "TradeLogger.h"

#include <fstream>
#include <iostream>

TradeLogger::TradeLogger(const std::string& filename) {
    this->filename = filename;
    initializeFile();
}

void TradeLogger::initializeFile() {
    std::ifstream inputFile(filename);

    bool fileExists = inputFile.good();
    bool fileEmpty = true;

    if (fileExists) {
        inputFile.seekg(0, std::ios::end);
        fileEmpty = (inputFile.tellg() == 0);
    }

    inputFile.close();

    if (!fileExists || fileEmpty) {
        std::ofstream outputFile(filename);

        outputFile << "TradeID,BuyOrderID,SellOrderID,Symbol,Price,Quantity,Timestamp\n";

        outputFile.close();
    }
}

void TradeLogger::logTrade(const Trade& trade) {
    std::ofstream outputFile(filename, std::ios::app);

    if (!outputFile.is_open()) {
        std::cout << "Error: Could not open trade log file.\n";
        return;
    }

    outputFile << trade.tradeId << ","
               << trade.buyOrderId << ","
               << trade.sellOrderId << ","
               << trade.symbol << ","
               << trade.price << ","
               << trade.quantity << ","
               << trade.timestamp << "\n";

    outputFile.close();
}

void TradeLogger::logTrades(const std::vector<Trade>& trades) {
    for (const Trade& trade : trades) {
        logTrade(trade);
    }
}