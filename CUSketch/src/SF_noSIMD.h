#ifndef SF_H
#define SF_H

#include "BOBHash32.h"
#include "SPA.h"
#include <cstring>
#include <algorithm>

using namespace std;

uint32_t getFP(uint32_t key, int key_len)
{
    static BOBHash32 fpHash(100);
    return fpHash.run((const char *)&key, 4) % 0xFFFF + 1;
}

class Bucket
{
public:
    uint32_t *fp;
    uint32_t *counter;

    Bucket() {}

    Bucket(int cols)
    {
        fp = new uint32_t[cols];
        counter = new uint32_t[cols];
        memset(fp, 0, sizeof(uint32_t) * cols);
        memset(counter, 0, sizeof(uint32_t) * cols);
    }

    void permutation(int p) // permute the p-th item to the first
    {
        for (int i = p; i > 0; --i)
        {
            swap(fp[i], fp[i - 1]);
            swap(counter[i], counter[i - 1]);
        }
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
    int cols, key_len, counter_len;
    int thres2;

    LadderFilter() {}
    LadderFilter(int _bucket_num1, int _bucket_num2,
                 int _cols, int _key_len, int _counter_len,
                 int _thres1, int _thres2,
                 int rand_seed1, int rand_seed2)
        : bucket_num1(_bucket_num1), bucket_num2(_bucket_num2),
          cols(_cols), key_len(_key_len), counter_len(_counter_len),
          thres2(_thres2)
    {
        Bucket1 = new Bucket[bucket_num1];
        for (int i = 0; i < bucket_num1; ++i)
            Bucket1[i] = Bucket(_cols);
        Bucket2 = new Bucket[bucket_num2];
        for (int i = 0; i < bucket_num2; ++i)
            Bucket2[i] = Bucket(_cols);
        l1Hash = new BOBHash32(rand_seed1);
        l2Hash = new BOBHash32(rand_seed2);
    }

    int insert(uint32_t key, int count = 1) // return dequeued item
    {
        auto keyfp = getFP(key, key_len);

        /* if the item is in B1 */
        auto &B1 = Bucket1[l1Hash->run((const char *)&key, 4) % bucket_num1];
        for (int i = 0; i < cols; ++i)
            if (B1.fp[i] == keyfp)
            {
                B1.permutation(i);
                B1.counter[0] = min(B1.counter[0] + count, (1u << counter_len) - 1);
                return B1.counter[0];
            }
        
        /* if the item is in B2 */
        auto &B2 = Bucket2[l2Hash->run((const char *)&key, 4) % bucket_num2];
        for (int i = 0; i < cols; ++i)
            if (B2.fp[i] == keyfp)
            {
                B2.permutation(i);
                B2.counter[0] = min(B2.counter[0] + count, (1u << counter_len) - 1);
                return B2.counter[0];
            }
        

        /* if B1 is not full */
        for (int i = 0; i < cols; ++i)
            if (B1.fp[i] == 0)
            {
                B1.permutation(i);
                B1.fp[0] = keyfp;
                B1.counter[0] = count;
                return B1.counter[0];
            }

        /* dequeue the LRU item in B1, if is promising item, insert to B2 */
        if (B1.counter[cols - 1] >= thres2)
        {
            /* if Bucket2 is not full*/
            bool isFull = true;
            for (int i = 0; i < cols; ++i)
                if (B2.fp[i] == 0)
                {
                    B2.permutation(i);
                    B2.fp[0] = B1.fp[cols - 1];
                    B2.counter[0] = B1.counter[cols - 1];
                    isFull = false;
                    break;
                }

            /* dequeue the LRU item in B2 */
            if (isFull)
            {
                B2.permutation(cols - 1);
                B2.fp[0] = B1.fp[cols - 1];
                B2.counter[0] = B1.counter[cols - 1];
            }
        }

        /* insert the item to the empty cell */
        B1.permutation(cols - 1);
        B1.fp[0] = keyfp;
        B1.counter[0] = count;
        return B1.counter[0];
    }
};

#endif
