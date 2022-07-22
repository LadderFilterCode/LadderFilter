# LadderFilter: Filtering Infrequent Items with Small Memory and Time Overhead

## Introduction

Data stream processing is critical in streaming databases. Existing works pay a lot of attention to frequent items. To improve the accuracy for frequent items, existing solutions focus on accurately filtering infrequent items. While these solutions are effective, they keep track of all infrequent items and require multiple hash computations and memory accesses. This increases memory and time overhead. To reduce this overhead, we propose LadderFilter, which can discard infrequent items efficiently in terms of both memory and time. To achieve memory efficiency, LadderFilter discards (approximately) infrequent items using multiple LRU queues. To achieve time efficiency, we leverage SIMD instructions to implement LRU policy without timestamps. We apply LadderFilter to four types of sketches. Our experimental results show that LadderFilter improves the accuracy by up to 60.6×, and the throughput by up to 1.37×, and can maintain high accuracy with small memory usage. All related code is provided open-source at Github.



## File Description

*  `CUSketch/`: the source code of estimating item frequency.
*  `SpaceSaving/`: the source code of finding top-$k$ items.
*  `FlowRadar/`: the source code of finding heavy changes.
*  `WavingSketch/`: the source code of finding super-spreaders.



## Requirements

- cmake
- g++



## References

[1] Cold Filter: A Meta-Framework for Faster and More Accurate Stream Processing. https://github.com/zhouyangpkuer/ColdFilter.

[2] LogLog Filter: Filtering Cold Items within a Large Range over High Speed Data Streams https://github.com/ICDE2021/LogLogFilter.