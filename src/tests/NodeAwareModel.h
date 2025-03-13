//
// Created by Christopher on 3/3/2025.
//

#ifndef NODEAWAREMODEL_H
#define NODEAWAREMODEL_H
#define PPN 8
#define PPS 4
#define SPN 1

#include "tests/sparse_mat.hpp"
#include <iostream>

double NodeAwareModel(ParMat<int> A) {
    double T = 0;

    for (int i = 0; i < A.send_comm.n_msgs; i++) {
        double Alpha = 0;
        double Beta = 0;
        int proc = A.send_comm.procs[i];
        int msg_size = A.send_comm.counts[i];
        int rank = i;

        if (proc/PPS == rank/PPS) {
            if (msg_size < 8192) {
                Alpha = 5.3E-7;
                Beta = 1/3.2E9;
            }

            if (msg_size > 8192) {
                Alpha = 1.7E-6;
                Beta = 1/6.2E9;
            }
        }
        else if (proc/PPN == rank/PPN) {
            if (msg_size < 8192) {
                Alpha = 1.2E-6;
                Beta = 1/9.6E8;
            }

            if (msg_size > 8192) {
                Alpha = 2.5E-6;
                Beta = 1/6.2E9;
            }
        }
        else {
            if (msg_size < 8192) {
                Alpha = 7.0E-6;
                Beta = 1/7.5E8;
            }

            if (msg_size > 8192) {
                Alpha = 3.0E-6;
                Beta = 1/2.9E9;
            }
        }

        T += Alpha + msg_size * Beta;
    }

    //std::cout << "Total Estimates Timing: " << T << " (sec)" << std::endl;
    return T;
}


#endif //NODEAWAREMODEL_H
