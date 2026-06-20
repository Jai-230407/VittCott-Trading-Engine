#include "OrderBook.h"

#include <iostream>
#include <algorithm>

bool OrderBook::hasAnyFill(std::shared_ptr<Order> order) const {
    return order->remainingQuantity < order->quantity;
}

void OrderBook::addToBook(std::shared_ptr<Order> order) {
    if (order->side == Side::BUY) {
        auto& level = buyLevels[order->price];
        level.push_back(order);

        auto it = level.end();
        --it;

        orderLocations[order->orderId] = {
            order->side,
            order->price,
            it
        };
    } else {
        auto& level = sellLevels[order->price];
        level.push_back(order);

        auto it = level.end();
        --it;

        orderLocations[order->orderId] = {
            order->side,
            order->price,
            it
        };
    }
}

bool OrderBook::canFullyFill(std::shared_ptr<Order> order) const {
    int requiredQuantity = order->remainingQuantity;

    if (order->side == Side::BUY) {
        for (const auto& level : sellLevels) {
            double askPrice = level.first;

            if (order->type == OrderType::LIMIT && askPrice > order->price) {
                break;
            }

            for (const auto& restingOrder : level.second) {
                if (restingOrder->status != OrderStatus::CANCELLED &&
                    restingOrder->status != OrderStatus::FILLED) {
                    requiredQuantity -= restingOrder->remainingQuantity;

                    if (requiredQuantity <= 0) {
                        return true;
                    }
                }
            }
        }
    } else {
        for (const auto& level : buyLevels) {
            double bidPrice = level.first;

            if (order->type == OrderType::LIMIT && bidPrice < order->price) {
                break;
            }

            for (const auto& restingOrder : level.second) {
                if (restingOrder->status != OrderStatus::CANCELLED &&
                    restingOrder->status != OrderStatus::FILLED) {
                    requiredQuantity -= restingOrder->remainingQuantity;

                    if (requiredQuantity <= 0) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

std::vector<Trade> OrderBook::matchBuyOrder(
    std::shared_ptr<Order> order,
    int& tradeIdCounter
) {
    std::vector<Trade> trades;

    while (order->remainingQuantity > 0 && !sellLevels.empty()) {
        auto bestAskIt = sellLevels.begin();
        double bestAskPrice = bestAskIt->first;

        if (order->type == OrderType::LIMIT && bestAskPrice > order->price) {
            break;
        }

        auto& sellQueue = bestAskIt->second;

        while (order->remainingQuantity > 0 && !sellQueue.empty()) {
            auto restingSell = sellQueue.front();

            if (restingSell->status == OrderStatus::CANCELLED ||
                restingSell->status == OrderStatus::FILLED) {
                sellQueue.pop_front();
                continue;
            }

            int tradeQuantity = std::min(
                order->remainingQuantity,
                restingSell->remainingQuantity
            );

            double tradePrice = restingSell->price;

            order->remainingQuantity -= tradeQuantity;
            restingSell->remainingQuantity -= tradeQuantity;

            trades.emplace_back(
                tradeIdCounter++,
                order->orderId,
                restingSell->orderId,
                order->symbol,
                tradePrice,
                tradeQuantity
            );

            if (restingSell->remainingQuantity == 0) {
                restingSell->status = OrderStatus::FILLED;
                orderLocations.erase(restingSell->orderId);
                sellQueue.pop_front();
            } else {
                restingSell->status = OrderStatus::PARTIALLY_FILLED;
            }
        }

        if (sellQueue.empty()) {
            sellLevels.erase(bestAskIt);
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::matchSellOrder(
    std::shared_ptr<Order> order,
    int& tradeIdCounter
) {
    std::vector<Trade> trades;

    while (order->remainingQuantity > 0 && !buyLevels.empty()) {
        auto bestBidIt = buyLevels.begin();
        double bestBidPrice = bestBidIt->first;

        if (order->type == OrderType::LIMIT && bestBidPrice < order->price) {
            break;
        }

        auto& buyQueue = bestBidIt->second;

        while (order->remainingQuantity > 0 && !buyQueue.empty()) {
            auto restingBuy = buyQueue.front();

            if (restingBuy->status == OrderStatus::CANCELLED ||
                restingBuy->status == OrderStatus::FILLED) {
                buyQueue.pop_front();
                continue;
            }

            int tradeQuantity = std::min(
                order->remainingQuantity,
                restingBuy->remainingQuantity
            );

            double tradePrice = restingBuy->price;

            order->remainingQuantity -= tradeQuantity;
            restingBuy->remainingQuantity -= tradeQuantity;

            trades.emplace_back(
                tradeIdCounter++,
                restingBuy->orderId,
                order->orderId,
                order->symbol,
                tradePrice,
                tradeQuantity
            );

            if (restingBuy->remainingQuantity == 0) {
                restingBuy->status = OrderStatus::FILLED;
                orderLocations.erase(restingBuy->orderId);
                buyQueue.pop_front();
            } else {
                restingBuy->status = OrderStatus::PARTIALLY_FILLED;
            }
        }

        if (buyQueue.empty()) {
            buyLevels.erase(bestBidIt);
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::addOrder(
    std::shared_ptr<Order> order,
    int& tradeIdCounter
) {
    std::vector<Trade> trades;

    if (order->timeInForce == TimeInForce::FOK && !canFullyFill(order)) {
        order->status = OrderStatus::REJECTED;
        return trades;
    }

    if (order->side == Side::BUY) {
        trades = matchBuyOrder(order, tradeIdCounter);
    } else {
        trades = matchSellOrder(order, tradeIdCounter);
    }

    if (order->remainingQuantity == 0) {
        order->status = OrderStatus::FILLED;
        return trades;
    }

    if (hasAnyFill(order)) {
        order->status = OrderStatus::PARTIALLY_FILLED;
    }

    if (order->type == OrderType::LIMIT &&
        order->timeInForce == TimeInForce::GTC) {
        if (!hasAnyFill(order)) {
            order->status = OrderStatus::NEW;
        }

        addToBook(order);
    } else {
        if (!hasAnyFill(order)) {
            order->status = OrderStatus::CANCELLED;
        }
    }

    return trades;
}

bool OrderBook::cancelOrder(int orderId) {
    auto it = orderLocations.find(orderId);

    if (it == orderLocations.end()) {
        return false;
    }

    OrderLocation location = it->second;

    if (location.side == Side::BUY) {
        auto levelIt = buyLevels.find(location.price);

        if (levelIt == buyLevels.end()) {
            return false;
        }

        auto order = *(location.iterator);
        order->status = OrderStatus::CANCELLED;

        levelIt->second.erase(location.iterator);

        if (levelIt->second.empty()) {
            buyLevels.erase(levelIt);
        }
    } else {
        auto levelIt = sellLevels.find(location.price);

        if (levelIt == sellLevels.end()) {
            return false;
        }

        auto order = *(location.iterator);
        order->status = OrderStatus::CANCELLED;

        levelIt->second.erase(location.iterator);

        if (levelIt->second.empty()) {
            sellLevels.erase(levelIt);
        }
    }

    orderLocations.erase(orderId);
    return true;
}

void OrderBook::printOrderBook() const {
    std::cout << "\n========== ORDER BOOK ==========\n";

    std::cout << "\n----- SELL ORDERS -----\n";
    std::cout << "Price\tQuantity\tOrders\n";

    for (const auto& level : sellLevels) {
        int totalQuantity = 0;
        int orderCount = 0;

        for (const auto& order : level.second) {
            if (order->status != OrderStatus::CANCELLED &&
                order->status != OrderStatus::FILLED) {
                totalQuantity += order->remainingQuantity;
                orderCount++;
            }
        }

        if (orderCount > 0) {
            std::cout << level.first << "\t"
                      << totalQuantity << "\t\t"
                      << orderCount << "\n";
        }
    }

    std::cout << "\n----- BUY ORDERS -----\n";
    std::cout << "Price\tQuantity\tOrders\n";

    for (const auto& level : buyLevels) {
        int totalQuantity = 0;
        int orderCount = 0;

        for (const auto& order : level.second) {
            if (order->status != OrderStatus::CANCELLED &&
                order->status != OrderStatus::FILLED) {
                totalQuantity += order->remainingQuantity;
                orderCount++;
            }
        }

        if (orderCount > 0) {
            std::cout << level.first << "\t"
                      << totalQuantity << "\t\t"
                      << orderCount << "\n";
        }
    }

    std::cout << "================================\n";
}

double OrderBook::getBestBid() const {
    if (buyLevels.empty()) {
        return -1.0;
    }

    return buyLevels.begin()->first;
}

double OrderBook::getBestAsk() const {
    if (sellLevels.empty()) {
        return -1.0;
    }

    return sellLevels.begin()->first;
}

double OrderBook::getSpread() const {
    double bestBid = getBestBid();
    double bestAsk = getBestAsk();

    if (bestBid < 0 || bestAsk < 0) {
        return -1.0;
    }

    return bestAsk - bestBid;
}

int OrderBook::getTotalBuyQuantityAtPrice(double price) const {
    auto it = buyLevels.find(price);

    if (it == buyLevels.end()) {
        return 0;
    }

    int totalQuantity = 0;

    for (const auto& order : it->second) {
        if (order->status != OrderStatus::CANCELLED &&
            order->status != OrderStatus::FILLED) {
            totalQuantity += order->remainingQuantity;
        }
    }

    return totalQuantity;
}

int OrderBook::getTotalSellQuantityAtPrice(double price) const {
    auto it = sellLevels.find(price);

    if (it == sellLevels.end()) {
        return 0;
    }

    int totalQuantity = 0;

    for (const auto& order : it->second) {
        if (order->status != OrderStatus::CANCELLED &&
            order->status != OrderStatus::FILLED) {
            totalQuantity += order->remainingQuantity;
        }
    }

    return totalQuantity;
}

void OrderBook::printMarketDepth(int levels) const {
    std::cout << "\n========== MARKET DEPTH ==========\n";

    double bestBid = getBestBid();
    double bestAsk = getBestAsk();
    double spread = getSpread();

    if (bestBid < 0) {
        std::cout << "Best Bid: NONE\n";
    } else {
        std::cout << "Best Bid: " << bestBid << "\n";
    }

    if (bestAsk < 0) {
        std::cout << "Best Ask: NONE\n";
    } else {
        std::cout << "Best Ask: " << bestAsk << "\n";
    }

    if (spread < 0) {
        std::cout << "Spread: NONE\n";
    } else {
        std::cout << "Spread: " << spread << "\n";
    }

    std::cout << "\n----- ASKS -----\n";
    std::cout << "Price\tQuantity\n";

    int printed = 0;

    for (auto it = sellLevels.begin(); it != sellLevels.end() && printed < levels; ++it) {
        int quantity = getTotalSellQuantityAtPrice(it->first);

        if (quantity > 0) {
            std::cout << it->first << "\t" << quantity << "\n";
            printed++;
        }
    }

    if (printed == 0) {
        std::cout << "No asks\n";
    }

    std::cout << "\n----- BIDS -----\n";
    std::cout << "Price\tQuantity\n";

    printed = 0;

    for (auto it = buyLevels.begin(); it != buyLevels.end() && printed < levels; ++it) {
        int quantity = getTotalBuyQuantityAtPrice(it->first);

        if (quantity > 0) {
            std::cout << it->first << "\t" << quantity << "\n";
            printed++;
        }
    }

    if (printed == 0) {
        std::cout << "No bids\n";
    }

    std::cout << "==================================\n";
    
    
}
