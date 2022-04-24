#ifndef _LF_CU_SKETCH_H
#define _LF_CU_SKETCH_H

#include "CUSketch.h"
#include "LF.h"
#include <iostream>

#define THRESHOLD_LF 10

template<int sketch_memory_in_bytes, int filter_memory_in_bytes>
class CUSketchWithLF
{
public:
	LogFilter<filter_memory_in_bytes, THRESHOLD_LF> lf;
	CUSketch<sketch_memory_in_bytes, 3> cu;
	int colditem = 0;
	CUSketchWithLF()
	{
		lf.init_spa(&cu);
	}

	inline void insert(uint32_t item)
	{
		lf.insert(item);
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
			ret = lf.query2() + (double)cu.query(item);
		}			
		return ret;
	}

	inline void statistics()
	{
		lf.statistics();
		printf("the number of items embedded in the filter: %d\n", colditem);
	}
};

#endif // _LFCUSKETCH_H