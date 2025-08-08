#include "amr_monitor.h"
#include "tools-common/logging.h"

#include <cstdio>
#include <mpi.h>
#include <pdlfs-common/env.h>

extern "C" {
int MPI_Init(int* argc, char*** argv) {
  int rv = PMPI_Init(argc, argv);

  if (rv != MPI_SUCCESS) {
    return rv;
  }

  int rank;
  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int nranks;
  PMPI_Comm_size(MPI_COMM_WORLD, &nranks);

  amr::monitor =
      std::make_unique<amr::AMRMonitor>(pdlfs::Env::Default(), rank, nranks);

  if (rank == 0) {
    MLOG(MLOG_INFO, "AMRMonitor initialized on rank %d", rank);
  }

  return rv;
}

int MPI_Finalize() {
  amr::monitor.reset();

  PMPI_Barrier(MPI_COMM_WORLD);

  int rv = PMPI_Finalize();
  return rv;
}

//
// MPI Point-to-Point Hooks.
//

int MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm) {
  int rv = PMPI_Send(buf, count, datatype, dest, tag, comm);
  amr::monitor->LogMPISend(dest, datatype, count);

  amr::monitor->tracer_.LogMPIIsend(nullptr, count, dest, tag);
  return rv;
}

int MPI_Isend(const void* buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request* request) {
  int rv = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
  amr::monitor->LogMPISend(dest, datatype, count);

  amr::monitor->tracer_.LogMPIIsend(request, count, dest, tag);
  return rv;
}

int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status* status) {
  int rv = PMPI_Recv(buf, count, datatype, source, tag, comm, status);
  amr::monitor->LogMPIRecv(source, datatype, count);

  amr::monitor->tracer_.MPIIrecv(nullptr, count, source, tag);
  return rv;
}

int MPI_Irecv(void* buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request* request) {
  int rv = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  amr::monitor->LogMPIRecv(source, datatype, count);

  amr::monitor->tracer_.MPIIrecv(request, count, source, tag);
  return rv;
}

int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status) {
  int rv = PMPI_Test(request, flag, status);
  amr::monitor->tracer_.LogMPITestEnd(request, *flag);
  amr::monitor->LogMPITestEnd(*flag, request);
  return rv;
}

int MPI_Wait(MPI_Request* request, MPI_Status* status) {
  int rv = PMPI_Wait(request, status);
  amr::monitor->tracer_.LogMPIWait(request);
  return rv;
}

int MPI_Waitall(int count, MPI_Request* requests, MPI_Status* statuses) {
  int rv = PMPI_Waitall(count, requests, statuses);
  amr::monitor->tracer_.LogMPIWaitall(requests, count);
  return rv;
}

//
// BEGIN MPI Collective Hooks. These are all the same template.
// This has not been validated yet to be exhaustive.
//

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm* newcomm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Comm_dup");

  int rv = PMPI_Comm_dup(comm, newcomm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Comm_dup", ts_end - ts_beg);

  return rv;
}

int MPI_Barrier(MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Barrier");

  int rv = PMPI_Barrier(comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Barrier", ts_end - ts_beg);

  return rv;
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Bcast");

  int rv = PMPI_Bcast(buffer, count, datatype, root, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Bcast", ts_end - ts_beg);

  return rv;
}

int MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
               void* recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Gather");

  int rv = PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                       recvtype, root, comm);
  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Gather", ts_end - ts_beg);

  return rv;
}

int MPI_Gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                void* recvbuf, const int* recvcounts, const int* displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Gatherv");

  int rv = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                        displs, recvtype, root, comm);
  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Gatherv", ts_end - ts_beg);

  return rv;
}

int MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                void* recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Scatter");

  int rv = PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                        recvtype, root, comm);
  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Scatter", ts_end - ts_beg);

  return rv;
}

int MPI_Scatterv(const void* sendbuf, const int* sendcounts, const int* displs,
                 MPI_Datatype sendtype, void* recvbuf, int recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Scatterv");

  int rv = PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                         recvcount, recvtype, root, comm);
  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Scatterv", ts_end - ts_beg);

  return rv;
}

int MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                  void* recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Allgather");

  int rv = PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, comm);
  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Allgather", ts_end - ts_beg);

  return rv;
}

int MPI_Allgatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                   void* recvbuf, const int* recvcounts, const int* displs,
                   MPI_Datatype recvtype, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Allgatherv");

  int rv = PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                           displs, recvtype, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Allgatherv", ts_end - ts_beg);

  return rv;
}

int MPI_Alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                 void* recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Alltoall");

  int rv = PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                         recvtype, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Alltoall", ts_end - ts_beg);

  return rv;
}

int MPI_Alltoallv(const void* sendbuf, const int* sendcounts,
                  const int* sdispls, MPI_Datatype sendtype, void* recvbuf,
                  const int* recvcounts, const int* rdispls,
                  MPI_Datatype recvtype, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Alltoallv");

  int rv = PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                          recvcounts, rdispls, recvtype, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Alltoallv", ts_end - ts_beg);

  return rv;
}

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Reduce");

  int rv = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Reduce", ts_end - ts_beg);

  return rv;
}

int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Allreduce");

  int rv = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Allreduce", ts_end - ts_beg);

  return rv;
}

int MPI_Ireduce(const void* sendbuf, void* recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                MPI_Request* request) {
  auto ts_beg = amr::monitor->Now();
  amr::monitor->LogMPICollectiveBegin("MPI_Ireduce");

  int rv =
      PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);

  auto ts_end = amr::monitor->Now();
  amr::monitor->LogMPICollectiveEnd("MPI_Ireduce", ts_end - ts_beg);

  return rv;
}

int MPI_Iallreduce(const void* sendbuf, void* recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Request* request) {
  amr::monitor->LogAsyncMPIRequest("MPI_Iallreduce", request);

  int rv =
      PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request);

  return rv;
}

//
// END MPI Collective Hooks
//
}  // extern "C"
