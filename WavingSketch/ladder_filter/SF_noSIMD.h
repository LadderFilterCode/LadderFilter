/*
 * SlidingFilter with 3 layers
 * 2022年3月19日17:24:47
 * Note: every bucket has the same depth(rows)
 */
#ifndef SF_H
#define SF_H

#include "../BOBHash32.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cassert>

#define COUNTER_MASK(i) ((i == 32) ? 0xFFFFFFFF : (1u << i) - 1)
using namespace std;

uint32_t getFP(uint32_t key, int key_len)
{
    static BOBHash32 fpHash(100);
    if (key_len == 32)
        return fpHash.run((const char *)&key, 4);
    return fpHash.run((const char *)&key, 4) & ((1 << key_len) - 1); // TODO
}

class Bucket
{
public:
    uint32_t *fp = nullptr;
    uint32_t *counter = nullptr;
    uint32_t cols;

    Bucket() {}

    Bucket(int _cols) : cols(_cols)
    {
        if (cols == 0)
        {
            return;
        }
        fp = new uint32_t[cols];
        counter = new uint32_t[cols];
        for (int i = 0; i < cols; i++)
        {
            fp[i] = 0;
            counter[i] = 0;
        }
    }

    void permutation(int p) // permute the p-th item to the first
    {
        for (int i = p; i > 0; --i)
        {
            swap(fp[i], fp[i - 1]);
            swap(counter[i], counter[i - 1]);
        }
    }

    void print()
    {
        for (int i = 0; i < cols; i++)
        {
            cout << '\t' << fp[i] << ' ' << counter[i] << endl;
        }
    }
};

class SlidingFilter
{
public:
    Bucket *Bucket1 = nullptr;
    Bucket *Bucket2 = nullptr;
    Bucket *Bucket3 = nullptr;
    BOBHash32 *l1Hash = nullptr;
    BOBHash32 *l2Hash = nullptr;
    BOBHash32 *l3Hash = nullptr;

    int bucket_num1, bucket_num2, bucket_num3;
    int cols, key_len, counter_len;
    int thres1, thres2;

    SlidingFilter(int _bucket_num1, int _bucket_num2, int _bucket_num3,
                  int _cols, int _key_len, int _counter_len,
                  int _thres1, int _thres2,
                  int rand_seed1 = 100, int rand_seed2 = 102, int rand_seed3 = 2567)
        : bucket_num1(_bucket_num1), bucket_num2(_bucket_num2), bucket_num3(_bucket_num3),
          cols(_cols), key_len(_key_len), counter_len(_counter_len),
          thres1(_thres1), thres2(_thres2)
    {
        if (bucket_num1 != 0)
        {
            Bucket1 = new Bucket[bucket_num1];
            for (int i = 0; i < bucket_num1; ++i)
                Bucket1[i] = Bucket(cols);
            l1Hash = new BOBHash32(rand_seed1);
        }
        else
        {
            cout << "Number of buckets in layer 1 can not be 0." << endl;
            return;
        }
        if (bucket_num2 != 0)
        {
            Bucket2 = new Bucket[bucket_num2];
            for (int i = 0; i < bucket_num2; ++i)
                Bucket2[i] = Bucket(cols);
            l2Hash = new BOBHash32(rand_seed2);
        }

        if (bucket_num3 != 0)
        {
            Bucket3 = new Bucket[bucket_num3];
            for (int i = 0; i < bucket_num3; i++)
                Bucket3[i] = Bucket(cols);
            l3Hash = new BOBHash32(rand_seed2);
        }

        // cout << "Build Successful.." << endl;
    }

    void printInfo()
    {
        printf("Basic Info of SlidingFilter:\n");
        auto mem_total = (bucket_num1 + bucket_num2 + bucket_num3) * cols * (key_len + counter_len) / 1024.0 / 8;
        printf("\tThe number of buckets is %d %d %d\n", bucket_num1, bucket_num2, bucket_num3);
        printf("\tThe number of rows of a bucket is %d\n", cols);
        printf("\tThe length of key is %d bits, the length of counter is %d bits\n", key_len, counter_len);
        printf("\tThe total memory cost is: %.3f KB\n", mem_total);
    }

    int insert(uint32_t key, int count = 1) // return evict item
    {
        auto fp = getFP(key, key_len);
        // auto fp = key;

        // check queues from top to bottom
        // if the item is in Layer3
        if (bucket_num3 != 0)
        {
            auto &B3 = Bucket3[l3Hash->run((const char *)&key, 4) % bucket_num3];
            for (int i = 0; i < cols; i++)
            {
                if (B3.fp[i] == fp)
                {
                    B3.permutation(i);
                    B3.counter[0] = min(B3.counter[0] + count, COUNTER_MASK(counter_len));
                    return B3.counter[0];
                }
            }
        }

        // if the item is in Layer2
        if (bucket_num2 != 0)
        {
            auto &B2 = Bucket2[l2Hash->run((const char *)&key, 4) % bucket_num2];
            for (int i = 0; i < cols; ++i)
            {
                if (B2.fp[i] == fp)
                {
                    B2.permutation(i);
                    B2.counter[0] = min(B2.counter[0] + count, COUNTER_MASK(counter_len));
                    return B2.counter[0];
                }
            }
        }

        // if the item is in Layer1
        auto &B1 = Bucket1[l1Hash->run((const char *)&key, 4) % bucket_num1];
        for (int i = 0; i < cols; ++i)
        {
            if (B1.fp[i] == fp)
            {
                B1.permutation(i);
                B1.counter[0] = min(B1.counter[0] + count, COUNTER_MASK(counter_len));
                return B1.counter[0];
            }
        }

        // item is new to every layer
        // if Layer1 is not full
        for (int i = 0; i < cols; ++i)
            if (B1.fp[i] == 0)
            {
                B1.permutation(i);
                B1.fp[0] = fp;
                B1.counter[0] = count;
                return B1.counter[0];
            }

        // evict the LRU item in Layer1, if is a potential hot item, insert it to Layer2
        if (bucket_num2 != 0 && B1.counter[cols - 1] >= thres1)
        {
            /* if Bucket2 is not full*/
            // cout << "Potential from 1 to 2" << endl;
            bool isFull = true;
            auto &B2 = Bucket2[l2Hash->run((const char *)&key, 4) % bucket_num2];

            for (int i = 0; i < cols; ++i)
            {
                if (B2.fp[i] == 0)
                {
                    B2.permutation(i);
                    B2.fp[0] = B1.fp[cols - 1];
                    B2.counter[0] = B1.counter[cols - 1];
                    isFull = false;
                    break;
                }
            }

            // layer2 is full, evict the LRU item in Bucket2
            if (isFull)
            {
                if (bucket_num3 != 0 && B2.counter[cols - 1] > thres2)
                {
                    // cout << "Potential from 2 to 3" << endl;
                    bool isFull_l3 = true;
                    auto &B3 = Bucket3[l3Hash->run((const char *)&key, 4) % bucket_num3];
                    for (int i = 0; i < cols; i++)
                    {
                        if (B3.fp[i] == 0)
                        {
                            isFull_l3 = false;
                            B3.permutation(i);
                            B3.fp[0] = B2.fp[cols - 1];
                            B3.counter[0] = B2.counter[cols - 1];
                            break;
                        }
                    }
                }
                B2.permutation(cols - 1);
                B2.fp[0] = B1.fp[cols - 1];
                B2.counter[0] = B1.counter[cols - 1];
            }
        }

        /* insert the item to the evicted place */
        B1.permutation(cols - 1);
        B1.fp[0] = fp;
        B1.counter[0] = count;
        return B1.counter[0];
    }
};

#endif
