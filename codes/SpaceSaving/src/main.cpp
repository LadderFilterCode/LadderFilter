#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <list>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <cassert>
#include <cstring>

#include "SpaceSaving.h"
#include "SF_SpaceSaving.h"
#include "SC_SpaceSaving.h"
#include "LF_SpaceSaving.h"

// Bob hash
#include "BOBHash32.h"
#include "SPA.h"
// SIMD 
#include <immintrin.h>
#define UNIX
#ifdef UNIX
#include <x86intrin.h>
#else
#include <intrin.h>
#endif

using namespace std;

#define MAX_INSERT_PACKAGE 33000000

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];
uint32_t query_data[MAX_INSERT_PACKAGE];
FILE* SS_ACC;
FILE* SS_AAE;
FILE* SS_TPT;



// load data
int load_data(const char *filename) {
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    ground_truth.clear();

    char ip[13];
    char ts[8];
    int ret = 0;
    while (1) {
        size_t rsize;
        rsize = fread(ip, 1, 13, pf);
        if(rsize != 13) break;
        rsize = fread(ts, 1, 8, pf);
        if(rsize != 8) break;

        uint32_t key = *(uint32_t *) ip;
        insert_data[ret] = key;
        ground_truth[key]++;
        ret++;
        if (ret == MAX_INSERT_PACKAGE){
            cout << "MAX_INSERT_PACKAGE" << endl;
            break;
        }
    }
    fclose(pf);

    int i = 0;
    for (auto itr: ground_truth) {
        query_data[i++] = itr.first;
    }

    printf("Total stream size = %d\n", ret);
    printf("Distinct item number = %ld\n", ground_truth.size());

    int max_freq = 0;
    for (auto itr: ground_truth) {
        max_freq = std::max(max_freq, itr.second);
    }
    printf("Max frequency = %d\n", max_freq);

    vector<int> s;
    for (auto itr: ground_truth) {
        s.push_back(itr.second);
    }
    sort(s.begin(), s.end());
    cout << s[s.size() - 1] << " " << s[s.size() - 1000] << endl;

    return ret;
}


pair<double, double> ss_compare_value_with_ground_truth(uint32_t k, vector<pair<uint32_t, uint32_t>> & result)
{
    // prepare top-k ground truth
    vector<pair<uint32_t, int>> gt_ordered;
    for (auto itr: ground_truth) {
        gt_ordered.emplace_back(itr);
    }
    std::sort(gt_ordered.begin(), gt_ordered.end(), [](const std::pair<uint32_t, int> &left, const std::pair<uint32_t, int> &right) {
        return left.second > right.second;
    });
    set<uint32_t> set_gt;
    int i = 0;
    int th;
    for (auto itr: gt_ordered) {
        if (i >= k && itr.second < th) {
            break;
        }
        set_gt.insert(itr.first);
        i++;
        if (i == k) {
            th = itr.second;
        }
    }

    double aae = 0;

    set<uint32_t> set_rp;
    unordered_map<uint32_t, uint32_t> mp_rp;

    int lp = 0;
    for (lp = 0; lp < k; ++lp) {
        set_rp.insert(result[lp].first);
        mp_rp[result[lp].first] = result[lp].second;
    }

    vector<uint32_t> intersection(k);
    auto end_itr = std::set_intersection(
            set_gt.begin(), set_gt.end(),
            set_rp.begin(), set_rp.end(),
            intersection.begin()
    );

    for (auto itr = intersection.begin(); itr != end_itr; ++itr) {
        int diff = int(mp_rp[*itr]) - int(ground_truth[*itr]);
        //cout << int(mp_rp[*itr]) << " " << int(ground_truth[*itr]) << endl;
	aae += abs(diff);
    }

    int num = end_itr - intersection.begin();
    num = num > 0 ? num : 1;

    return make_pair(double(num) / k, aae / num);
}

template<uint32_t mem_in_byte, uint32_t topk, uint32_t L2Thres>
void demo_ss(int packet_num)
{
    printf("\nExp for top-k query:\n");
    printf("\ttest top %d\n", topk);
    printf("\tmem_in_byte %d\n",mem_in_byte);
    auto sf_ss = new SF_SpaceSaving<(int)(topk*1.5)>((mem_in_byte / (8 * 3)) * 0.99, (mem_in_byte / (8 * 3)) * 0.01,
                                          8, 16, 8,
                                          250, 50,
                                          750, 800);
    auto cf_ss = new SC_SpaceSaving<(int)(topk*1.5), mem_in_byte, L2Thres, 16>();
    auto lf_ss = new SpaceSavingWithLF<(int)(topk*1.5), mem_in_byte>();
    auto ss = new SpaceSaving<(int)(topk*1.5)+mem_in_byte/100>();
    
    
    timespec dtime1, dtime2;
    long long delay = 0.0;
    
    int k = topk;
    vector<pair<uint32_t, uint32_t>> result(k);
    
    printf("-----------------------------------------------\n");
    
    vector< pair<double, double> > res;
    
    // LadderFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    sf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        sf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tLadder Filter \n");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf,",ret.first);
        // fprintf(SS_AAE,"%lf,",ret.second);
        // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    }
    
    // ColdFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    cf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        cf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tCold Filter:\n");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf,",ret.first);
        // fprintf(SS_AAE,"%lf,",ret.second);
        // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    }
    
    // LogLogFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    lf_ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        lf_ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\LogLog Filter:\n");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf,",ret.first);
        // fprintf(SS_AAE,"%lf,",ret.second);
        // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    }

    // SpaceSaving
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    ss->build(insert_data, packet_num);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    
    {
        ss->get_top_k_with_frequency(k, result);
        auto ret = ss_compare_value_with_ground_truth(k, result);
        res.push_back(ret);
        printf("\tSpaceSaving:\n");
        printf("\t  accuracy %lf, AAE %lf, throughput %lfMops\n", ret.first, ret.second, (double)(1000)*(double)(packet_num)/(double)delay);
        // fprintf(SS_ACC,"%lf\n",ret.first);
        // fprintf(SS_AAE,"%lf\n",ret.second);
        // fprintf(SS_TPT,"%lf\n",(double)(1000)*(double)(packet_num)/(double)delay);
    }

    return;
}

int main(){
    // SS_ACC=fopen("topk1000-a-accuracy.csv","w");
    // SS_AAE=fopen("topk1000-a-AAE.csv","w");
    // SS_TPT=fopen("topk1000-a-throughput.csv","w");

    int packet_num = load_data("your dataset direction");
    demo_ss<20*1024, 1000, 2000>(packet_num);

    // fclose(SS_ACC);
    // fclose(SS_AAE);
    // fclose(SS_TPT);

    return 0;
}
