#ifndef STREAMCLASSIFIER_SF_IBLT_H
#define STREAMCLASSIFIER_SF_IBLT_H

#include "InsertableIblt.h"
// #include "SF_3level.h"
#include "SF_SIMD.h"
// #include "SF.h"

template<uint64_t iblt_size_in_bytes, uint64_t sf_memory_in_bytes,
                int cols, int key_len, int counter_len,
                int thres1, int thres2>

class SF_InsertableIBLT: public VirtualIBLT
{
public:                           //bucket1_memory               bucket2_memory
    LadderFilter sf = LadderFilter(sf_memory_in_bytes*0.99, sf_memory_in_bytes*0.01, cols, key_len, counter_len, thres2);
    // LadderFilter sf = LadderFilter(sf_memory_in_bytes*0.99, sf_memory_in_bytes*0.01, thres2);
    InsertableIBLT<iblt_size_in_bytes> iblt;

    // SF_InsertableIBLT(int _bucket_num1, int _bucket_num2,    sf_memory_in_bytes*0
    //               int _cols, int _key_len, int _counter_len,
    //               int _thres1, int _thres2)
    // {
    //     thres = _thres1;
    //     sf = LadderFilter(_bucket_num1, _bucket_num2, _cols, _key_len, _counter_len,
    //                        _thres1, _thres2);
    // }

    inline void build(uint32_t * items, int n)
    {
        for (int i = 0; i < n; ++i) {
            if (iblt.isRecorded(items[i]))
                continue;
            auto res = sf.insert(items[i], 1);
            if (res > thres1)
                iblt.insert(items[i], (res == thres1) ? thres1 : 1);
        }
    }

    void dump(unordered_map<uint32_t, int> &result) {
        iblt.dump(result);

        for (auto itr: result) {
            int remain = sf.query(itr.first);
            result[itr.first] += remain;
        }
    }

    //?
    int approximate_query(uint32_t key) {
        int val_sf = sf.query(key);
        int val_iblt = iblt.approximate_query(key);

        if (val_iblt == 0) {
            return val_sf;
        } else if (val_sf != thres1) {
            return val_sf;
        } else {
            return -1;
        }
    }
};



#endif