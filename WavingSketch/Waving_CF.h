#ifndef WAVING_CF
#define WAVING_CF

#include <bitset>
#include <unistd.h>
#include "./abstract.h"
#include "./bitset.h"
#include "cold_filter/SC.h"
#include <set>

#define data_type Data
#define count_type int

set<uint32_t> reachWaving;

template <uint32_t slot_num, uint32_t counter_num>
class WavingSketch_CF : public Abstract
{
public:
    struct Bucket
    {
        Data items[slot_num];
        int counters[slot_num];
        int16_t incast[counter_num];

        void Insert(const Data &item, uint32_t seed_s, uint32_t seed_incast)
        {
            uint32_t choice = item.Hash(seed_s) & 1;
            uint32_t whichcast = item.Hash(seed_incast) % counter_num;
            int min_num = INT_MAX;
            uint32_t min_pos = -1;

            for (uint32_t i = 0; i < slot_num; ++i)
            {
                if (counters[i] == 0)
                {
                    items[i] = item;
                    counters[i] = -1;
                    return;
                }
                else if (items[i] == item)
                {
                    if (counters[i] < 0)
                        counters[i]--;
                    else
                    {
                        counters[i]++;
                        incast[whichcast] += COUNT[choice];
                    }
                    return;
                }

                int counter_val = std::abs(counters[i]);
                if (counter_val < min_num)
                {
                    min_num = counter_val;
                    min_pos = i;
                }
            }

            if (incast[whichcast] * COUNT[choice] >= int(min_num))
            {
                if (counters[min_pos] < 0)
                {
                    uint32_t min_choice = items[min_pos].Hash(seed_s) & 1;
                    incast[items[min_pos].Hash(seed_incast) % counter_num] -= COUNT[min_choice] * counters[min_pos];
                }
                items[min_pos] = item;
                counters[min_pos] = min_num + 1;
            }
            incast[whichcast] += COUNT[choice];
        }

        int Query(const Data &item, uint32_t seed_s, uint32_t seed_incast)
        {
            uint32_t choice = item.Hash(seed_s) & 1;
            uint32_t whichcast = item.Hash(seed_incast) % counter_num;
            int retv = 0;

            for (uint32_t i = 0; i < slot_num; ++i)
            {
                if (items[i] == item)
                {
                    return std::abs(counters[i]);
                }
            }
            return retv;
        }
    };

    WavingSketch_CF(int mem, int _HIT, uint32_t BF_LENGTH, uint32_t _HASH_NUM = 3)
        : BUCKET_NUM(mem * 0.3 / sizeof(Bucket)), HIT(_HIT), HASH_NUM(_HASH_NUM), LENGTH(BF_LENGTH * 8)
    {
        cout << "BUCKET_SIZE: " << sizeof(Bucket) << endl;
        cout << "HIT: " << HIT << endl;
        name = (char *)"Waving_Filter_CF";
        record = 0;
        bitset = new BitSet(LENGTH);
        buckets = new Bucket[BUCKET_NUM];

        // first parameter is memory
        CF = new StreamClassifier<0, 0, 263>(mem * 0.7, 40);
        CF->print_basic_info();
        memset(buckets, 0, BUCKET_NUM * sizeof(Bucket));

        aae = are = pr = cr = 0;
        sep = "\t";
        seed_choice = std::clock();
        sleep(1);
        seed_incast = std::clock();
        sleep(1);
        seed_s = std::clock();
    }
    ~WavingSketch_CF()
    {
        delete[] buckets;
        delete CF;
    }

    void Init(const Data &from, const Data &to)
    {
        Stream stream(from, to);

        bool init = true;

        auto key = *((uint32_t *)from.str);

        for (uint i = 0; i < HASH_NUM; ++i)
        {
            uint position = stream.Hash(i) % LENGTH;
            if (!bitset->Get(position))
            {
                init = false;
                bitset->Set(position);
            }
        }

        if (!init)
        {
            distinct_num++;

            auto key = *((uint32_t *)from.str);

            auto res = CF->insert_SC(key, 1);

            if (res != 0)
            {
                filter_num++;
                // reachWaving.insert(key);
                uint32_t bucket_pos = from.Hash(seed_choice) % BUCKET_NUM;
                for (int kkk = 0; kkk < res; kkk++)
                {
                    buckets[bucket_pos].Insert(from, seed_s, seed_incast);
                }
            }
        }
    }

    inline int Query(const Data &item)
    {
        int res = CF->query(*(uint32_t *)(item.str));
        if (res != CF->thres)
            return res;
        uint32_t bucket_pos = item.Hash(seed_choice) % BUCKET_NUM;
        return buckets[bucket_pos].Query(item, seed_s, seed_incast) + CF->thres;
    }

    void Check(HashMap mp)
    {
        cout << "\t" << reachWaving.size() << endl;
        cout << "Pass_BF: " << this->pass_bf << endl;
        cout << "Distinct Number: " << this->distinct_num << endl;
        cout << "Hit Number: " << this->hit_num << endl;
        cout << "Filter Number: " << this->filter_num << endl;
        HashMap::iterator it;
        int value = 0, all = 0, hit = 0, size = 0;
        for (it = mp.begin(); it != mp.end(); ++it)
        {
            value = Query(it->first);
            if (it->second > HIT)
            {
                all++;
                if (value > HIT)
                {
                    hit += 1;
                    aae += abs(it->second - value);
                    are += abs(it->second - value) / (double)it->second;
                }
            }
            if (value > HIT)
                size += 1;
        }
        aae /= hit;
        are /= hit;
        cr = hit / (double)all;
        pr = hit / (double)size;
        cout << "total_counter: " << BUCKET_NUM * slot_num << endl;
        cout << "hit: " << hit << endl;
        cout << "size: " << size << endl;
        cout << "all: " << all << endl;
    }

public:
    Bucket *buckets;
    uint32_t BUCKET_NUM;
    uint record;
    BitSet *bitset;
    const uint HASH_NUM;
    const uint LENGTH;

    uint32_t seed_choice;
    uint32_t seed_s;
    uint32_t seed_incast;

    uint32_t pass_bf = 0;
    uint32_t distinct_num = 0;
    uint32_t filter_num = 0;
    uint32_t hit_num = 0;

    int HIT;
    StreamClassifier<0, 0, 263> *CF;
};

#endif // WAVING_CF
