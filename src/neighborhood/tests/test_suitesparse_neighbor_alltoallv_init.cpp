// EXPECT_EQ and ASSERT_EQ are macros
// EXPECT_EQ test execution and continues even if there is a failure
// ASSERT_EQ test execution and aborts if there is a failure
// The ASSERT_* variants abort the program execution if an assertion fails
// while EXPECT_* variants continue with the run.


#include "gtest/gtest.h"
#include "mpi_advance.h"
#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <vector>
#include <numeric>
#include <set>
#include <ctime>

#include "tests/sparse_mat.hpp"
#include "tests/par_binary_IO.hpp"
#include "tests/BasicPerformanceModel.h"
#include "tests/NodeAwareModel.h"

void test_matrix(const char* filename)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Read suitesparse matrix
    ParMat<int> A;
    int idx;
    readParMatrix(filename, A);
    form_comm(A);

    std::vector<int> std_recv_vals, neigh_recv_vals, new_recv_vals,
            locality_recv_vals, part_locality_recv_vals;
    std::vector<int> send_vals, alltoallv_send_vals;
    std::vector<long> send_indices;

    if (A.on_proc.n_cols)
    {
        send_vals.resize(A.on_proc.n_cols);
        std::iota(send_vals.begin(), send_vals.end(), 0);
        for (int i = 0; i < A.on_proc.n_cols; i++)
            send_vals[i] += (rank*1000);
    }

    if (A.recv_comm.size_msgs)
    {
        std_recv_vals.resize(A.recv_comm.size_msgs);
        neigh_recv_vals.resize(A.recv_comm.size_msgs);
        new_recv_vals.resize(A.recv_comm.size_msgs);
        locality_recv_vals.resize(A.recv_comm.size_msgs);
        part_locality_recv_vals.resize(A.recv_comm.size_msgs);
    }

    if (A.send_comm.size_msgs)
    {
        alltoallv_send_vals.resize(A.send_comm.size_msgs);
        send_indices.resize(A.send_comm.size_msgs);
        for (int i = 0; i < A.send_comm.size_msgs; i++)
        {
            idx = A.send_comm.idx[i];
            alltoallv_send_vals[i] = send_vals[idx];
            send_indices[i] = A.send_comm.idx[i] + A.first_col;
        }
    }

    communicate(A, send_vals, std_recv_vals, MPI_INT);

    MPI_Comm std_comm;
    MPI_Status status;
    MPIX_Comm* neighbor_comm;
    MPIX_Request* neighbor_request;
    MPIX_Info* xinfo;

    MPIX_Info_init(&xinfo);

    int* s = A.recv_comm.procs.data();
    if (A.recv_comm.n_msgs == 0)
        s = MPI_WEIGHTS_EMPTY;
    int* d = A.send_comm.procs.data();
    if (A.send_comm.n_msgs  == 0)
        d = MPI_WEIGHTS_EMPTY;

    PMPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,
            A.recv_comm.n_msgs,
            s,
            MPI_UNWEIGHTED,
            A.send_comm.n_msgs,
            d,
            MPI_UNWEIGHTED,
            MPI_INFO_NULL,
            0,
            &std_comm);

    int* send_counts = A.send_comm.counts.data();
    if (A.send_comm.counts.data() == NULL)
        send_counts = new int[1];
    int* recv_counts = A.recv_comm.counts.data();
    if (A.recv_comm.counts.data() == NULL)
        recv_counts = new int[1];

    // Timing test modifications

    int TimeTestCount = 1000;
    double t1, t2;
    double tFinalSend, tFinalReceive, PredictiveTimingSend, PredictiveTimingReceive;

    t1 = MPI_Wtime();

    for (int i = 0; i < TimeTestCount; i++) {
        PMPI_Neighbor_alltoallv(alltoallv_send_vals.data(),
           send_counts,
           A.send_comm.ptr.data(),
           MPI_INT,
           neigh_recv_vals.data(),
           recv_counts,
           A.recv_comm.ptr.data(),
           MPI_INT,
           std_comm);
    }

    t2 = MPI_Wtime();

    tFinalSend = (t2 - t1) / TimeTestCount;
    MPI_Allreduce(&tFinalSend, &tFinalReceive, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    PredictiveTimingSend = NodeAwareModel(A);
    MPI_Allreduce(&PredictiveTimingSend, &PredictiveTimingReceive, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) { /* use time on master node */
        std::cout << "| Actual Computation Time: " << tFinalReceive << " seconds |";
        std::cout << "Predicted Computation Time: " << PredictiveTimingReceive << " seconds | \n";
    }

    if (A.send_comm.counts.data() == NULL)
        delete[] send_counts;
    if (A.recv_comm.counts.data() == NULL)
        delete[] recv_counts;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    int temp=RUN_ALL_TESTS();
    MPI_Finalize();
    return temp;
} // end of main() //


TEST(RandomCommTest, TestsInTests)
{
    // Get MPI Information
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    test_matrix("../../../../test_data/dwt_162.pm");
    test_matrix("../../../../test_data/odepa400.pm");
    test_matrix("../../../../test_data/ww_36_pmec_36.pm");
    test_matrix("../../../../test_data/bcsstk01.pm");
    test_matrix("../../../../test_data/west0132.pm");
    test_matrix("../../../../test_data/gams10a.pm");
    test_matrix("../../../../test_data/gams10am.pm");
    test_matrix("../../../../test_data/D_10.pm");
    test_matrix("../../../../test_data/oscil_dcop_11.pm");
    test_matrix("../../../../test_data/tumorAntiAngiogenesis_4.pm");
    test_matrix("../../../../test_data/ch5-5-b1.pm");
    test_matrix("../../../../test_data/msc01050.pm");
    test_matrix("../../../../test_data/SmaGri.pm");
    test_matrix("../../../../test_data/radfr1.pm");
    test_matrix("../../../../test_data/bibd_49_3.pm");
    test_matrix("../../../../test_data/can_1054.pm");
    test_matrix("../../../../test_data/can_1072.pm");
    test_matrix("../../../../test_data/lp_sctap2.pm");
    test_matrix("../../../../test_data/lp_woodw.pm");
}

