#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include <arm_neon.h>
#include <sstream>
#include <fstream>
#include <semaphore.h>
using namespace std;

unsigned int Act[85401][2670] = {0};
unsigned int Pas[85401][2670] = {0};
int NUM_THREADS = 7;
sem_t sem_leader;
sem_t *sem_Next = new sem_t[NUM_THREADS - 1];
bool sign;

typedef struct
{
    int t_id;
} threadParam_t;

void *threadFunc(void *param)
{
    threadParam_t *p = (threadParam_t *)param;
    int t_id = p->t_id;
    uint32x4_t va_Pas = vmovq_n_u32(0);
    uint32x4_t va_Act = vmovq_n_u32(0);

    while (sign == true)
    {
        int i;
        for (i = 85400; i - 8 >= -1; i -= 8)
        {
            for (int j = t_id; j < 54274; j += NUM_THREADS)
            {
                while (Pas[j][2669] <= i && Pas[j][2669] >= i - 7)
                {
                    int index = Pas[j][2669];

                    if (Act[index][2669] == 1)
                    {
                        int k;
                        for (k = 0; k + 4 <= 2669; k += 4)
                        {
                            va_Pas = vld1q_u32(&(Pas[j][k]));
                            va_Act = vld1q_u32(&(Act[index][k]));
                            va_Pas = veorq_u32(va_Pas, va_Act);
                            vst1q_u32(&(Pas[j][k]), va_Pas);
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

        for (i = i + 8; i >= 0; i--)
        {
            for (int j = t_id; j < 54274; j += NUM_THREADS)
            {
                while (Pas[j][2669] == i)
                {
                    if (Act[i][2669] == 1)
                    {
                        int k;
                        for (k = 0; k + 4 <= 2669; k += 4)
                        {
                            va_Pas = vld1q_u32(&(Pas[j][k]));
                            va_Act = vld1q_u32(&(Act[i][k]));
                            va_Pas = veorq_u32(va_Pas, va_Act);
                            vst1q_u32(&(Pas[j][k]), va_Pas);
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
        if (t_id == 0)
            for (int i = 0; i < NUM_THREADS - 1; ++i)
                sem_wait(&sem_leader);
        else
        {
            sem_post(&sem_leader);         
            sem_wait(&sem_Next[t_id - 1]); 
        }
        if (t_id == 0)
        {
            sign = false;
            for (int i = 0; i < 85401; i++)
            {
                int temp = Pas[i][2669];
                if (temp == -1)
                    continue;
                if (Act[temp][2669] == 0)
                {
                    for (int k = 0; k < 2669; k++)
                        Act[temp][k] = Pas[i][k];
                    Pas[i][2669] = -1;
                    sign = true;
                }
            }
        }
        if (t_id == 0)
            for (int i = 0; i < NUM_THREADS - 1; ++i)
                sem_post(&sem_Next[i]);
    }
    pthread_exit(NULL);
}

int main()
{
    struct timeval start, end;
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
    
    pthread_t *handles = new pthread_t[NUM_THREADS];
    threadParam_t *param = new threadParam_t[NUM_THREADS];
    for (int t_id = 0; t_id < NUM_THREADS; t_id++)
    {
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id], NULL, threadFunc, (void *)&param[t_id]);
    }

    for (int t_id = 0; t_id < NUM_THREADS; t_id++)
        pthread_join(handles[t_id], NULL);

    sem_destroy(&sem_leader);
    sem_destroy(sem_Next);

    gettimeofday(&end, NULL);
    cout << "用时: " << (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0 << "ms" << endl;
}