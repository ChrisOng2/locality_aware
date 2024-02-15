#include "sparse_coll.h"
#include <stdlib.h>
#include <string.h>

int MPIX_Alltoall_crs(
        int send_nnz,
        int* dest,
        int sendcount,
        MPI_Datatype sendtype,
        int* sendvals,
        int* recv_nnz,
        int* src,
        int recvcount,
        MPI_Datatype recvtype,
        int* recvvals,
        MPIX_Comm* xcomm)
{
    return alltoall_crs_rma(send_nnz, dest, sendcount, sendtype, sendvals,
            recv_nnz, src, recvcount, recvtype, recvvals, xcomm);
}

int MPIX_Alltoallv_crs(
        int send_nnz,
        int* dest,
        int* sendcounts,
        int* sdispls,
        MPI_Datatype sendtype,
        int* sendvals,
        int* recv_nnz,
        int* recv_size,
        int* src,
        int* recvcounts,
        int* rdispls,
        MPI_Datatype recvtype,
        int* recvvals,
        MPIX_Comm* comm)
{
    return alltoallv_crs_personalized(send_nnz, dest, sendcounts, sdispls,
            sendtype, sendvals, recv_nnz, recv_size, src, recvcounts, 
            rdispls, recvtype, recvvals, comm);
}

int alltoall_crs_rma(int send_nnz, int* dest, int sendcount, 
        MPI_Datatype sendtype, void* sendvals,
        int* recv_nnz, int* src, int recvcount, MPI_Datatype recvtype,
        void* recvvals, MPIX_Comm* comm)
{ 
    int rank, num_procs;
    MPI_Comm_rank(comm->global_comm, &rank);
    MPI_Comm_size(comm->global_comm, &num_procs);


    int start, end, proc, count, ctr, flag;
    MPI_Status recv_status;

    // Get bytes per datatype, total bytes to be recvd (size of win_array)
    char* send_buffer = (char*)sendvals;
    char* recv_buffer = (char*)recvvals;
    int send_bytes, recv_bytes;
    MPI_Type_size(sendtype, &send_bytes);
    MPI_Type_size(recvtype, &recv_bytes);
    int bytes = num_procs * recvcount * recv_bytes;

    if (comm->win_bytes != bytes
            || comm->win_type_bytes != 1)
        MPIX_Comm_win_free(comm);

    if (comm->win == MPI_WIN_NULL)
    {
        MPIX_Comm_win_init(comm, bytes, 1);
    }

    // RMA puts to find sizes recvd from each process
    memset((comm->win_array), 0, bytes);

    send_bytes *= sendcount;
    recv_bytes *= recvcount;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_fence(MPI_MODE_NOSTORE|MPI_MODE_NOPRECEDE, comm->win);
    for (int i = 0; i < send_nnz; i++)
    {
         MPI_Put(&(send_buffer[i*send_bytes]), send_bytes, MPI_CHAR,
                 dest[i], rank*recv_bytes, recv_bytes, MPI_CHAR, comm->win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_fence(MPI_MODE_NOPUT|MPI_MODE_NOSUCCEED, comm->win);

    ctr = 0;
    for (int i = 0; i < num_procs; i++)
    {
        flag = 0;
        for (int j = 0; j < recv_bytes; j++)
        {
            if (comm->win_array[i*recv_bytes+j])
            {
                flag = 1;
                break;
            }
        }
        if (flag)
        {
            src[ctr] = i;
            memcpy(&(recv_buffer[ctr*recv_bytes]), &(comm->win_array[i*recv_bytes]), recv_bytes);
            ctr++;
        }
    }

    *recv_nnz = ctr;

    return MPI_SUCCESS;
}

int alltoall_crs_personalized(int send_nnz, int* dest, int sendcount,
        MPI_Datatype sendtype, void* sendvals,
        int* recv_nnz, int* src, int recvcount, MPI_Datatype recvtype,
        void* recvvals, MPIX_Comm* comm)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int tag = 928431;
    MPI_Status recv_status;
    int proc, ctr;

    char* send_buffer = (char*)sendvals;
    char* recv_buffer = (char*)recvvals;
    int send_bytes, recv_bytes;
    MPI_Type_size(sendtype, &send_bytes);
    MPI_Type_size(recvtype, &recv_bytes);
    send_bytes *= sendcount;
    recv_bytes *= recvcount;

    if (*recv_nnz < 0)
    {
        int* msg_counts = (int*)malloc(num_procs*sizeof(int));
        memset(msg_counts, 0, num_procs*sizeof(int));

        for (int i = 0; i < send_nnz; i++)
            msg_counts[dest[i]] = 1;
        MPI_Allreduce(MPI_IN_PLACE, msg_counts, num_procs, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        *recv_nnz = msg_counts[rank];
        free(msg_counts);
    }

    if (comm->n_requests < send_nnz)
        MPIX_Comm_req_resize(comm, send_nnz);

    for (int i = 0; i < send_nnz; i++)
    {
        proc = dest[i];
        MPI_Isend(&(send_buffer[i*send_bytes]), send_bytes, MPI_BYTE, proc, tag, comm->global_comm,
                &(comm->requests[i]));
    }

    ctr = 0;
    while (ctr < *recv_nnz)
    {
        MPI_Probe(MPI_ANY_SOURCE, tag, comm->global_comm, &recv_status);
        proc = recv_status.MPI_SOURCE;
        src[ctr] = proc;
        MPI_Recv(&(recv_buffer[ctr*send_bytes]), send_bytes, MPI_BYTE, proc, tag,
                comm->global_comm, &recv_status);
        ctr++;
    }

    if (send_nnz)
        MPI_Waitall(send_nnz, comm->requests, MPI_STATUSES_IGNORE);

    return MPI_SUCCESS;
}


int alltoall_crs_nonblocking(int send_nnz, int* dest, int sendcount,
        MPI_Datatype sendtype, void* sendvals,
        int* recv_nnz, int* src, int recvcount, MPI_Datatype recvtype,
        void* recvvals, MPIX_Comm* comm)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    char* send_buffer = (char*)sendvals;
    char* recv_buffer = (char*)recvvals;
    int send_bytes, recv_bytes;
    MPI_Type_size(sendtype, &send_bytes);
    MPI_Type_size(recvtype, &recv_bytes);
    send_bytes *= sendcount;
    recv_bytes *= recvcount;

    int proc, ctr, flag, ibar;
    MPI_Status recv_status;
    MPI_Request bar_req;
    int tag = 897239;

    if (comm->n_requests < send_nnz)
        MPIX_Comm_req_resize(comm, send_nnz);

    for (int i = 0; i < send_nnz; i++)
    {
        proc = dest[i];
        MPI_Issend(&(send_buffer[i*send_bytes]), send_bytes, MPI_BYTE, proc, tag,
                comm->global_comm, &(comm->requests[i]));
    }

    ibar = 0;
    ctr = 0;
    while (1)
    {
        MPI_Iprobe(MPI_ANY_SOURCE, tag, comm->global_comm, &flag, &recv_status);
        if (flag)
        {
            proc = recv_status.MPI_SOURCE;
            src[ctr] = proc;
            MPI_Recv(&(recv_buffer[ctr*recv_bytes]), recv_bytes, MPI_BYTE, proc, tag,
                    comm->global_comm, &recv_status);
            ctr++;
        }
        if (ibar)
        {
           MPI_Test(&bar_req, &flag, &recv_status);
           if (flag) 
               break;
        }
        else
        {
            MPI_Testall(send_nnz, comm->requests, &flag, MPI_STATUSES_IGNORE);
            if (flag)
            {
                ibar = 1;
                MPI_Ibarrier(comm->global_comm, &bar_req);
            }
        }
    }
    *recv_nnz = ctr;

    return MPI_SUCCESS;
}







int alltoallv_crs_personalized(int send_nnz, int* dest, int* sendcounts,
        int* sdispls, MPI_Datatype sendtype, void* sendvals,
        int* recv_nnz, int* recv_size, int* src, int* recvcounts, 
        int* rdispls, MPI_Datatype recvtype, void* recvvals, MPIX_Comm* comm)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int tag = 928431;
    MPI_Status recv_status;
    int proc, ctr, idx, count;

    char* send_buffer = (char*)sendvals;
    char* recv_buffer = (char*)recvvals;
    int send_bytes, recv_bytes;
    MPI_Type_size(sendtype, &send_bytes);
    MPI_Type_size(recvtype, &recv_bytes);


    if (*recv_size < 0)
    {
        int* msg_counts = (int*)malloc(num_procs*sizeof(int));
        memset(msg_counts, 0, num_procs*sizeof(int));

        for (int i = 0; i < send_nnz; i++)
        {
            msg_counts[dest[i]] = sendcounts[i]*send_bytes;
        }
        MPI_Allreduce(MPI_IN_PLACE, msg_counts, num_procs, MPI_INT, 
                MPI_SUM, MPI_COMM_WORLD);
        *recv_size = msg_counts[rank] / recv_bytes;
        free(msg_counts);
    }

    if (comm->n_requests < send_nnz)
        MPIX_Comm_req_resize(comm, send_nnz);

    for (int i = 0; i < send_nnz; i++)
    {
        proc = dest[i];
        MPI_Isend(&(send_buffer[sdispls[i]*send_bytes]), sendcounts[i]*send_bytes, MPI_BYTE, 
                proc, tag, comm->global_comm, &(comm->requests[i]));
    }

    ctr = 0;
    idx = 0;
    rdispls[0] = 0;
    int bytes = *recv_size * recv_bytes;
    while (ctr < bytes )
    {
        MPI_Probe(MPI_ANY_SOURCE, tag, comm->global_comm, &recv_status);
        MPI_Get_count(&recv_status, MPI_BYTE, &count);
        proc = recv_status.MPI_SOURCE;
        src[idx] = proc;
        recvcounts[idx] = count / recv_bytes;
        rdispls[idx+1] = rdispls[idx] + recvcounts[idx];
        MPI_Recv(&(recv_buffer[ctr]), count, MPI_BYTE, proc, tag,
                comm->global_comm, &recv_status);
        ctr += count;
        idx++;
    }
    *recv_nnz = idx;

    if (send_nnz)
        MPI_Waitall(send_nnz, comm->requests, MPI_STATUSES_IGNORE);

    return MPI_SUCCESS;
}

