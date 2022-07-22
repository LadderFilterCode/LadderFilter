#ifndef SF_H
#define SF_H

#include "BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <inttypes.h>
#include <immintrin.h>

using namespace std;

const int CELL_NUM = 8;

uint16_t getFP(uint32_t key)
{
    static BOBHash32 fpHash(100);
    return fpHash.run((const char *)&key, 4) % 0xFFFF + 1;
}

__m128i rearrange_index[] = {
    _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15),
    _mm_setr_epi8(10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15),
    _mm_setr_epi8(12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15),
    _mm_setr_epi8(14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)};
uint64_t low_mask[] = {
    0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
    0xFFFFFF0000000000, 0xFFFF000000000000, 0xFF00000000000000, 0x0000000000000000};
uint64_t high_mask[] = {
    0x0000000000000000, 0x00000000000000FF, 0x000000000000FFFF, 0x0000000000FFFFFF,
    0x00000000FFFFFFFF, 0x000000FFFFFFFFFF, 0x0000FFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF};

class Bucket
{
public:
    /* in implement, we merge the two adjacent buckets for alignment */
    uint16_t fp[CELL_NUM * 2];
    uint8_t counter[CELL_NUM * 2];

    Bucket()
    {
        memset(fp, 0, sizeof(fp));
        memset(fp, 0, sizeof(counter));
    }

    int find(uint16_t in_fp, int bid)
    {
        __m128i _in_fp = _mm_set1_epi16(in_fp);
        __m128i *_fp = (__m128i *)(bid == 0 ? fp : fp + CELL_NUM);

        uint32_t match = _mm_cmpeq_epi16_mask(_in_fp, _fp[0]);
        if (match)
            return _tzcnt_u32(match);
        return -1;
    }

    int find_empty(int bid)
    {
        return -1;
    }

    void rearrange(int p, int bid) // rearrange the p-th item to the first cell
    {
        __m128i *_fp = (__m128i *)(bid == 0 ? fp : fp + CELL_NUM);
        _fp[0] = _mm_shuffle_epi8(_fp[0], rearrange_index[p]);

        uint64_t *_counter = (uint64_t *)(bid == 0 ? counter : counter + CELL_NUM);
        _counter[0] = (_counter[0] & low_mask[p]) | ((_counter[0] & high_mask[p]) << 8) | ((_counter[0] >> p * 8) & 0xFF);
    }
};

class LadderFilter
{
public:
    Bucket *Bucket1;
    Bucket *Bucket2;
    BOBHash32 *l1Hash;
    BOBHash32 *l2Hash;
    int bucket_num1, bucket_num2;
    int thres2;

    LadderFilter() {}

    LadderFilter(int _bucket_num1, int _bucket_num2,
                 int _cols, int _key_len, int _counter_len, /* not available in SIMD version, fixed to 8, 16, 8 */
                 int _thres1, int _thres2,
                 int rand_seed1, int rand_seed2)
        : bucket_num1(_bucket_num1), bucket_num2(_bucket_num2),
          thres2(_thres2)
    {
        Bucket1 = new Bucket[(bucket_num1 + 1) / 2];
        Bucket2 = new Bucket[(bucket_num2 + 1) / 2];
        l1Hash = new BOBHash32(rand_seed1);
        l2Hash = new BOBHash32(rand_seed2);
    }

    void addCounter(uint8_t &counter, uint8_t add_val)
    {
        counter = counter + add_val >= counter ? counter + add_val : 0xFFFF;
    }

    int insert(uint32_t key, int count = 1)
    {
        auto fp = getFP(key);

        /* if the item is in B1 */
        int id1 = l1Hash->run((const char *)&key, 4) % bucket_num1;
        auto &B1 = Bucket1[id1 >> 1];
        int p1 = B1.find(fp, id1 & 1);
        if (p1 != -1)
        {
            B1.rearrange(p1, id1 & 1);
            addCounter(B1.counter[(id1 & 1) == 0 ? 0 : CELL_NUM], count);
            return B1.counter[(id1 & 1) == 0 ? 0 : CELL_NUM];
        }

        /* if the item is in B2 */
        int id2 = l2Hash->run((const char *)&key, 4) % bucket_num2;
        auto &B2 = Bucket2[id2 >> 1];
        int p2 = B2.find(fp, id2 & 1);
        if (p2 != -1)
        {
            B2.rearrange(p2, id2 & 1);
            addCounter(B2.counter[(id2 & 1) == 0 ? 0 : CELL_NUM], count);
            return B2.counter[(id2 & 1) == 0 ? 0 : CELL_NUM];
        }

        /* if B1 is full, dequeue the LRU item;
           if the LRU item is a promising item, insert it to B2 */
        if (p1 == -1 && B1.counter[(id1 & 1) == 0 ? CELL_NUM - 1 : CELL_NUM * 2 - 1] >= thres2)
        {
            /* if B2 is full, dequeue the LRU item in B2 directly */
            p2 = B2.find(0, id2 & 1);
            B2.rearrange(p2 != -1 ? p2 : CELL_NUM - 1, id2 & 1);
            B2.fp[(id2 & 1) == 0 ? 0 : CELL_NUM] = B1.fp[(id1 & 1) == 0 ? CELL_NUM - 1 : CELL_NUM * 2 - 1];
            B2.counter[(id2 & 1) == 0 ? 0 : CELL_NUM] = B1.counter[(id1 & 1) == 0 ? CELL_NUM - 1 : CELL_NUM * 2 - 1];
        }

        /* insert the arriving item to the empty cell */
        B1.rearrange(p1 != -1 ? p1 : CELL_NUM - 1, id1 & 1);
        B1.fp[(id1 & 1) == 0 ? 0 : CELL_NUM] = fp;
        B1.counter[(id1 & 1) == 0 ? 0 : CELL_NUM] = count;
        return B1.counter[(id1 & 1) == 0 ? 0 : CELL_NUM];
    }

    int query(uint32_t key)
    {
        auto fp = getFP(key);

        /* if the item is in B1 */
        int id1 = l1Hash->run((const char *)&key, 4) % bucket_num1;
        auto &B1 = Bucket1[id1 >> 1];
        int p1 = B1.find(fp, id1 & 1);
        if (p1 != -1)
        {
            return B1.counter[(id1 & 1) == 0 ? p1 : p1 + CELL_NUM];
        }

        /* if the item is in B2 */
        int id2 = l2Hash->run((const char *)&key, 4) % bucket_num2;
        auto &B2 = Bucket2[id2 >> 1];
        int p2 = B2.find(fp, id2 & 1);
        if (p2 != -1)
        {
            return B2.counter[(id2 & 1) == 0 ? p2 : p2 + CELL_NUM];
        }

        return 0;
    }
};

#endif
