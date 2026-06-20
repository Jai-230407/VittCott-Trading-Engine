#ifndef RISKMANAGER_H
#define RISKMANAGER_H

#include <string>

struct ValidationResult {
    bool accepted;
    std::string reason;

    ValidationResult(bool accepted, const std::string& reason) {
        this->accepted = accepted;
        this->reason = reason;
    }
};

class RiskManager {
private:
    int maxQuantity;
    double maxPrice;
    double maxNotional;

    bool isValidSymbol(const std::string& symbol);

public:
    RiskManager();

    ValidationResult validateLimitOrder(
        const std::string& symbol,
        double price,
        int quantity
    );

    ValidationResult validateMarketOrder(
        const std::string& symbol,
        int quantity
    );
};

#endif