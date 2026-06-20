#include "RiskManager.h"

#include <cctype>

RiskManager::RiskManager() {
    maxQuantity = 1000000;
    maxPrice = 1000000.0;
    maxNotional = 100000000.0;
}

bool RiskManager::isValidSymbol(const std::string& symbol) {
    if (symbol.empty()) {
        return false;
    }

    if (symbol.size() > 10) {
        return false;
    }

    for (char ch : symbol) {
        if (!std::isalpha(static_cast<unsigned char>(ch))) {
            return false;
        }
    }

    return true;
}

ValidationResult RiskManager::validateLimitOrder(
    const std::string& symbol,
    double price,
    int quantity
) {
    if (!isValidSymbol(symbol)) {
        return ValidationResult(false, "Invalid symbol. Symbol must contain only letters and max length 10.");
    }

    if (price <= 0) {
        return ValidationResult(false, "Invalid price. Price must be greater than 0.");
    }

    if (quantity <= 0) {
        return ValidationResult(false, "Invalid quantity. Quantity must be greater than 0.");
    }

    if (quantity > maxQuantity) {
        return ValidationResult(false, "Rejected by risk check. Quantity too large.");
    }

    if (price > maxPrice) {
        return ValidationResult(false, "Rejected by risk check. Price too large.");
    }

    double notional = price * quantity;

    if (notional > maxNotional) {
        return ValidationResult(false, "Rejected by risk check. Notional value too large.");
    }

    return ValidationResult(true, "Accepted");
}

ValidationResult RiskManager::validateMarketOrder(
    const std::string& symbol,
    int quantity
) {
    if (!isValidSymbol(symbol)) {
        return ValidationResult(false, "Invalid symbol. Symbol must contain only letters and max length 10.");
    }

    if (quantity <= 0) {
        return ValidationResult(false, "Invalid quantity. Quantity must be greater than 0.");
    }

    if (quantity > maxQuantity) {
        return ValidationResult(false, "Rejected by risk check. Quantity too large.");
    }

    return ValidationResult(true, "Accepted");
}