#include <iostream>
#include <mpi.h>
#include <sys/time.h>
#include <omp.h>
using namespace std;

const int n = 1000;
float A[n][n];

void LU(float A[][n], int myid, int num)
{
    int prev = (myid - 1 + num) % num;
    int next = (myid + 1) % num;
    for (int k = 0; k < n; k++)
    {
        if (k % num == myid)
        {
            for (int j = k + 1; j < n; j++)
                A[k][j] = A[k][j] / A[k][k];
            A[k][k] = 1.0;
            MPI_Send(&A[k][0], n, MPI_FLOAT, next, 2, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Recv(&A[k][0], n, MPI_FLOAT, prev, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (k % num != next)
                MPI_Send(&A[k][0], n, MPI_FLOAT, next, 2, MPI_COMM_WORLD);
        }
        for (int i = k + 1; i < n; i++)
        {
            if (i % num == myid)
            {
                for (int j = k + 1; j < n; j++)
                    A[i][j] = A[i][j] - A[k][j] * A[i][k];
                A[i][k] = 0.0;
            }
        }
    }
}

int main()
{
    timeval start, end;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
            A[i][j] = 0.0;
        A[i][i] = 1.0;
        for (int j = i + 1; j < n; j++)
            A[i][j] = rand();
    }
    for (int k = 0; k < n; k++)
        for (int i = k + 1; i < n; i++)
            for (int j = 0; j < n; j++)
                A[i][j] += A[k][j];
    MPI_Init(NULL, NULL);
    int size = 8;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        gettimeofday(&start, NULL);
        for (int i = 0; i < n; i++)
            if (i % size != 0)
                MPI_Send(&A[i][0], n, MPI_FLOAT, i % size, 0, MPI_COMM_WORLD);
        LU(A, rank, size);
        for (int i = 0; i < n; i++)
            if (i % size != 0)
                MPI_Recv(&A[i][0], n, MPI_FLOAT, i % size, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        gettimeofday(&end, NULL);
        cout << "用时: " << (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0 << "ms" << endl;
    }
    else
    {
        for (int i = rank; i < n; i += size)
            MPI_Recv(&A[i][0], n, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        LU(A, rank, size);
        for (int i = rank; i < n; i += size)
            MPI_Send(&A[i][0], n, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
}