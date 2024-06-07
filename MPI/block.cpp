#include <iostream>
#include <mpi.h>
#include <sys/time.h>
#include <omp.h>
using namespace std;

const int n = 1000;
float A[n][n];

void LU(float A[][n], int myid, int r1, int r2, int num)
{
    for (int k = 0; k < n; k++)
    { 
        if (k >= r1 && k <= r2)
        {
            for (int j = k + 1; j < n; j++)
                A[k][j] = A[k][j] / A[k][k];
            A[k][k] = 1.0; 
            for (int j = 0; j < num; j++)
                if (j != myid)
                    MPI_Send(&A[k][0], n, MPI_FLOAT, j, 2, MPI_COMM_WORLD);
        } 
        else
            MPI_Recv(&A[k][0], n, MPI_FLOAT, k * num / n, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = r1; i <= r2; i++)
            if (i >= k + 1)
            {
                for (int j = k + 1; j < n; j++)
                    A[i][j] = A[i][j] - A[k][j] * A[i][k];
                A[i][k] = 0.0;
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
    int block_size = n / size;
    int remainder = n % size; 
    if (rank == 0)
    { 
        gettimeofday(&start, NULL);
        for (int i = 1; i < size; i++)
        {
            if (i != size - 1)
                for (int j = 0; j < block_size; j++)
                    MPI_Send(&A[i * block_size + j][0], n, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
            else
                for (int j = 0; j < block_size + remainder; j++)
                    MPI_Send(&A[i * block_size + j][0], n, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
        }
        int r1 = rank * block_size;
        int r2 = r1 + block_size - 1;
        LU(A, rank, r1, r2, size); 
        for (int i = 1; i < size; i++)
        {
            if (i != size - 1)
                for (int j = 0; j < block_size; j++)
                    MPI_Recv(&A[i * block_size + j][0], n, MPI_FLOAT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            else
                for (int j = 0; j < block_size + remainder; j++)
                    MPI_Recv(&A[i * block_size + j][0], n, MPI_FLOAT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        gettimeofday(&end, NULL);
        cout << "用时: " << (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0 << "ms" << endl;
    }
    else
    {
        int r1 = rank * block_size;
        int r2; 
        if (rank != size - 1)
        {
            r2 = r1 + block_size - 1;
            for (int j = 0; j < block_size; j++)
                MPI_Recv(&A[rank * block_size + j][0], n, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            r2 = r1 + block_size + remainder - 1;
            for (int j = 0; j < block_size + remainder; j++)
                MPI_Recv(&A[rank * block_size + j][0], n, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        LU(A, rank, r1, r2, size); 
        if (rank != size - 1)
            for (int j = 0; j < block_size; j++)
                MPI_Send(&A[rank * block_size + j][0], n, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
        else
            for (int j = 0; j < block_size + remainder; j++)
                MPI_Send(&A[rank * block_size + j][0], n, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
}
