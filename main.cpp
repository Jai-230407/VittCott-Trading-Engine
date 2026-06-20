#include "MatchingEngine.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>

int readInt(const std::string& prompt) {
    int value;

    while (true) {
        std::cout << prompt;

        if (std::cin >> value) {
            return value;
        }

        std::cout << "Invalid input. Please enter an integer.\n";

        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
    }
}

double readDouble(const std::string& prompt) {
    double value;

    while (true) {
        std::cout << prompt;

        if (std::cin >> value) {
            return value;
        }

        std::cout << "Invalid input. Please enter a number.\n";

        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
    }
}

std::string readSymbol(const std::string& prompt) {
    std::string symbol;

    std::cout << prompt;
    std::cin >> symbol;

    std::transform(
        symbol.begin(),
        symbol.end(),
        symbol.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        }
    );

    return symbol;
}

TimeInForce readTIF() {
    while (true) {
        std::cout << "Choose Time-In-Force:\n";
        std::cout << "1. GTC\n";
        std::cout << "2. IOC\n";
        std::cout << "3. FOK\n";

        int choice = readInt("Enter TIF choice: ");

        if (choice == 1) {
            return TimeInForce::GTC;
        } else if (choice == 2) {
            return TimeInForce::IOC;
        } else if (choice == 3) {
            return TimeInForce::FOK;
        }

        std::cout << "Invalid TIF. Try again.\n";
    }
}

void printMenu() {
    std::cout << "\nChoose option:\n";
    std::cout << "1. Place Buy Limit Order\n";
    std::cout << "2. Place Sell Limit Order\n";
    std::cout << "3. Place Buy Market Order\n";
    std::cout << "4. Place Sell Market Order\n";
    std::cout << "5. Show Order Book\n";
    std::cout << "6. Cancel Order\n";
    std::cout << "7. Modify Order\n";
    std::cout << "8. Query Order Status\n";
    std::cout << "9. Show Market Depth\n";
    std::cout << "10. Exit\n";
}

int main() {
    MatchingEngine engine;

    std::cout << "=====================================\n";
    std::cout << "   VittCott Trading Engine - v2.4\n";
    std::cout << "=====================================\n";
    std::cout << " map+list order book | GTC/IOC/FOK | market depth\n";

    while (true) {
        printMenu();

        int choice = readInt("Enter choice: ");

        if (choice == 1) {
            std::string symbol = readSymbol("Enter symbol: ");
            double price = readDouble("Enter price: ");
            int quantity = readInt("Enter quantity: ");
            TimeInForce timeInForce = readTIF();

            engine.placeLimitOrder(
                symbol,
                Side::BUY,
                timeInForce,
                price,
                quantity
            );
        } else if (choice == 2) {
            std::string symbol = readSymbol("Enter symbol: ");
            double price = readDouble("Enter price: ");
            int quantity = readInt("Enter quantity: ");
            TimeInForce timeInForce = readTIF();

            engine.placeLimitOrder(
                symbol,
                Side::SELL,
                timeInForce,
                price,
                quantity
            );
        } else if (choice == 3) {
            std::string symbol = readSymbol("Enter symbol: ");
            int quantity = readInt("Enter quantity: ");
            TimeInForce timeInForce = readTIF();

            engine.placeMarketOrder(
                symbol,
                Side::BUY,
                timeInForce,
                quantity
            );
        } else if (choice == 4) {
            std::string symbol = readSymbol("Enter symbol: ");
            int quantity = readInt("Enter quantity: ");
            TimeInForce timeInForce = readTIF();

            engine.placeMarketOrder(
                symbol,
                Side::SELL,
                timeInForce,
                quantity
            );
        } else if (choice == 5) {
            std::string symbol = readSymbol("Enter symbol: ");
            engine.printBook(symbol);
        } else if (choice == 6) {
            std::string symbol = readSymbol("Enter symbol: ");
            int orderId = readInt("Enter order ID to cancel: ");

            engine.cancelOrder(
                symbol,
                orderId
            );
        } else if (choice == 7) {
            std::string symbol = readSymbol("Enter symbol: ");
            int orderId = readInt("Enter order ID to modify: ");
            double newPrice = readDouble("Enter new price: ");
            int newQuantity = readInt("Enter new quantity: ");

            engine.modifyOrder(
                symbol,
                orderId,
                newPrice,
                newQuantity
            );
        } else if (choice == 8) {
            int orderId = readInt("Enter order ID: ");
            engine.showOrderStatus(orderId);
        } else if (choice == 9) {
            std::string symbol = readSymbol("Enter symbol: ");
            int levels = readInt("Enter number of levels: ");

            engine.showMarketDepth(
                symbol,
                levels
            );
        } else if (choice == 10) {
            std::cout << "Exiting Trading Engine...\n";
            break;
        } else {
            std::cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}