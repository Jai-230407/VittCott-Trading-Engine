#include "MatchingEngine.h"

#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace std;

int main() {
    const int TOTAL_ORDERS = 100000;

    MatchingEngine engine(
        false,  // console output disabled
        false,  // audit logging disabled
        false   // trade CSV logging disabled
    );

    engine.reserveCapacity(TOTAL_ORDERS + 10000);

    mt19937 rng(42);

    uniform_int_distribution<int> sideDist(0, 1);
    uniform_int_distribution<int> qtyDist(1, 100);
    uniform_int_distribution<int> priceOffsetDist(-50, 50);
    uniform_int_distribution<int> orderTypeDist(0, 9);

    const string symbol = "INFY";
    const double basePrice = 1000.0;

    // Seed initial liquidity.
    // This makes sure market orders and aggressive limit orders have something to match.
    for (int i = 0; i < 5000; i++) {
        double sellPrice = basePrice + (i % 50);
        double buyPrice = basePrice - (i % 50);

        engine.placeLimitOrder(
            symbol,
            Side::SELL,
            TimeInForce::GTC,
            sellPrice,
            100
        );

        engine.placeLimitOrder(
            symbol,
            Side::BUY,
            TimeInForce::GTC,
            buyPrice,
            100
        );
    }

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < TOTAL_ORDERS; i++) {
        int sideRandom = sideDist(rng);
        int quantity = qtyDist(rng);
        int priceOffset = priceOffsetDist(rng);
        int orderTypeRandom = orderTypeDist(rng);

        Side side = (sideRandom == 0) ? Side::BUY : Side::SELL;
        double price = basePrice + priceOffset;

        // 80% limit GTC orders
        // 10% limit IOC orders
        // 5% limit FOK orders
        // 5% market IOC orders
        if (orderTypeRandom <= 7) {
            engine.placeLimitOrder(
                symbol,
                side,
                TimeInForce::GTC,
                price,
                quantity
            );
        } else if (orderTypeRandom == 8) {
            engine.placeLimitOrder(
                symbol,
                side,
                TimeInForce::IOC,
                price,
                quantity
            );
        } else {
            engine.placeMarketOrder(
                symbol,
                side,
                TimeInForce::IOC,
                quantity
            );
        }
    }

    auto end = chrono::high_resolution_clock::now();

    auto totalNanoseconds = chrono::duration_cast<chrono::nanoseconds>(end - start).count();

    double totalSeconds = totalNanoseconds / 1e9;
    double ordersPerSecond = TOTAL_ORDERS / totalSeconds;
    double averageLatencyNs = static_cast<double>(totalNanoseconds) / TOTAL_ORDERS;
    double averageLatencyUs = averageLatencyNs / 1000.0;

    cout << fixed << setprecision(3);

    cout << "\n========== BENCHMARK RESULT ==========\n";
    cout << "Orders processed: " << TOTAL_ORDERS << "\n";
    cout << "Total time: " << totalSeconds << " sec\n";
    cout << "Throughput: " << ordersPerSecond << " orders/sec\n";
    cout << "Average latency: " << averageLatencyNs << " ns/order\n";
    cout << "Average latency: " << averageLatencyUs << " us/order\n";
    cout << "Trades executed: " << engine.getTotalTradesExecuted() << "\n";
    cout << "Orders submitted internally: " << engine.getTotalOrdersSubmitted() << "\n";
    cout << "Rejected orders: " << engine.getTotalOrdersRejected() << "\n";
    cout << "======================================\n";

    return 0;
}