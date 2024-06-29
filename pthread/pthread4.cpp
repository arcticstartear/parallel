# include <iostream>
# include <pthread.h>
# include <semaphore.h>
# include <sys/time.h>
# include <arm_neon.h> 
using namespace std;

const int n = 1000;
float m[n][n];
int NUM_THREADS = 7;

void init()
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
			m[i][j] = 0.0;
		m[i][i] = 1.0;
		for (int j = i + 1; j < n; j++)
			m[i][j] = rand();
	}
	for (int k = 0; k < n; k++)
		for (int i = k + 1; i < n; i++)
			for (int j = 0; j < n; j++)
				m[i][j] += m[k][j];
}

typedef struct
{
	int t_id; 
}threadParam_t;

pthread_barrier_t barrier_Divsion;
pthread_barrier_t barrier_Elimination;

void* threadFunc(void* param)
{
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p -> t_id;

	for (int k = 0; k < n; k++)
	{
		float32x4_t vt = vmovq_n_f32(m[k][k]);
		if (t_id == 0)
		{
			int j;
            for (j = k + 1; j + 4 <= n; j += 4)
            {
                float32x4_t va=vld1q_f32(&(m[k][j]) );
                va= vdivq_f32(va,vt);
                vst1q_f32(&(m[k][j]), va);
            }

            for (; j < n; j++)
				m[k][j] = m[k][j] * 1.0 / m[k][k];
			m[k][k] = 1.0;
		}

		pthread_barrier_wait(&barrier_Divsion);

		for (int i = k + 1 + t_id; i < n; i += NUM_THREADS)
		{
			float32x4_t vaik=vmovq_n_f32(m[i][k]);
			int j;
			for (j = k + 1; j + 4 <= n; j += 4)
			{
				float32x4_t vakj=vld1q_f32(&(m[k][j]));
				float32x4_t vaij=vld1q_f32(&(m[i][j]));
				float32x4_t vx=vmulq_f32(vakj,vaik);
				vaij=vsubq_f32(vaij,vx);
				vst1q_f32(&m[i][j], vaij);
			}
			for (; j < n; j++)
				m[i][j] = m[i][j] - m[k][j] * m[i][k];
			m[i][k] = 0.0;
		}

		pthread_barrier_wait(&barrier_Elimination);

	}
	pthread_exit(NULL);
}

int main()
{
	init();
	struct timeval start, end;
    double seconds;
    gettimeofday(&start, NULL);

	pthread_barrier_init(&barrier_Divsion, NULL, NUM_THREADS);
	pthread_barrier_init(&barrier_Elimination, NULL, NUM_THREADS);

	pthread_t* handles = new pthread_t[NUM_THREADS];
	threadParam_t* param = new threadParam_t[NUM_THREADS];
	for (int t_id = 0; t_id < NUM_THREADS; t_id++)
	{
		param[t_id].t_id = t_id;
		pthread_create(&handles[t_id], NULL, threadFunc, (void*)&param[t_id]);
	}

	for (int t_id = 0; t_id < NUM_THREADS; t_id++)
		pthread_join(handles[t_id], NULL);

	pthread_barrier_destroy(&barrier_Divsion);
	pthread_barrier_destroy(&barrier_Elimination);

	gettimeofday(&end, NULL);
    seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0; 
	cout << "用时: " << seconds << " ms" << endl;
}
