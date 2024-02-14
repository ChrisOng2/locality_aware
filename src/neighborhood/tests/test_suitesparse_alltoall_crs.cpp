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

#include "tests/sparse_mat.hpp"
#include "tests/par_binary_IO.hpp"

void test_matrix(const char* filename)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    MPIX_Comm* xcomm;
    MPIX_Comm_init(&xcomm, MPI_COMM_WORLD);

    // Read suitesparse matrix
    ParMat<int> A;
    int idx;
    readParMatrix(filename, A);
    form_comm(A);

    int n_recvs;
    std::vector<int> src(A.send_comm.n_msgs+1);
    std::vector<int> recvvals(A.send_comm.n_msgs+1);
    std::vector<int> recvcounts(num_procs, 0);

    /* TEST RMA VERSION */
    n_recvs = -1;
    alltoall_crs_rma(A.recv_comm.n_msgs, A.recv_comm.procs.data(), 1, MPI_INT, 
            A.recv_comm.counts.data(), &n_recvs, src.data(), 1, MPI_INT,
            recvvals.data(), xcomm);
    ASSERT_EQ(n_recvs, A.send_comm.n_msgs);
    for (int i = 0; i < n_recvs; i++)
    {
        recvcounts[src[i]] = recvvals[i];
    }
    for (int i = 0; i < A.send_comm.n_msgs; i++)
    {
        ASSERT_EQ(A.send_comm.counts[i], recvcounts[A.send_comm.procs[i]]);
    }

    /* TEST PERSONALIZED VERSION */
    n_recvs = -1;
    alltoall_crs_personalized(A.recv_comm.n_msgs, A.recv_comm.procs.data(), 1, MPI_INT,
            A.recv_comm.counts.data(), &n_recvs, src.data(), 1, MPI_INT,
            recvvals.data(), xcomm);
    ASSERT_EQ(n_recvs, A.send_comm.n_msgs);
    for (int i = 0; i < n_recvs; i++)
    {
        recvcounts[src[i]] = recvvals[i];
    }
    for (int i = 0; i < A.send_comm.n_msgs; i++)
    {
        ASSERT_EQ(A.send_comm.counts[i], recvcounts[A.send_comm.procs[i]]);
    }

    /* TEST NONBLOCKING VERSION */
    n_recvs = -1;
    alltoall_crs_nonblocking(A.recv_comm.n_msgs, A.recv_comm.procs.data(), 1, MPI_INT,
            A.recv_comm.counts.data(), &n_recvs, src.data(), 1, MPI_INT,
            recvvals.data(), xcomm);
    ASSERT_EQ(n_recvs, A.send_comm.n_msgs);
    for (int i = 0; i < n_recvs; i++)
    {
        recvcounts[src[i]] = recvvals[i];
    }
    for (int i = 0; i < A.send_comm.n_msgs; i++)
    {
        ASSERT_EQ(A.send_comm.counts[i], recvcounts[A.send_comm.procs[i]]);
    }

    MPIX_Comm_free(xcomm);
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
    test_matrix("../../../../test_data/lp_woodw.pm");
    test_matrix("../../../../test_data/lp_sctap2.pm");
}

