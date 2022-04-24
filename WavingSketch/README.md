# Ladder Filter for Super Spreaders

## File Description

- `Waving_XX.h`: implementation of Waving Sketch with different kinds of filters.
    * `Waving_Pure.h`: Original Waving Skethch without any filters.
    * `Waving.h`: Waving Sketch with Ladder Filter.
    * `Waving_CF.h`: Waving Sketch with Cold Filter.
    * `Waving_LLF.h`: Waving Sketch with Loglog Filter.
- `./ladder_filter`: implementation of Ladder Filter.
    * `SF_SIMD.h` is ladder filter with SIMD.
    * `SF_noSIMD` is ladder filter without SIMD.

- `./cold_filter`: implementation of Cold Filter.

- `./loglog_filter`: implementation of Log Log Filter.

- `main_XX.cpp`: examples of Waving Sketch with different kinds of filters.
    - `main.cpp`: includes Waving Sketch and **Waving Sketch with Ours**
    - `main_CF.cpp`: Waving Sketch with Cold Filter.
    - `main_LLF.cpp`: Waving Sketch with Loglog Filter.

## How to Run

Taking `main.cpp` as an example, compile the source file with command
`g++ main.cpp -o main.out -march=native -O3` and then `./main.out` to run it.

The default dataset is `./demo_CAIDA.dat`. Note that the dataset consists only 1250000 packets and is only used as an example and we conduct experiments with dataset with ~250 million packets.