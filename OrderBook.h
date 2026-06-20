#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"
#include "Trade.h"

#include <map>
#include <list>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

class OrderBook {
private:
    using OrderList = std::list<std::shared_ptr<Order>>;

    struct OrderLocation {
        Side side;
        double price;
        OrderList::iterator iterator;
    };

    std::map<double, OrderList, std::greater<double>> buyLevels;
    std::map<double, OrderList> sellLevels;

    std::unordered_map<int, OrderLocation> orderLocations;

    void addToBook(std::shared_ptr<Order> order);

    bool hasAnyFill(std::shared_ptr<Order> order) const;

    bool canFullyFill(std::shared_ptr<Order> order) const;

    std::vector<Trade> matchBuyOrder(
        std::shared_ptr<Order> order,
        int& tradeIdCounter
    );

    std::vector<Trade> matchSellOrder(
        std::shared_ptr<Order> order,
        int& tradeIdCounter
    );

public:
    std::vector<Trade> addOrder(
        std::shared_ptr<Order> order,
        int& tradeIdCounter
    );

    bool cancelOrder(int orderId);

    void printOrderBook() const;

    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;

    int getTotalBuyQuantityAtPrice(double price) const;
    int getTotalSellQuantityAtPrice(double price) const;

    void printMarketDepth(int levels = 5) const;
};

#endif