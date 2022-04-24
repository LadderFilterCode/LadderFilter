#ifndef _LF_H
#define _LF_H

#include "BOBHash32.h"
#include "SPA.h"
#include <cstring>
#include <algorithm>
#include <immintrin.h>
#include <stdexcept>
#include <cmath>
#include <math.h>
#include <iostream>
#include <chrono>
#include <random>
#include <bitset>
using namespace std;

#define MAX_HASH_NUM 4
#define NUM_BITS 4
#define PHI 0.77351

template<int memory_in_bytes, int threshold>
class LogFilter
{
	static constexpr int d = 3;
	static constexpr int w = int(memory_in_bytes * 8 / NUM_BITS);
	uint32_t f = 0;
	uint32_t timestamp = 0;

	uint32_t L[w];

	SPA * spa;

	BOBHash32 * IndexHash[d];
	BOBHash32 * ValueHash[d];

	void clear_data()
	{
		memset(L, 0, sizeof(L));
	}
public:
	LogFilter()
	{
		for (int i = 0; i < d; i++) {
			IndexHash[i] = new BOBHash32(500 + i);
			ValueHash[i] = new BOBHash32(1000 + i);
		}
		clear_data();
		spa = NULL;
	}

	~LogFilter()
	{
		for (int i = 0; i < d; i++) {
			delete IndexHash[i];
			delete ValueHash[i];
		}
	}

	void init_array_all()
	{
		memset(L, 0, sizeof(short int) * w);
	}

	void init_spa(SPA * _spa)
	{
		spa = _spa;
	}

	void insert(uint32_t key)
	{
		int value[MAX_HASH_NUM];
		int index[MAX_HASH_NUM];

		int v = 1 << 30;
		for (int i = 0; i < d; i++) {
			index[i] = (IndexHash[i]->run((const char *)&key, 4)) % w;
			value[i] = L[index[i]];
			v = std::min(value[i], v);
		}

		if (v < threshold) {
			f++;
			for (int i = 0; i < d; i++) {
				uint32_t seeds = key & timestamp;
				uint32_t random_num = (ValueHash[i]->run((const char *)&seeds, 12));
				int temp = 0;
				bitset<32> bittemp(random_num);
				for (int i = 31; i >= 0; i--) {
					if (bittemp[i] == 0) {
						temp++;
					}
					else
						break;
				}
				uint32_t hash_value = (temp > threshold) ? threshold : temp;
				uint32_t beta = (L[index[i]] > hash_value) ? L[index[i]] : hash_value;
				L[index[i]] = beta;
			}
		}
		else
		{
			spa->insert(key, 1);
		}
		timestamp++;
	}

	int probe(uint32_t key)
	{
		int v = 1 << 30;
		for (int i = 0; i < d; i++) {
			int index = (IndexHash[i]->run((const char *)&key, 4)) % w;
			int value = L[index];
			v = std::min(value, v);
		}
		if (v < threshold) {
			return 1;
		}
		else
		{
			return 0;
		}
	}

	double query1(uint32_t key)
	{
		double frequency = 0.0;
		double sum_value = 0.0;
		for (int i = 0; i < d; i++) {
			int index = (IndexHash[i]->run((const char *)&key, 4)) % w;
			sum_value = sum_value + L[index];
		}
		sum_value = sum_value / d;
		frequency = ((double)(w * d) / (w - d)) * (pow(2, sum_value) / PHI / d - (double)f / w);
		if (frequency > 0) {
			return frequency;
		}
		else {
			return (pow(2, sum_value) / PHI / d);
		}
	}

	double query2()
	{
		double frequency = 0.0;
		frequency = ((double)(w * d) / (w - d)) * (pow(2, threshold) / PHI / d - (double)f / w);
		return frequency;
	}

	void statistics()
	{
		int maxregister = 0;
		int minregister = w;
		int zeroregisters = 0;
		for (int i = 0; i < w; i++) {
			if (L[i] == 0) {
				zeroregisters++;
			}
			else {
				maxregister = (maxregister > i) ? maxregister : i;
				minregister = (minregister > i) ? i : minregister;
			}
		}

		printf("the index range: %d~%d\n", minregister, maxregister);
		printf("the number of registers: %d\n", w);
		printf("the number of zero registers: %d\n", zeroregisters);
	}
};

#endif//_LF_H
