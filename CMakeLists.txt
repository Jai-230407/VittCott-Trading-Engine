cmake_minimum_required(VERSION 3.10)

project(VittCottTradingEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    VittCottTradingEngine
    main.cpp
    OrderBook.cpp
    MatchingEngine.cpp
    TradeLogger.cpp
    Logger.cpp
    RiskManager.cpp
)