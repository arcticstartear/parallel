#include <iostream>
#include <mpi.h>
#include <sys/time.h>
#include <omp.h>
#include <sstream>
#include <fstream>
#include <pmmintrin.h>
using namespace std;

static const int thread_count = 8;

unsigned int Act[85401][2670] = {0};
unsigned int Pas[85401][2670] = {0};

void LU(int myid, int num)
{
    int i;
#pragma omp parallel num_threads(thread_count)
    for (i = 85400; i - 8 >= -1; i -= 8)
    {
#pragma omp for schedule(dynamic, 20)
        for (int j = 0; j < 54274; j++)
        {
            if (int(j % num) == myid)
            {
                while (Pas[j][2669] <= i && Pas[j][2669] >= i - 7)
                {
                    int index = Pas[j][2669];
                    if (Act[index][2669] == 1)
                    {
                        int k;
                        __m128 va_Pas, va_Act;
                        for (k = 0; k + 4 <= 2669; k += 4)
                        {
                            va_Pas = _mm_loadu_ps((float *)&(Pas[j][k]));
                            va_Act = _mm_loadu_ps((float *)&(Act[index][k]));
                            va_Pas = _mm_xor_ps(va_Pas, va_Act);
                            _mm_store_ss((float *)&(Pas[j][k]), va_Pas);
                        }
                        for (; k < 2669; k++)
                            Pas[j][k] = Pas[j][k] ^ Act[index][k];
                        int num = 0, S_num = 0;
                        for (num = 0; num < 2669; num++)
                        {
                            if (Pas[j][num] != 0)
                            {
                                unsigned int temp = Pas[j][num];
                                while (temp != 0)
                                {
                                    temp = temp >> 1;
                                    S_num++;
                                }
                                S_num += num * 32;
                                break;
                            }
                        }
                        Pas[j][2669] = S_num - 1;
                    }
                    else
                        break;
                }
            }
        }
    }
#pragma omp parallel num_threads(thread_count)
    for (int i = 85401 % 8 - 1; i >= 0; i--)
    {
#pragma omp for schedule(dynamic, 20)
        for (int j = 0; j < 54274; j++)
        {
            if (int(j % num) == myid)
            {
                while (Pas[j][2669] == i)
                {
                    if (Act[i][2669] == 1)
                    {
                        int k;
                        __m128 va_Pas, va_Act;
                        for (k = 0; k + 4 <= 2669; k += 4)
                        {
                            va_Pas = _mm_loadu_ps((float *)&(Pas[j][k]));
                            va_Act = _mm_loadu_ps((float *)&(Act[i][k]));
                            va_Pas = _mm_xor_ps(va_Pas, va_Act);
                            _mm_store_ss((float *)&(Pas[j][k]), va_Pas);
                        }
                        for (; k < 2669; k++)
                            Pas[j][k] = Pas[j][k] ^ Act[i][k];
                        int num = 0, S_num = 0;
                        for (num = 0; num < 2669; num++)
                        {
                            if (Pas[j][num] != 0)
                            {
                                unsigned int temp = Pas[j][num];
                                while (temp != 0)
                                {
                                    temp = temp >> 1;
                                    S_num++;
                                }
                                S_num += num * 32;
                                break;
                            }
                        }
                        Pas[j][2669] = S_num - 1;
                    }
                    else
                        break;
                }
            }
        }
    }
}

int main()
{
    timeval start, end;
    unsigned int a;
    ifstream infile("act.txt");
    char fin[100000] = {0};
    int index;
    while (infile.getline(fin, sizeof(fin)))
    {
        std::stringstream line(fin);
        int flag = false;
        while (line >> a)
        {
            if (flag == false)
            {
                index = a;
                flag = true;
            }
            int k = a % 32;
            int j = a / 32;
            int temp = 1 << k;
            Act[index][2668 - j] += temp;
            Act[index][2669] = 1;
        }
    }
    unsigned int a;
    ifstream infile("pas.txt");
    char fin[100000] = {0};
    int index = 0;
    while (infile.getline(fin, sizeof(fin)))
    {
        std::stringstream line(fin);
        int flag = false;
        while (line >> a)
        {
            if (flag == false)
            {
                Pas[index][2669] = a;
                flag = true;
            }
            int k = a % 32;
            int j = a / 32;
            int temp = 1 << k;
            Pas[index][2668 - j] += temp;
        }
        index++;
    }
    gettimeofday(&start, NULL);
    MPI_Init(NULL, NULL);
    int size = 8;
    int myid;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_myid(MPI_COMM_WORLD, &myid);
    if (myid == 0)
    {
        gettimeofday(&start, NULL);
        int sign;
        while (sign == 1)
        {
            for (int i = 0; i < 54274; i++)
                if (i % size != 0)
                    MPI_Send(&Pas[i], 2670, MPI_FLOAT, i % size, 0, MPI_COMM_WORLD);
            LU(myid, size);
            for (int i = 0; i < 54274; i++)
                if (i % size != 0)
                    MPI_Recv(&Pas[i], 2670, MPI_FLOAT, i % size, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sign = 0;
            for (int i = 0; i < 54274; i++)
            {
                int temp = Pas[i][2669];
                if (temp == -1)
                    continue;
                if (Act[temp][2669] == 0)
                {
                    for (int k = 0; k < 2669; k++)
                        Act[temp][k] = Pas[i][k];
                    Pas[i][2669] = -1;
                    sign = 1;
                }
            }
            for (int r = 1; r < size; r++)
                MPI_Send(&sign, 1, MPI_INT, r, 2, MPI_COMM_WORLD);
        }
        gettimeofday(&end, NULL);
        cout << "用时: " << (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0 << "ms" << endl;
    }
    else
    {
        int sign;
        while (sign == 1)
        {
            for (int i = myid; i < 54274; i += size)
                MPI_Recv(&Pas[i], 2670, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            LU(myid, size);
            for (int i = myid; i < 54274; i += size)
                MPI_Send(&Pas[i], 2670, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
            MPI_Recv(&sign, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    MPI_Finalize();
}