#include "OrderBook.h"
#include "RiskManager.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

int testsPassed = 0;
int testsFailed = 0;

void assertTrue(bool condition, const string& testName) {
    if (condition) {
        cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        cout << "[FAIL] " << testName << "\n";
        testsFailed++;
    }
}

void assertEqualInt(int actual, int expected, const string& testName) {
    if (actual == expected) {
        cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        cout << "[FAIL] " << testName
             << " | Expected: " << expected
             << " | Got: " << actual << "\n";
        testsFailed++;
    }
}

void assertEqualDouble(double actual, double expected, const string& testName) {
    if (fabs(actual - expected) < 1e-9) {
        cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        cout << "[FAIL] " << testName
             << " | Expected: " << expected
             << " | Got: " << actual << "\n";
        testsFailed++;
    }
}

void assertStatus(OrderStatus actual, OrderStatus expected, const string& testName) {
    if (actual == expected) {
        cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        cout << "[FAIL] " << testName
             << " | Expected: " << statusToString(expected)
             << " | Got: " << statusToString(actual) << "\n";
        testsFailed++;
    }
}

shared_ptr<Order> makeOrder(
    int orderId,
    const string& symbol,
    Side side,
    OrderType type,
    TimeInForce tif,
    double price,
    int quantity
) {
    return make_shared<Order>(
        orderId,
        symbol,
        side,
        type,
        tif,
        price,
        quantity
    );
}

void testLimitOrderRestsInBook() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 10);
    vector<Trade> trades = book.addOrder(sell, tradeId);

    assertEqualInt((int)trades.size(), 0, "GTC limit order should rest when no match exists");
    assertStatus(sell->status, OrderStatus::NEW, "Resting order status should be NEW");
    assertEqualInt(sell->remainingQuantity, 10, "Resting order quantity should remain unchanged");
}

void testBasicLimitMatch() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "Limit buy should match resting sell at same price");
    assertEqualInt(trades[0].buyOrderId, 2, "Trade should contain correct buy order ID");
    assertEqualInt(trades[0].sellOrderId, 1, "Trade should contain correct sell order ID");
    assertEqualDouble(trades[0].price, 100, "Trade price should be resting order price");
    assertEqualInt(trades[0].quantity, 5, "Trade quantity should match full order quantity");
    assertStatus(buy->status, OrderStatus::FILLED, "Incoming buy should be FILLED");
    assertStatus(sell->status, OrderStatus::FILLED, "Resting sell should be FILLED");
}

void testPartialFillRestingOrder() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 10);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 100, 4);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "Partial fill should create one trade");
    assertEqualInt(trades[0].quantity, 4, "Trade quantity should be incoming buy quantity");
    assertEqualInt(sell->remainingQuantity, 6, "Resting sell should have remaining quantity");
    assertStatus(sell->status, OrderStatus::PARTIALLY_FILLED, "Resting sell should become PARTIALLY_FILLED");
    assertStatus(buy->status, OrderStatus::FILLED, "Incoming smaller buy should be FILLED");
}

void testPartialFillIncomingGTCRests() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 100, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "Incoming GTC should partially fill first");
    assertEqualInt(buy->remainingQuantity, 5, "Incoming GTC should rest with remaining quantity");
    assertStatus(buy->status, OrderStatus::PARTIALLY_FILLED, "Incoming GTC should be PARTIALLY_FILLED");

    auto sell2 = makeOrder(3, "INFY", Side::SELL, OrderType::MARKET, TimeInForce::IOC, 0, 5);
    vector<Trade> trades2 = book.addOrder(sell2, tradeId);

    assertEqualInt((int)trades2.size(), 1, "Resting partial buy should be matched later");
    assertEqualInt(trades2[0].buyOrderId, 2, "Later sell should match resting buy order");
    assertStatus(buy->status, OrderStatus::FILLED, "Resting partial buy should become FILLED");
}

void testFIFOAtSamePrice() {
    OrderBook book;
    int tradeId = 1;

    auto sell1 = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    auto sell2 = makeOrder(2, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 7);

    book.addOrder(sell1, tradeId);
    book.addOrder(sell2, tradeId);

    auto buy = makeOrder(3, "INFY", Side::BUY, OrderType::MARKET, TimeInForce::IOC, 0, 8);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 2, "Market buy should match two FIFO orders");
    assertEqualInt(trades[0].sellOrderId, 1, "First trade should hit first sell order");
    assertEqualInt(trades[0].quantity, 5, "First FIFO order should fully fill");
    assertEqualInt(trades[1].sellOrderId, 2, "Second trade should hit second sell order");
    assertEqualInt(trades[1].quantity, 3, "Second FIFO order should partially fill");
    assertEqualInt(sell2->remainingQuantity, 4, "Second sell should have remaining quantity 4");
}

void testSellPricePriority() {
    OrderBook book;
    int tradeId = 1;

    auto sellHigh = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 105, 5);
    auto sellLow = makeOrder(2, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);

    book.addOrder(sellHigh, tradeId);
    book.addOrder(sellLow, tradeId);

    auto buy = makeOrder(3, "INFY", Side::BUY, OrderType::MARKET, TimeInForce::IOC, 0, 5);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "Market buy should match one sell order");
    assertEqualInt(trades[0].sellOrderId, 2, "Market buy should hit lowest sell price first");
    assertEqualDouble(trades[0].price, 100, "Market buy trade price should be lowest ask");
}

void testBuyPricePriority() {
    OrderBook book;
    int tradeId = 1;

    auto buyLow = makeOrder(1, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    auto buyHigh = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 105, 5);

    book.addOrder(buyLow, tradeId);
    book.addOrder(buyHigh, tradeId);

    auto sell = makeOrder(3, "INFY", Side::SELL, OrderType::MARKET, TimeInForce::IOC, 0, 5);
    vector<Trade> trades = book.addOrder(sell, tradeId);

    assertEqualInt((int)trades.size(), 1, "Market sell should match one buy order");
    assertEqualInt(trades[0].buyOrderId, 2, "Market sell should hit highest buy price first");
    assertEqualDouble(trades[0].price, 105, "Market sell trade price should be highest bid");
}

void testIOCFullFill() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::IOC, 100, 5);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "IOC should execute immediately when liquidity exists");
    assertStatus(buy->status, OrderStatus::FILLED, "Fully filled IOC should be FILLED");
}

void testIOCPartialFill() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::IOC, 100, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "IOC should partially fill available quantity");
    assertEqualInt(trades[0].quantity, 5, "IOC trade should consume available quantity");
    assertEqualInt(buy->remainingQuantity, 5, "IOC should keep unfilled quantity internally");
    assertStatus(buy->status, OrderStatus::PARTIALLY_FILLED, "Partially filled IOC should be PARTIALLY_FILLED");
}

void testIOCNoFillCancelled() {
    OrderBook book;
    int tradeId = 1;

    auto buy = makeOrder(1, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::IOC, 100, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 0, "IOC with no liquidity should create no trades");
    assertStatus(buy->status, OrderStatus::CANCELLED, "Unfilled IOC should be CANCELLED");
}

void testFOKFullFill() {
    OrderBook book;
    int tradeId = 1;

    auto sell1 = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    auto sell2 = makeOrder(2, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 101, 5);

    book.addOrder(sell1, tradeId);
    book.addOrder(sell2, tradeId);

    auto buy = makeOrder(3, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::FOK, 101, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 2, "FOK should execute when full quantity available");
    assertStatus(buy->status, OrderStatus::FILLED, "Fully executable FOK should be FILLED");
}

void testFOKRejected() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::FOK, 100, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 0, "FOK should not partially execute");
    assertStatus(buy->status, OrderStatus::REJECTED, "FOK should be REJECTED if full quantity unavailable");
    assertEqualInt(sell->remainingQuantity, 5, "Rejected FOK should not consume resting liquidity");
}

void testMarketOrderNoLiquidity() {
    OrderBook book;
    int tradeId = 1;

    auto buy = makeOrder(1, "INFY", Side::BUY, OrderType::MARKET, TimeInForce::IOC, 0, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 0, "Market order with no liquidity should create no trades");
    assertStatus(buy->status, OrderStatus::CANCELLED, "Unfilled market order should be CANCELLED");
}

void testMarketOrderPartialFill() {
    OrderBook book;
    int tradeId = 1;

    auto sell = makeOrder(1, "INFY", Side::SELL, OrderType::LIMIT, TimeInForce::GTC, 100, 5);
    book.addOrder(sell, tradeId);

    auto buy = makeOrder(2, "INFY", Side::BUY, OrderType::MARKET, TimeInForce::IOC, 0, 10);
    vector<Trade> trades = book.addOrder(buy, tradeId);

    assertEqualInt((int)trades.size(), 1, "Market order should consume available liquidity");
    assertEqualInt(buy->remainingQuantity, 5, "Market order should have unfilled remainder");
    assertStatus(buy->status, OrderStatus::PARTIALLY_FILLED, "Partially filled market order should be PARTIALLY_FILLED");
}

void testCancelOrder() {
    OrderBook book;
    int tradeId = 1;

    auto buy = makeOrder(1, "INFY", Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 100, 10);
    book.addOrder(buy, tradeId);

    bool cancelled = book.cancelOrder(1);

    assertTrue(cancelled, "Cancel should return true for active order");
    assertStatus(buy->status, OrderStatus::CANCELLED, "Cancelled order should have CANCELLED status");

    auto sell = makeOrder(2, "INFY", Side::SELL, OrderType::MARKET, TimeInForce::IOC, 0, 10);
    vector<Trade> trades = book.addOrder(sell, tradeId);

    assertEqualInt((int)trades.size(), 0, "Cancelled order should not match later");
}

void testCancelInvalidOrder() {
    OrderBook book;

    bool cancelled = book.cancelOrder(999);

    assertTrue(!cancelled, "Cancel should fail for missing order");
}

void testRiskManagerValidLimitOrder() {
    RiskManager risk;

    ValidationResult result = risk.validateLimitOrder("INFY", 100, 10);

    assertTrue(result.accepted, "Risk manager should accept valid limit order");
}

void testRiskManagerRejectsBadPrice() {
    RiskManager risk;

    ValidationResult result = risk.validateLimitOrder("INFY", -100, 10);

    assertTrue(!result.accepted, "Risk manager should reject negative price");
}

void testRiskManagerRejectsBadQuantity() {
    RiskManager risk;

    ValidationResult result = risk.validateLimitOrder("INFY", 100, 0);

    assertTrue(!result.accepted, "Risk manager should reject zero quantity");
}

void testRiskManagerRejectsBadSymbol() {
    RiskManager risk;

    ValidationResult result = risk.validateLimitOrder("INFY123", 100, 10);

    assertTrue(!result.accepted, "Risk manager should reject invalid symbol");
}

void testRiskManagerRejectsLargeNotional() {
    RiskManager risk;

    ValidationResult result = risk.validateLimitOrder("INFY", 1000000, 1000000);

    assertTrue(!result.accepted, "Risk manager should reject too-large notional value");
}

int main() {
    cout << "=====================================\n";
    cout << "   VittCott Trading Engine Tests\n";
    cout << "=====================================\n\n";

    testLimitOrderRestsInBook();
    testBasicLimitMatch();
    testPartialFillRestingOrder();
    testPartialFillIncomingGTCRests();
    testFIFOAtSamePrice();
    testSellPricePriority();
    testBuyPricePriority();

    testIOCFullFill();
    testIOCPartialFill();
    testIOCNoFillCancelled();

    testFOKFullFill();
    testFOKRejected();

    testMarketOrderNoLiquidity();
    testMarketOrderPartialFill();

    testCancelOrder();
    testCancelInvalidOrder();

    testRiskManagerValidLimitOrder();
    testRiskManagerRejectsBadPrice();
    testRiskManagerRejectsBadQuantity();
    testRiskManagerRejectsBadSymbol();
    testRiskManagerRejectsLargeNotional();

    cout << "\n=====================================\n";
    cout << "Tests passed: " << testsPassed << "\n";
    cout << "Tests failed: " << testsFailed << "\n";
    cout << "=====================================\n";

    if (testsFailed == 0) {
        cout << "ALL TESTS PASSED ✅\n";
        return 0;
    }

    cout << "SOME TESTS FAILED ❌\n";
    return 1;
}