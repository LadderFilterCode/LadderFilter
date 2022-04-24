#ifndef _SF_CU_SKETCH_H
#define _SF_CU_SKETCH_H

#include "CUSketch.h"
#include "SF_SIMD.h"

template <uint64_t total_memory_in_bytes, int filter_memory_percent>
class CUSketchWithSF
{
public:
    LadderFilter sf;
    CUSketch<int((total_memory_in_bytes) * (100 - filter_memory_percent) / 100), 3> cu;
    int thres;

    CUSketchWithSF(int _bucket_num1, int _bucket_num2,
                   int _cols, int _key_len, int _counter_len,
                   int _thres1, int _thres2,
                   int rand_seed1, int rand_seed2)
    {
        thres = _thres1;
        sf = LadderFilter(_bucket_num1, _bucket_num2,
                          _cols, _key_len, _counter_len,
                          _thres1, _thres2,
                          rand_seed1, rand_seed2);
    }

    inline void insert(uint32_t item)
    {
        auto res = sf.insert(item);
        if (res >= thres)
            cu.insert(item, (res == thres) ? thres : 1);
    }

    inline void synchronize()
    {
    }

    inline void build(uint32_t *items, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            sf.insert(items[i]);
        }
    }

    inline int query(uint32_t item)
    {
        int ret = cu.query(item);
        return ret;
    }

    inline int batch_query(uint32_t items[], int n)
    {
        int ret = 0;
        for (int i = 0; i < n; ++i)
        {
            ret += CUSketchWithSF::query(items[i]);
        }

        return ret;
    }
};

#endif // _SFCUSKETCH_H