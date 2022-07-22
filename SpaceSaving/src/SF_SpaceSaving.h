#ifndef SF_SPACESAVING_H
#define SF_SPACESAVING_H

#include "../../LadderFilter/SF_SIMD.h"
#include <algorithm>

template<uint32_t ss_capacity>
class SF_SpaceSaving
{
public:
    LadderFilter sf;
    int thres;
    SpaceSaving<ss_capacity> ss;

    SF_SpaceSaving(int _bucket_num1, int _bucket_num2,
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

    inline void build(uint32_t * items, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            if (ss.ifRecorded(items[i]))
                continue;
            auto res = sf.insert(items[i]);
            if (res >= thres)
                ss.insert(items[i], (res == thres) ? thres : 1);
        }
    }

    void get_top_k(uint32_t k, uint32_t items[])
    {
        ss.get_top_k(k, items);
    }

    void get_top_k_with_frequency(uint32_t k, vector<pair<uint32_t, uint32_t>> & result)
    {
        ss.get_top_k_with_frequency(k, result);
    }
};


#endif
