#include "MatchingEngine.h"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <string>

using namespace std;

struct SimulatedOrder {
    bool isMarketOrder;
    Side side;
    TimeInForce tif;
    double price;
    int quantity;
};

struct BenchmarkResult {
    string name;
    double totalTimeSeconds;
    double throughput;
    double avgLatencyNs;
    double avgLatencyUs;
    long long tradesExecuted;
    long long ordersSubmitted;
    long long rejectedOrders;
};

void seedInitialLiquidity(MatchingEngine& engine) {
    string symbol = "INFY";

    for (int i = 0; i < 5000; i++) {
        double sellPrice = 1000.0 + (i % 50);
        engine.placeLimitOrder(symbol, Side::SELL, TimeInForce::GTC, sellPrice, 100);
    }

    for (int i = 0; i < 5000; i++) {
        double buyPrice = 999.0 - (i % 50);
        engine.placeLimitOrder(symbol, Side::BUY, TimeInForce::GTC, buyPrice, 100);
    }
}

vector<SimulatedOrder> generateWorkload(int totalOrders) {
    vector<SimulatedOrder> workload;
    workload.reserve(totalOrders);

    mt19937 rng(42);

    uniform_int_distribution<int> sideDist(0, 1);
    uniform_int_distribution<int> qtyDist(1, 100);
    uniform_int_distribution<int> priceOffsetDist(-50, 50);
    uniform_int_distribution<int> orderTypeDist(0, 9);

    for (int i = 0; i < totalOrders; i++) {
        int orderKind = orderTypeDist(rng);

        SimulatedOrder order;

        order.side = sideDist(rng) == 0 ? Side::BUY : Side::SELL;
        order.quantity = qtyDist(rng);

        double basePrice = 1000.0;
        order.price = basePrice + priceOffsetDist(rng);

        if (orderKind <= 7) {
            order.isMarketOrder = false;
            order.tif = TimeInForce::GTC;
        } else if (orderKind == 8) {
            order.isMarketOrder = false;
            order.tif = TimeInForce::IOC;
        } else {
            order.isMarketOrder = true;
            order.tif = TimeInForce::IOC;
        }

        workload.push_back(order);
    }

    return workload;
}

BenchmarkResult runBenchmark(
    const string& benchmarkName,
    const vector<SimulatedOrder>& workload,
    bool enableAuditLogging
) {
    MatchingEngine engine(
        false,              // console output OFF
        enableAuditLogging, // audit logging ON/OFF
        false               // trade CSV persistence OFF
    );

    engine.reserveCapacity(workload.size() + 20000);

    seedInitialLiquidity(engine);

    string symbol = "INFY";

    auto start = chrono::high_resolution_clock::now();

    for (const SimulatedOrder& order : workload) {
        if (order.isMarketOrder) {
            engine.placeMarketOrder(
                symbol,
                order.side,
                order.tif,
                order.quantity
            );
        } else {
            engine.placeLimitOrder(
                symbol,
                order.side,
                order.tif,
                order.price,
                order.quantity
            );
        }
    }

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = end - start;

    BenchmarkResult result;

    result.name = benchmarkName;
    result.totalTimeSeconds = elapsed.count();
    result.throughput = workload.size() / result.totalTimeSeconds;
    result.avgLatencyNs = (result.totalTimeSeconds * 1e9) / workload.size();
    result.avgLatencyUs = result.avgLatencyNs / 1000.0;

    result.tradesExecuted = engine.getTotalTradesExecuted();
    result.ordersSubmitted = engine.getTotalOrdersSubmitted();
    result.rejectedOrders = engine.getTotalOrdersRejected();

    return result;
}

void printResult(const BenchmarkResult& result) {
    cout << "\n========== " << result.name << " ==========\n";
    cout << fixed << setprecision(3);

    cout << "Total time: " << result.totalTimeSeconds << " sec\n";
    cout << "Throughput: " << result.throughput << " orders/sec\n";
    cout << "Average latency: " << result.avgLatencyNs << " ns/order\n";
    cout << "Average latency: " << result.avgLatencyUs << " us/order\n";
    cout << "Trades executed: " << result.tradesExecuted << "\n";
    cout << "Orders submitted internally: " << result.ordersSubmitted << "\n";
    cout << "Rejected orders: " << result.rejectedOrders << "\n";
    cout << "=====================================\n";
}

int main() {
    const int TOTAL_ORDERS = 100000;

    cout << "=====================================\n";
    cout << " Logging Impact Benchmark\n";
    cout << "=====================================\n";

    cout << "Generating workload...\n";
    vector<SimulatedOrder> workload = generateWorkload(TOTAL_ORDERS);

    cout << "Running benchmark with logging OFF...\n";
    BenchmarkResult noLogging = runBenchmark(
        "LOGGING OFF",
        workload,
        false
    );

    cout << "Running benchmark with ASYNC logging ON...\n";
    BenchmarkResult asyncLogging = runBenchmark(
        "ASYNC LOGGING ON",
        workload,
        true
    );

    printResult(noLogging);
    printResult(asyncLogging);

    double slowdownPercent =
        ((asyncLogging.totalTimeSeconds - noLogging.totalTimeSeconds)
        / noLogging.totalTimeSeconds) * 100.0;

    cout << "\n========== COMPARISON ==========\n";
    cout << fixed << setprecision(2);
    cout << "Logging overhead: " << slowdownPercent << "%\n";

    if (slowdownPercent < 10.0) {
        cout << "Result: Async logger adds low hot-path overhead.\n";
    } else {
        cout << "Result: Logging overhead is noticeable but still benchmarked.\n";
    }

    cout << "================================\n";

    return 0;
}