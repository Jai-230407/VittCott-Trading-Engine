# VittCott Trading Engine

A C++17 limit-order-book trading engine that simulates core exchange-style order matching with market/limit orders, GTC/IOC/FOK execution, price-time priority, cancellation/modification, risk checks, market-depth snapshots, CSV replay, logging, tests, and performance benchmarking.

## Overview

VittCott Trading Engine is a simulated trading engine built in C++17. It implements a limit order book using STL data structures and supports realistic order-matching behavior such as partial fills, FIFO matching within price levels, time-in-force conditions, order cancellation, order modification, risk validation, trade persistence, and market-depth display.

The project was built to understand the internal design of electronic trading systems, matching engines, and performance-oriented backend systems.

## Features

* Market and limit order support
* GTC, IOC, and FOK time-in-force handling
* Price-time priority matching
* Partial and full fills
* FIFO matching within the same price level
* Order cancellation and modification
* Order status tracking
* Pre-trade risk validation
* Trade CSV persistence
* Asynchronous audit logging
* Market-depth snapshots
* CSV-based order replay system
* Custom C++ test suite
* Synthetic performance benchmarking

## Tech Stack

* C++17
* STL containers
* Multithreading
* File I/O
* Command-line interface
* MSYS2 MinGW-w64 GCC

## Core Data Structures

The order book is implemented using:

```cpp
std::map<double, OrderList, std::greater<double>> buyLevels;
std::map<double, OrderList> sellLevels;
std::unordered_map<int, OrderLocation> orderLocations;
```

Design choices:

* `std::map` keeps price levels sorted.
* Buy levels are sorted in descending order, so the best bid is available at `buyLevels.begin()`.
* Sell levels are sorted in ascending order, so the best ask is available at `sellLevels.begin()`.
* `std::list` is used at every price level to preserve FIFO order.
* `std::unordered_map` stores order locations for efficient cancellation using stored iterators.

## Supported Order Types

### Limit Order

Executes at the specified price or better. If unfilled and marked GTC, the remaining quantity rests in the order book.

### Market Order

Executes immediately against the best available opposite-side liquidity.

## Supported Time-in-Force Types

### GTC — Good Till Cancelled

The order remains in the book until it is filled or cancelled.

### IOC — Immediate Or Cancel

The order executes immediately as much as possible, and the remaining quantity is cancelled.

### FOK — Fill Or Kill

The order executes only if the full quantity can be filled immediately. Otherwise, it is rejected.

## Market Depth

The engine supports top-N market depth snapshots showing:

* Best bid
* Best ask
* Spread
* Top ask levels
* Top bid levels
* Aggregated quantity at each price level

Example:

```text
========== MARKET DEPTH ==========
Best Bid: 99
Best Ask: 100
Spread: 1

----- ASKS -----
Price   Quantity
100     10
101     20
102     30

----- BIDS -----
Price   Quantity
99      15
98      25
==================================
```

After a buy market IOC order of quantity 12:

```text
TRADE EXECUTED
Price: 100
Quantity: 10

TRADE EXECUTED
Price: 101
Quantity: 2
```

Updated depth:

```text
========== MARKET DEPTH ==========
Best Bid: 99
Best Ask: 101
Spread: 2

----- ASKS -----
Price   Quantity
101     18
102     30

----- BIDS -----
Price   Quantity
99      15
98      25
==================================
```

## CSV Replay System

The project includes a CSV replay system that can process simulated order streams from a file.

Supported replay actions:

* `NEW`
* `CANCEL`
* `MODIFY`
* `DEPTH`
* `BOOK`
* `STATUS`

Example `replay_orders.csv`:

```csv
action,symbol,side,orderType,tif,price,quantity,orderId,newPrice,newQuantity,levels
NEW,INFY,SELL,LIMIT,GTC,100,10,0,0,0,0
NEW,INFY,SELL,LIMIT,GTC,101,20,0,0,0,0
NEW,INFY,BUY,MARKET,IOC,0,12,0,0,0,0
DEPTH,INFY,BUY,LIMIT,GTC,0,0,0,0,0,5
```

## Risk Validation

The engine includes pre-trade risk checks for:

* Invalid symbol
* Invalid price
* Invalid quantity
* Excessive quantity
* Excessive price
* Excessive notional value

Orders that fail risk validation are rejected before entering the order book.

## Testing

A custom C++ test suite validates the core matching logic.

Test coverage includes:

* FIFO matching
* Price priority
* Partial fills
* Market orders
* Limit orders
* IOC behavior
* FOK behavior
* Cancellation
* Modification
* Risk rejection

Final test result:

```text
Tests passed: 60
Tests failed: 0
```

## Benchmark Results

A synthetic benchmark was run on 100,000 simulated orders with console output, audit logging, and CSV persistence disabled.

Result:

```text
Orders processed: 100000
Total time: 0.140 sec
Throughput: 714,015+ orders/sec
Average latency: ~1.4 us/order
Trades executed: 82,296
Rejected orders: 0
```

## Logging Benchmark

A separate benchmark compared performance with audit logging disabled and enabled.

```text
Logging OFF:
Throughput: 1,090,849 orders/sec
Average latency: 0.917 us/order

Async Logging ON:
Throughput: 93,819 orders/sec
Average latency: 10.659 us/order

Logging overhead: 1,062%
```

This showed that async logging still introduced overhead due to hot-path string formatting, timestamping, and mutex locking. A future improvement would be structured numeric event logging where the hot path pushes lightweight events and the logger thread formats them asynchronously.

## Build Instructions

### Compile Main CLI

```bash
set PATH=C:\msys64\ucrt64\bin;%PATH%

C:\msys64\ucrt64\bin\g++.exe -O2 -std=c++17 main.cpp OrderBook.cpp MatchingEngine.cpp TradeLogger.cpp Logger.cpp RiskManager.cpp -o main.exe -pthread
```

Run:

```bash
.\main.exe
```

### Compile Replay System

```bash
set PATH=C:\msys64\ucrt64\bin;%PATH%

C:\msys64\ucrt64\bin\g++.exe -O2 -std=c++17 replay_main.cpp OrderBook.cpp MatchingEngine.cpp TradeLogger.cpp Logger.cpp RiskManager.cpp -o replay.exe -pthread
```

Run:

```bash
.\replay.exe
```

### Compile Tests

```bash
set PATH=C:\msys64\ucrt64\bin;%PATH%

C:\msys64\ucrt64\bin\g++.exe -O2 -std=c++17 tests.cpp OrderBook.cpp RiskManager.cpp -o tests.exe
```

Run:

```bash
.\tests.exe
```

### Compile Benchmark

```bash
set PATH=C:\msys64\ucrt64\bin;%PATH%

C:\msys64\ucrt64\bin\g++.exe -O3 -DNDEBUG -std=c++17 benchmark_main.cpp OrderBook.cpp MatchingEngine.cpp TradeLogger.cpp Logger.cpp RiskManager.cpp -o benchmark.exe -pthread
```

Run:

```bash
.\benchmark.exe
```

## Example CLI Menu

```text
VittCott Trading Engine - v2.4

1. Place Buy Limit Order
2. Place Sell Limit Order
3. Place Buy Market Order
4. Place Sell Market Order
5. Show Order Book
6. Cancel Order
7. Modify Order
8. Query Order Status
9. Show Market Depth
10. Exit
```

## Key Learnings

* Designed a limit order book using sorted price levels and FIFO queues.
* Implemented price-time priority matching.
* Understood GTC, IOC, and FOK execution semantics.
* Implemented partial fills and order lifecycle transitions.
* Used stored iterators for efficient cancellation.
* Added benchmarking to measure throughput and latency.
* Identified logging overhead as a performance bottleneck.
* Built replay support for automated order-stream processing.

## Resume Summary

Built a C++17 limit-order-book trading engine with market/limit orders, GTC/IOC/FOK execution, partial fills, price-time priority matching, O(1) cancellation using stored iterators, risk validation, async logging, CSV replay, market-depth snapshots, 60 passing tests, and 714K+ simulated orders/sec benchmark throughput.
