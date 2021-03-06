/********************************************************************
 * Ultra-narrow filters for VLBI software correlation
 * (C) 2010 Jan Wagner MPIfR
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *********************************************************************/

#include "filterchain.h"
#include "filters.h"
#include "filterhelpers.h"
#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
using std::cout;
using std::endl;
using std::flush;

#define VERIFY1 0 // verification test with input file
#define VERIFY2 0 // verification test with fixed DC-level '1.0'
#define BENCH   0 // benchmarking test

#if 1  // short filter size test
static const long Nch = 32;
static const long NI = 512; //1024
#else  // wide filter size test
static const long Nch = 1024;
static const long NI = 32;
#endif
static const long Nsamps = 80000;

////////////////////////////////////////////////////////////////////
// Timing auto-class
////////////////////////////////////////////////////////////////////
class Timing {
   public:
     Timing() : _msg(std::string("")),_samples(8000*1024.0) 
        { start(); }
     Timing(const char* msg, double samples) : _msg(std::string(msg)),_samples(samples) 
        { start(); }
     ~Timing() { 
         double t = stop();
         double msps = _samples*1e-6 / t;
         cout << _msg << ": " << (1e6*t) << " usec  "
              << msps << " Ms/s " << endl << flush; 
     }
   public:
     void start() { gettimeofday(&_tstart, NULL); }
     double stop() { 
         double dsec;
         gettimeofday(&_tstop, NULL);
         dsec = (_tstop.tv_sec - _tstart.tv_sec);
         dsec += 1e-6*(_tstop.tv_usec - _tstart.tv_usec);
         return dsec;
     }
   private:
     std::string _msg;
     double _samples;
     struct timeval _tstart;
     struct timeval _tstop;
};


////////////////////////////////////////////////////////////////////
// Result comparison
////////////////////////////////////////////////////////////////////
void cout_precision_init() {
    //cout.setf(std::ios::scientific,std::ios::floatfield);
    cout.precision(10);
}
bool quite_equal(const double a, const double b) {
    double m = std::min(std::abs(a)+0.5, std::abs(b)+0.5);
    return (std::abs(a-b)<(m*1e-9));
}
bool quite_equal(const Ipp32fc a, const Ipp32fc b) {
    return (quite_equal((double)a.re,(double)b.re) && quite_equal((double)a.im,(double)b.im));
}

void compare_to_ref(const Ipp32fc act, const Ipp32fc ref)
{
    double e_re = std::abs(act.re - ref.re);
    double e_im = std::abs(act.im - ref.im);
    cout << "Final filter output   : {" << act.re << ", " << act.im << "}" << endl;
    cout << "Expected filter output: {" << ref.re << ", " << ref.im << "}" << endl;
    cout << "Error                 : {e=" << e_re   << ", e=" << e_im << "}" << endl;
    if (quite_equal(act,ref))
        cout << "PASS" << endl;
    else
        cout << "FAIL" << endl;
}

////////////////////////////////////////////////////////////////////
// Input data generators
////////////////////////////////////////////////////////////////////
Ipp32fc* load_datafile(void)
{
    std::string fn("pulse_32fc.raw");
    std::ifstream ifile(fn.c_str());
    Ipp32fc* singlechannel = ippsMalloc_32fc(Nsamps*sizeof(Ipp32fc));
    ifile.read((char*)singlechannel, Nsamps*sizeof(Ipp32fc));
    cout << "Read " << (ifile.gcount()/sizeof(Ipp32fc)) 
         << " complex samples from " << fn << endl;
    return singlechannel;
}

Ipp32fc* load_constvec(const Ipp32fc val)
{
    Ipp32fc* fullchannels = ippsMalloc_32fc(Nch);
    ippsSet_32fc(val, fullchannels, Nch);
    return fullchannels;
}

////////////////////////////////////////////////////////////////////
// Helper class test
////////////////////////////////////////////////////////////////////
void test_helperclass()
{
    std::string fn("filter_test_commentfile.txt");
    cout << endl << "---- Test Helper .coeff parser using file " << fn << endl;
    std::vector<double> vec;
    Helpers::parse_numerics_file(vec, fn.c_str());
    if (vec.size()<1) {
        cout << "Parse failed or file is empty" << endl;
    }
    for (size_t i=0; i<vec.size(); i++) { 
        cout << " " << vec.at(i); 
    }
    cout << endl;
}

////////////////////////////////////////////////////////////////////
// Filter factory test
////////////////////////////////////////////////////////////////////
void test_filterfactory()
{
    const int NI2 = 10000;
    cout << endl << "---- Test filter factory using Filter::getFilter(FLT_AVERAGING)" << endl;
    Ipp32fc* tvec = ippsMalloc_32fc(Nch);
    Filter*  f = Filter::getFilter(FLT_AVERAGING);
    f->init(0.01, Nch);
    f->clear();
    Ipp32fc c = {1, 0.5};
    Ipp32fc ref = {c.re*NI*NI2,c.im*NI*NI2};
    ippsSet_32fc(c, tvec, Nch);
    if (1) {
        Timing T("Filter run with INT filter", Nch*NI*NI2);
        for (int i=0; i<NI*NI2; i++) {
            f->filter(tvec);
        }
    }
    compare_to_ref(*(f->y()), ref);
    delete f;
    ippsFree(tvec);
}

////////////////////////////////////////////////////////////////////
// Filter moving average
////////////////////////////////////////////////////////////////////
void test_filter_mavg()
{
    const int NI2 = 10000;
    cout << endl << "---- Test Filter::getFilter(FLT_MAVG)" << endl;
    Ipp32fc* tvec = ippsMalloc_32fc(Nch);
    Filter*  f = Filter::getFilter(FLT_MAVG);
    f->init(/*windowsize*/128, Nch);
    f->clear();
    Ipp32fc c = {1, 0.5};
    Ipp32fc ref = c;
    ippsSet_32fc(c, tvec, Nch);
    if (1) {
        Timing T("Filter run with MAvg filter", Nch*NI*NI2);
        for (int i=0; i<NI*NI2; i++) {
            f->filter(tvec);
        }
    }
    compare_to_ref(*(f->y()), ref);
    delete f;
    ippsFree(tvec);
}

////////////////////////////////////////////////////////////////////
// Filter coeff file parser
////////////////////////////////////////////////////////////////////
void test_filterloader()
{
    std::string fn("filter_longchain.coeff");
    cout << endl << "---- Test filter chain loader with " << fn << endl;
    FilterChain fc;
    fc.buildFromFile(fn.c_str(), /*channels:*/Nch);
    fc.summary(cout);
}

////////////////////////////////////////////////////////////////////
// Benchmark performance of direct filter versus single-filter chain
////////////////////////////////////////////////////////////////////
void bench_filter_vs_chain()
{
    cout << endl << "---- Benchmark DSVF filter on its own and in a chain" << endl;
    FilterChain fc;
    Filter* flt1 = new IntFilter();
    Filter* flt2 = new IntFilter();
    flt1->init(/*order ignored if DSVF, window if MAvg:*/0, Nch);
    flt2->init(/*order ignored if DSVF, window if MAvg:*/0, Nch);
    flt1->generate_coeffs(4e-3);
    flt2->generate_coeffs(4e-3);
    fc.appendFilter(flt2);

    Ipp32fc c = {0, 0};
    Ipp32fc ref = {0.0f,0.0f};
    Ipp32fc* singlechdata = load_datafile();
    Ipp32fc* multichannelline = load_constvec(c);

    Ipp32fc* filtered = ippsMalloc_32fc(Nch);

    if (1) {
        Timing T("** Computing the reference on one channel ", Nsamps);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            ref.re += singlechdata[s].re;
            ref.im += singlechdata[s].im;
        }
    }
    ippsZero_32fc(filtered, Nch);
    if (1) {
        Timing T("** Cache warmup with inline ippsAdd_32fc_I", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            c = singlechdata[s];
            //default method:
            ippsSet_32fc(c, multichannelline, Nch);
            ippsAdd_32fc_I(multichannelline, filtered, Nch);
            //other method:
            //ippsSet_32f(c.re, (Ipp32f*)multichannelline, 2*Nch);
            //ippsAdd_32f_I((Ipp32f*)multichannelline, (Ipp32f*)filtered, 2*Nch);
        }
    }
    ippsZero_32fc(filtered, Nch);
    if (1) {
        Timing T("** Inline ippsAdd_32fc_I speed", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            c = singlechdata[s];
            ippsSet_32fc(c, multichannelline, Nch);
            ippsAdd_32fc_I(multichannelline, filtered, Nch);
        }
    }
    compare_to_ref(filtered[0], ref);
    if (1) {
        Timing T("** Separate complex INT filter", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            c = singlechdata[s];
            ippsSet_32fc(c, multichannelline, Nch);
            flt1->filter(multichannelline);
        }
    }
    compare_to_ref(*(flt1->y()), ref);
    if (1) {
        Timing T("** Chain 1-entry complex INT filter", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            c = singlechdata[s];
            ippsSet_32fc(c, multichannelline, Nch);
            fc.filter(multichannelline);
        }
    }
    compare_to_ref(*(fc.y()), ref);
    ippsFree(singlechdata);
    ippsFree(multichannelline);
    ippsFree(filtered);
    delete flt1;
    // delete flt2; -- deleted by FilterChain
}


////////////////////////////////////////////////////////////////////
// Test filterchain created from .coeff file and filter a constant
////////////////////////////////////////////////////////////////////
void test_filterloader_on_data(bool ownOutput)
{
    Ipp32fc c = {1, 0.5};
    std::string fn("filter_chain1.coeff");
    cout << endl << "---- Test filtering of constant "
         << "{" << c.re << "," << c.im << "} " 
         << (NI*Nsamps) << " times with setup in " << fn << endl;
    FilterChain fc;
    fc.buildFromFile(fn.c_str(), /*channels:*/Nch);
    fc.summary(cout);

    Ipp32fc* tvec = ippsMalloc_32fc(Nch);
    Ipp32fc* ovec = ippsMalloc_32fc(Nch);
    ippsZero_32fc(ovec, Nch);
    if (ownOutput) { 
        fc.setUserOutbuffer(ovec); 
        cout << "Assigned USER output buffer to filterchain" << endl;
    }
    ippsSet_32fc(c, tvec, Nch);
    if (1) {
        Timing T("** filter_chain1.coeff", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            fc.filter(tvec);
            if (0) cout << "Output[" << (s + i*Nsamps) << "] = {" << fc.y()->re << "," << fc.y()->im << "} " << endl;
        }
    }
    cout << "Output = {" << fc.y()->re << "," << fc.y()->im << "} " << endl;

    // some expected values (really depends on filter_chain1.coeff setup though!)
    cout << "** Expected value _IF_ 1:3 decimation and then integration" << endl;
    Ipp32fc ex1 = { c.re*std::ceil(NI*Nsamps/3.0), c.im*std::ceil(NI*Nsamps/3.0) };
    compare_to_ref(*(fc.y()), ex1);
    ippsFree(tvec);
    ippsFree(ovec);
}

////////////////////////////////////////////////////////////////////
// Test filterchain created from .coeff file and filter a constant
////////////////////////////////////////////////////////////////////
void test_filterloader_on_data2(bool ownOutput)
{
    Ipp32fc c = {1, 0.5};
    std::string fn("filter_longchain.coeff");
    cout << endl << "---- Test filtering of constant "
         << "{" << c.re << "," << c.im << "} " 
         << (NI*Nsamps) << " times with setup in " << fn << endl;
    FilterChain fc;
    fc.buildFromFile(fn.c_str(), /*channels:*/Nch);
    fc.summary(cout);

    Ipp32fc* tvec = ippsMalloc_32fc(Nch);
    Ipp32fc* ovec = ippsMalloc_32fc(Nch);
    ippsZero_32fc(ovec, Nch);
    if (ownOutput) { 
        fc.setUserOutbuffer(ovec); 
        cout << "Assigned USER output buffer to filterchain" << endl;
    }
    ippsSet_32fc(c, tvec, Nch);
    if (1) {
        Timing T("** filter_longchain.coeff", Nsamps*Nch*NI);
        for (int i=0;i<NI;i++)
        for (int s=0;s<Nsamps;s++) {
            fc.filter(tvec);
            if (0) cout << "Output[" << (s + i*Nsamps) << "] = {" << fc.y()->re << "," << fc.y()->im << "} " << endl;
        }
    }
    cout << "Output = {" << fc.y()->re << "," << fc.y()->im << "} " << endl;
    ippsFree(tvec);
    ippsFree(ovec);
}

////////////////////////////////////////////////////////////////////
// MAIN choose tests
////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    ippStaticInit();
    cout_precision_init();

    cout << "----------------------------------------------------" << endl;
    cout << "Test setup: Nch=" << Nch << " Nsamps=" << Nsamps << " NI=" << NI << endl;
    cout << "----------------------------------------------------" << endl;

    test_helperclass();

    test_filterfactory();

    test_filter_mavg();

    bench_filter_vs_chain();

    test_filterloader();

    test_filterloader_on_data(false);

    test_filterloader_on_data(true);

    test_filterloader_on_data2(false);

    test_filterloader_on_data2(true);

    cout << endl << "---- END" << endl;
    return 0;
}
