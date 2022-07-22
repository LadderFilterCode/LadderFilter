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

#include "CUSketch.h"
#include "SF_CUSketch.h"
#include "SC_CUSketch.h"
#include "LF_CUSketch.h"

// Bob hash
#include "BOBHash32.h"
#include "CUSketch.h"
#include "SPA.h"
// SIMD 
#include <immintrin.h>

using namespace std;

#define MAX_INSERT_PACKAGE 40000000
#define percent 10

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];
uint32_t query_data[MAX_INSERT_PACKAGE];
FILE* SS_AAE;
FILE* SS_ARE;
FILE* SS_TPT;

int thres1=18;
int thres2=2;


// load data
int load_data() {
    const char *filename="your dataset direction";
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

    return ret;
}

template<uint32_t meminkb>
void demo_cu(int packet_num)
{
    printf("\nExp for frequency estimation:\n");
    printf("\tAllocate %dKB memory for each algorithm\n",meminkb);

    // fprintf(SS_AAE,"%d,",meminkb);
    // fprintf(SS_ARE,"%d,",meminkb);

    auto cu = new CUSketch<meminkb * 1024, 3>();
    auto sf_cu = new CUSketchWithSF<meminkb * 1024, percent>((meminkb * 1024*0.01*percent / (8 * 3)) * 0.99, (meminkb * 1024*0.01*percent  / (8 * 3)) * 0.01,
                                          8, 16, 8,
                                          thres1, thres2,
                                          750, 800);
    auto sc_cu = new CUSketchWithSC<meminkb * 1024, 90, 16>();
    auto lf_cu = new CUSketchWithLF<int(0.9 * 1024 * meminkb), int(0.1 * 1024 * meminkb)>();

    timespec dtime1, dtime2;
    long long delay = 0.0;

    double tot_ae;
    double tot_ar;

    // build and query for cu
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for (int i = 0; i < packet_num; ++i) {
        cu->insert(insert_data[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);

    tot_ae = 0;
    tot_ar = 0;
    
    for (auto itr: ground_truth) {
        int report_val = cu->query(itr.first);
        int old=tot_ae;
        tot_ae += abs(report_val - itr.second);
        tot_ar += (double)(abs(report_val - itr.second))/(double)itr.second;
        if(tot_ae<old)
        {
            printf("overflow!!!\n");
        }
    }

    printf("\tCU AAE: %lf ARE: %lf\n", double(tot_ae) / ground_truth.size(), double(tot_ar) / ground_truth.size());
    // fprintf(SS_AAE,"%lf,",double(tot_ae) / ground_truth.size());
    // fprintf(SS_ARE,"%lf,",double(tot_ar) / ground_truth.size());
    // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);

    // build and query for cu + LadderFilter
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for (int i = 0; i < packet_num; ++i) {
        sf_cu->insert(insert_data[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    sf_cu->synchronize(); // refresh SIMD buffer

    tot_ae = 0;
    tot_ar = 0;
    for (auto itr: ground_truth) {
        int report_val = sf_cu->query(itr.first);
        int old=tot_ae;
        tot_ae += abs(report_val - itr.second);
        tot_ar += (double)(abs(report_val - itr.second))/(double)itr.second;
        if(tot_ae<old)
        {
            printf("overflow!!!\n");
        }
    }

    printf("\tSF+CU AAE: %lf ARE: %lf\n", double(tot_ae) / ground_truth.size(), double(tot_ar) / ground_truth.size());
    // fprintf(SS_AAE,"%lf,",double(tot_ae) / ground_truth.size());
    // fprintf(SS_ARE,"%lf,",double(tot_ar) / ground_truth.size());
    // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);

    // build and query for cu + sc
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for (int i = 0; i < packet_num; ++i) {
        sc_cu->insert(insert_data[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    sc_cu->synchronize(); // refresh SIMD buffer

    tot_ae = 0;
    tot_ar = 0;
    for (auto itr: ground_truth) {
        int report_val = sc_cu->query(itr.first);
        tot_ae += abs(report_val - itr.second);
        tot_ar += (double)(abs(report_val - itr.second))/(double)itr.second;
    }

    printf("\tSC+CU AAE: %lf ARE: %lf\n", double(tot_ae) / ground_truth.size(), double(tot_ar) / ground_truth.size());
    // fprintf(SS_AAE,"%lf,",double(tot_ae) / ground_truth.size());
    // fprintf(SS_ARE,"%lf,",double(tot_ar) / ground_truth.size());
    // fprintf(SS_TPT,"%lf,",(double)(1000)*(double)(packet_num)/(double)delay);
    

    // build and query for cu + lf
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for (int i = 0; i < packet_num; ++i) {
        lf_cu->insert(insert_data[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);

    tot_ae = 0;
    tot_ar = 0;
    for (auto itr: ground_truth) {
        int report_val = lf_cu->query(itr.first);
        tot_ae += abs(report_val - itr.second);
        tot_ar += (double)(abs(report_val - itr.second))/(double)itr.second;
    }

    printf("\tLF+CU AAE: %lf ARE: %lf\n", double(tot_ae) / ground_truth.size(), double(tot_ar) / ground_truth.size());
    // fprintf(SS_AAE,"%lf\n",double(tot_ae) / ground_truth.size());
    // fprintf(SS_ARE,"%lf\n",double(tot_ar) / ground_truth.size());
    // fprintf(SS_TPT,"%lf\n",(double)(1000)*(double)(packet_num)/(double)delay);
}

int main(){

    // SS_AAE=fopen("CU-mem-AAE_LF.csv","w");
    // SS_ARE=fopen("CU-mem-ARE_LF.csv","w");
    // SS_TPT=fopen("CU-mem-throughput.csv","w");

    int packet_num = load_data();
    demo_cu<200>(packet_num);

    // fclose(SS_AAE);
    // fclose(SS_ARE);
    // fclose(SS_TPT);

    return 0;
}
