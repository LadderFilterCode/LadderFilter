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
#include <fstream>

#include "InsertableIblt.h"
#include "SC_InsertableIBLT.h"
#include "SF_InsertableIBLT.h"
#include "LF_InsertableIBLT.h"

// Bob hash
#include "BOBHash32.h"
#include "SPA.h"
// SIMD 
#include <immintrin.h>

#define FR_IBLT 2800  //raw flowradar memory

#define sffr_iblt 900     //sc+sf+log iblt memory
#define sffr_sf 100    //sc+sf+log filter memory

#define thres_insert 9    //sf insert_thres
#define thres_evict 3   //sf evict

#define log 1  //execute log or not
#define log_thr 3   //log thres

#define sc 1   //execute sc or not 
const int threshold = 1000;                 //heacy change threshold

using namespace std;
#define MAX_INSERT_PACKAGE 35000000

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];
uint32_t query_data[MAX_INSERT_PACKAGE];

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


typedef unordered_map<uint32_t, int> ItemSet;
ItemSet fr_dump_diff(VirtualIBLT * algo1, VirtualIBLT * algo2)
{
    ItemSet a, b, c;
    algo1->dump(a);
    algo2->dump(b);
    bool approximate = true;
    // try another method to dump diff
    for (auto itr: a) {
        if (b.find(itr.first) != b.end()){
            c[itr.first] = a[itr.first] - b[itr.first];
        } else if (approximate) {
            int appr_b = algo2->approximate_query(itr.first);
            if (appr_b != -1) {
                c[itr.first] = a[itr.first] - appr_b;
            }
        }
    }

    for (auto itr: b) {
        if (c.find(itr.first) != c.end()) {
            continue;
        }
        if (a.find(itr.first) != a.end()) {
            c[itr.first] = a[itr.first] - b[itr.first];
            cout << "error" << endl;
        } else if (approximate){
            int appr_a = algo1->approximate_query(itr.first);
            if (appr_a != -1) {
                c[itr.first] = appr_a - b[itr.first];
            }
        }
    }

    return c;
}

void fr_get_heavy_changer(ItemSet & diff, int threshold, set<uint32_t>& detected)
{
    detected.clear();
    for (auto itr: diff) {
        if (abs(itr.second) >= threshold) {
            detected.insert(itr.first);
        }
    }
}

int fr_get_gt_diff(int package_num, ItemSet & gt_diff)
{
    int i;
    gt_diff.clear();
    for (i = 0; i < package_num / 2; i++) {
        gt_diff[insert_data[i]]++;
    }
    for (; i < package_num; i++) {
        gt_diff[insert_data[i]]--;
    }

    int ret = 0;
    for (auto itr: gt_diff) {
        ret += abs(itr.second);
    }

    return ret;
}

void demo_flowradar(int packet_num /*, ofstream &oFile*/)
{
    timespec dtime1, dtime2;
    long long delay = 0.0;
    // long long throughput = 0.0;
    printf("\nHeavy change detection %d:\n", threshold);

    ItemSet detected_diff, gt_diff;
    set<uint32_t> detected_changer, gt_changer;

    fr_get_gt_diff(packet_num, gt_diff);
    fr_get_heavy_changer(gt_diff, threshold, gt_changer);
    printf("dete_groundtruth: %d", gt_changer.size());

    int num_intersection;
    double precision, recall, f1;
    vector<uint32_t> intersection(ground_truth.size());


    #ifdef sffr_iblt
    #ifdef sffr_sf
    #ifdef sc
    // FlowRadar with SC          iblt_size_in_bytes, sc_memory_in_bytes,  sc_thres,  bucket_num---------------
    auto * sc_fr_a = new SC_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, 16>();
    auto * sc_fr_b = new SC_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, 16>();

    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    sc_fr_a->build(insert_data, packet_num / 2);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);

    sc_fr_b->build(insert_data + packet_num / 2, packet_num / 2);
    detected_diff = fr_dump_diff(sc_fr_a, sc_fr_b);
    fr_get_heavy_changer(detected_diff, threshold, detected_changer);
    auto end_itr2 = std::set_intersection(
            gt_changer.begin(), gt_changer.end(),
            detected_changer.begin(), detected_changer.end(),
            intersection.begin()
    );
    num_intersection = end_itr2 - intersection.begin();
    printf("dete_sc: %d", detected_changer.size());
    precision = detected_changer.size() ? double(num_intersection) / detected_changer.size() : (gt_changer.size() ? 0 : 1);
    recall = gt_changer.size() ? double(num_intersection) / gt_changer.size() : 1;
    f1 = (2 * precision * recall) / (precision + recall);

    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    auto throughput2 = (double)(1000)*(double)(packet_num/2)/(double)(delay);
    printf("\tFlowRadar with SC: %d KB FR + %d KB SC\n", sffr_iblt, sffr_sf);
    printf("\t  precision: %lf, recall: %lf, f1-score: %lf, throughput: %lfMops\n", precision, recall, f1, throughput2);
    // oFile << precision <<"," << recall <<"," << f1 << ","<< sffr_iblt <<","<< sffr_sf<<","<<throughput2<<",SC"<<endl;
    #endif

    #ifdef thres_insert
    #ifdef thres_evict
    // FlowRadar with SF                  iblt_size_in_bytes, sf_memory_in_bytes, -------------------
    //                                                                  cols,   key, counter, thres_insert, thres_evict
    auto * sf_fr_a = new SF_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, 16,       16, 8,         thres_insert, thres_evict>();
    auto * sf_fr_b = new SF_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, 16,       16, 8,         thres_insert, thres_evict>();
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    sf_fr_a->build(insert_data, packet_num / 2);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    sf_fr_b->build(insert_data + packet_num / 2, packet_num / 2);
    detected_diff = fr_dump_diff(sf_fr_a, sf_fr_b);
    fr_get_heavy_changer(detected_diff, threshold, detected_changer);
    auto end_itr3 = std::set_intersection(
            gt_changer.begin(), gt_changer.end(),
            detected_changer.begin(), detected_changer.end(),
            intersection.begin()
    );
    num_intersection = end_itr3 - intersection.begin();
    printf("dete_sf: %d", detected_changer.size());
    precision = detected_changer.size() ? double(num_intersection) / detected_changer.size() : (gt_changer.size() ? 0 : 1);
    recall = gt_changer.size() ? double(num_intersection) / gt_changer.size() : 1;
    f1 = (2 * precision * recall) / (precision + recall);

    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    auto throughput3 = (double)(1000)*(double)(packet_num/2)/(double)(delay);
    printf("\tFlowRadar with LadderFilter: %d KB FR + %d KB LadderFilter\n", sffr_iblt, sffr_sf);
    printf("\t  precision: %lf, recall: %lf, f1-score: %lf, throughput: %lfMops\n", precision, recall, f1, throughput3);
    // oFile << precision <<"," << recall <<"," << f1 << ","<< sffr_iblt <<","<<sffr_sf<<","<< thres_evict<<","<<thres_insert<<","<<throughput3<<",SF,"<<endl;
    #endif
    #endif

    #ifdef log
    #ifdef log_thr
    auto * log_fr_a = new LF_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, log_thr>();
    auto * log_fr_b = new LF_InsertableIBLT<sffr_iblt*1024, sffr_sf*1024, log_thr>();
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    log_fr_a->build(insert_data, packet_num / 2);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    log_fr_b->build(insert_data + packet_num / 2, packet_num / 2);
    detected_diff = fr_dump_diff(log_fr_a, log_fr_b);
    fr_get_heavy_changer(detected_diff, threshold, detected_changer);
    auto end_itr4 = std::set_intersection(
            gt_changer.begin(), gt_changer.end(),
            detected_changer.begin(), detected_changer.end(),
            intersection.begin()
    );
    num_intersection = end_itr4 - intersection.begin();
    printf("dete_log: %d", detected_changer.size());
    precision = detected_changer.size() ? double(num_intersection) / detected_changer.size() : (gt_changer.size() ? 0 : 1);
    recall = gt_changer.size() ? double(num_intersection) / gt_changer.size() : 1;
    f1 = (2 * precision * recall) / (precision + recall);

    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    auto throughput4 = (double)(1000)*(double)(packet_num/2)/(double)(delay);
    printf("\tFlowRadar with Log: %d KB FR + %d KB Log\n", sffr_iblt, sffr_sf);
    printf("\t  precision: %lf, recall: %lf, f1-score: %lf, throughput: %lfMops\n", precision, recall, f1, throughput4);
    // oFile << precision <<"," << recall <<"," << f1 << ","<< sffr_iblt <<","<<sffr_sf<<","<<throughput4<<",Log,"<<log_thr<<endl;
    #endif
    #endif


    #endif
    #endif

    #ifdef FR_IBLT
    // Raw FlowRadar    ---------------------------------------------------------------------------
    auto * fr_a = new InsertableIBLT<FR_IBLT * 1024>();
    auto * fr_b = new InsertableIBLT<FR_IBLT * 1024>();
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    fr_a->build(insert_data, packet_num / 2);
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    fr_b->build(insert_data + packet_num / 2, packet_num / 2);

    detected_diff = fr_dump_diff(fr_a, fr_b);
    fr_get_heavy_changer(detected_diff, threshold, detected_changer);
    auto end_itr1 = std::set_intersection(
            gt_changer.begin(), gt_changer.end(),
            detected_changer.begin(), detected_changer.end(),
            intersection.begin()
    );
    num_intersection = end_itr1 - intersection.begin();
    printf("dete_fr: %d", detected_changer.size());
    precision = detected_changer.size() ? double(num_intersection) / detected_changer.size() : (gt_changer.size() ? 0 : 1);
    recall = gt_changer.size() ? double(num_intersection) / gt_changer.size() : 1;
    f1 = (2 * precision * recall) / (precision + recall);

    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    auto throughput1 = (double)(1000)*(double)(packet_num/2)/(double)(delay);
    printf("\tFlowRadar: %d KB\n", FR_IBLT);
    printf("\t  precision: %lf, recall: %lf, f1-score: %lf, throughput: %lfMops\n", precision, recall, f1, throughput1);
    // oFile <<","<< precision <<"," << recall <<"," << f1 << ","<< FR_IBLT <<","<<throughput1 << ",RFR"<<endl;
    #endif
}



int main(){

    // ofstream oFile;
	// oFile.open("sheet4.csv", ios::app);
	// if (!oFile) return 0;

    int packet_num = load_data();
    demo_flowradar(packet_num /*, oFile*/);
    
    return 0;
}

