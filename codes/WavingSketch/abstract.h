#ifndef ABSTRACT_H
#define ABSTRACT_H

#include "data.h"
#include <iostream>
#include <iomanip>

class Abstract
{
public:
    string name;
    double aae;
    double are;
    double pr;
    double cr;
    string sep;
    int HIT;

    virtual void Init(const Data &from, const Data &to) = 0;
    virtual void Check(HashMap mp) = 0;
    virtual ~Abstract() {}
    void rename(int slot_num, int counter_num)
    {
        char *tp = new char[20];
        std::sprintf(tp, "%s<%d,%d>", name.c_str(), slot_num, counter_num);
        name = tp;
    }
    void print_are(ofstream &out, int num)
    {
        out << name << "," << num << "," << are << endl;
        cout << name << "," << num << "," << are << endl;
    }
    void print_aae(ofstream &out, int num)
    {
        out << name << "," << num << "," << aae << endl;
        cout << name << "," << num << "," << aae << endl;
    }
    void print_pr(ofstream &out, int num)
    {
        out << name << "," << num << "," << pr << endl;
        cout << name << "," << num << "," << pr << endl;
    }
    void print_cr(ofstream &out, int num)
    {
        out << name << "," << num << "," << cr << endl;
        cout << name << "," << num << "," << cr << endl;
    }
    void print_f1(ofstream &out, int num)
    {
        cout << name << sep << 2 * pr * cr / (pr + cr) << endl;
    }
    void print_info(ofstream &out)
    {
        out.precision(6);
        out << setfill('0');
        out << name << sep << 2 * pr * cr / (pr + cr) << sep << pr << sep << cr << sep << aae << sep << are << endl;
    }
};

#endif // ABSTRACT_H
