#ifndef _LF_SPACESAVING_H
#define _LF_SPACESAVING_H

#include "LF.h"
#include <iostream>

#define THRESHOLD_LF 10

template<uint32_t ss_capacity, int filter_memory_in_bytes>
class SpaceSavingWithLF
{
public:
	LogFilter<filter_memory_in_bytes, THRESHOLD_LF> lf;
	SpaceSaving<ss_capacity> ss;
	int colditem = 0;
	SpaceSavingWithLF()
	{
		lf.init_spa(&ss);
	}

	inline void insert(uint32_t item)
	{
		lf.insert(item);
	}

	inline void build(uint32_t * items, int n)
    {
        for (int i = 0; i < n; ++i) {
            lf.insert(items[i]);
        }
    }

    void get_top_k(uint32_t k, uint32_t items[])
    {
        ss.get_top_k(k, items);
    }

    void get_top_k_with_frequency(uint32_t k, vector<pair<uint32_t, uint32_t>> &result)
    {
        ss.get_top_k_with_frequency(k, result);
        for (int i = 0; i < k; ++i) {
            result[i].second += lf.query2();
        }
    }
};

#endif // _LFCUSKETCH_H