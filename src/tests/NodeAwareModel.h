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
                Alpha = 6.67E-7;
                Beta = 5.8E-10;
            }

            if (msg_size > 8192) {
                Alpha = 3.20E-7;
                Beta = 3.46E-10;
            }
        }
        else if (proc/PPN == rank/PPN) {
            if (msg_size < 8192) {
                Alpha = 6.47E-7;
                Beta = 5.93E-10;
            }

            if (msg_size > 8192) {
                Alpha = 3.49E-7;
                Beta = 3.46E-10;
            }
        }
        else {
            if (msg_size < 8192) {
                Alpha = 4.53E-6;
                Beta = 2.66E-9;
            }

            if (msg_size > 8192) {
                Alpha = 3.23E-6;
                Beta = 6.87E-10;
            }
        }

        T += Alpha + msg_size * Beta;
    }

    //std::cout << "Total Estimates Timing: " << T << " (sec)" << std::endl;
    return T;
}


#endif //NODEAWAREMODEL_H
