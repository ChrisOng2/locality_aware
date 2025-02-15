//
// Created by Christopher on 2/14/2025.
//

#ifndef BASICPERFORMANCEMODEL_H
#define BASICPERFORMANCEMODEL_H

#include "tests/sparse_mat.hpp"
#include <iostream>
#include <cmath>
#include <vector>

void BasicTimingMode(ParMat<int> A) {
    double salpha = 4.0E-6;
    double ealpha = 1.1E-5;
    double ralpha = 2.0E-5;

    double aRn    = 1e99;
    double eRn    = 1e99;
    double rRn    = 5.5E9;

    double aRCb   = 6.3E8;
    double eRCb   = 1.7E9;
    double rRCb   = 3.6E9;

    double sRCi   = -1.8E7;
    double eRCi   = 6.2E7;
    double rRCi   = 6.1E8;

    double cfloprate = 8900.7*1024*1024;
    double sfloprate = 9046.2*1024*1024;
    double afloprate = 15548.3*1024*1024;
    double tfloprate = 16606.1*1024*1024;

    int messagesize  = A.send_comm.size_msgs;
    int messagenum   = A.send_comm.n_msgs;
    int onprocflops  = A.on_proc.nnz;
    int offprocflops = A.off_proc.nnz;
    int totalflops   = onprocflops + offprocflops;

    // int procnum = 1;
    // long double beta = procnum / std::min(rRn, procnum * rRCb);
    double eagmesssize  = 0;
    double eagmessnum = 0;
    double rendmesssize = 0;
    double rendmessnum  = 0;

    double ebeta = 1 / eRCb;
    double rbeta = 1 / rRCb;

    if (messagesize < 8192) {
        eagmessnum = messagenum;
        eagmesssize = messagesize;
    }

    if (messagesize > 8192) {
        rendmessnum = messagenum;
        rendmesssize = messagesize;
    }

    //double Timing = ralpha * messagenum + beta * messagesize
    //              + totalflops / tfloprate;
    double Timing = ealpha * eagmessnum + ralpha * rendmessnum
                  + ebeta * eagmesssize + rbeta * rendmesssize
                  + totalflops / tfloprate;
    std::cout << "Sent Message Timing: " << Timing << " sec" << std::endl;
}

#endif //BASICPERFORMANCEMODEL_H
