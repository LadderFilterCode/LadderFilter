#ifndef STREAMCLASSIFIER_LF_IBLT_H
#define STREAMCLASSIFIER_LF_IBLT_H

#include "InsertableIblt.h"
#include "LF.h"
#include <iostream>

// #define THRESHOLD_LF 3

template<int iblt_size_in_bytes, int filter_memory_in_bytes, int THRESHOLD_LF>
class LF_InsertableIBLT: public VirtualIBLT
{
public:
    LogFilter<filter_memory_in_bytes, THRESHOLD_LF> lf;
    InsertableIBLT<iblt_size_in_bytes> iblt;
    int colditem = 0;

    LF_InsertableIBLT() {
        lf.init_spa(&iblt);
    }

    inline void build(uint32_t * items, int n)
    {
        for (int i = 0; i < n; ++i) {
            lf.insert(items[i]);
        }
    }

    void dump(unordered_map<uint32_t, int> &result) {
        iblt.dump(result);

        // for (auto itr: result) {
        //     int remain = query(itr.first);
        //     printf("r:%d", remain);
        //     result[itr.first] += remain;
        // }
    }

    inline double query(uint32_t item)
	{
		double ret = 0.0;
		int temp = lf.probe(item);
		if (temp == 1) {
			ret = lf.query1(item);
			colditem++;
		}
		else if (temp == 0) {
			ret = lf.query2();
		}
		return ret;
	}

    int approximate_query(uint32_t key) {
        int val_lf = query(key);
        int val_iblt = iblt.approximate_query(key);

        if (val_iblt == 0) {
            return val_lf;
        } else if (val_lf != 15 + THRESHOLD_LF) {
            return val_lf;
        } else {
            return -1;
        }
    }

	inline void statistics()
	{
		lf.statistics();
		printf("the number of items embedded in the filter: %d\n", colditem);
	}

};

#endif //STREAMCLASSIFIER_LF_IBLT_H
