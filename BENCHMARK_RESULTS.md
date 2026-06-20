# VittCott Trading Engine Benchmark Results

## Environment
- OS: Windows 10
- Compiler: MinGW g++
- Optimization: -O3 -DNDEBUG
- Order Book: std::map price levels + std::list FIFO queues
- Logging: disabled during benchmark
- Trade persistence: disabled during benchmark
- Console output: disabled during benchmark

## Result
- Orders processed: 100,000
- Total time: 0.185 sec
- Throughput: 541,480 orders/sec
- Average latency: 1,846.790 ns/order
- Average latency: 1.847 us/order
- Trades executed: 82,296
- Orders submitted internally: 110,000
- Rejected orders: 0

## Notes
The benchmark measures simulated order-processing performance with console output, audit logging, and CSV trade persistence disabled to isolate matching-engine throughput.