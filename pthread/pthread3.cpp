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

sem_t sem_leader;
sem_t* sem_Divsion = new sem_t[NUM_THREADS - 1];
sem_t* sem_Elimination = new sem_t[NUM_THREADS - 1];

void* threadFunc(void* param)
{
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p -> t_id;

	for (int k = 0; k < n; k++)
	{
		if (t_id == 0)
		{
			float32x4_t vt=vmovq_n_f32(m[k][k]);
	    	int j;
			for (j = k + 1; j + 4 <= n; j += 4)
			{
		    	float32x4_t va=vld1q_f32(&(m[k][j]));
				va= vdivq_f32(va,vt);
				vst1q_f32(&(m[k][j]), va);
			}

			for (; j < n; j++)
				m[k][j] = m[k][j] * 1.0 / m[k][k];
			m[k][k] = 1.0;
		}
		else
			sem_wait(&sem_Divsion[t_id - 1]); 

		if (t_id == 0)
		{
			for (int i = 0; i < NUM_THREADS - 1; ++i)
				sem_post(&sem_Divsion[i]);
		}

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


		if (t_id == 0)
		{
			for (int i = 0; i < NUM_THREADS - 1; i++)
				sem_wait(&sem_leader); 

			for (int i = 0; i < NUM_THREADS - 1; i++)
				sem_post(&sem_Elimination[i]);
		}
		else
		{
			sem_post(&sem_leader);
			sem_wait(&sem_Elimination[t_id - 1]); 
		}
	}
	pthread_exit(NULL);
}

int main()
{
	init();
	struct timeval start, end;
	double seconds;
	gettimeofday(&start, NULL);

	sem_init(&sem_leader, 0, 0);
	for (int i = 0; i < NUM_THREADS - 1; i++)
	{
		sem_init(sem_Divsion, 0, 0);
		sem_init(sem_Elimination, 0, 0);
	}

	pthread_t* handles = new pthread_t[NUM_THREADS];
	threadParam_t* param = new threadParam_t[NUM_THREADS];
	for (int t_id = 0; t_id < NUM_THREADS; t_id++)
	{
		param[t_id].t_id = t_id;
		pthread_create(&handles[t_id], NULL, threadFunc, (void*)&param[t_id]);
	}

	for (int t_id = 0; t_id < NUM_THREADS; t_id++)
		pthread_join(handles[t_id], NULL);

	sem_destroy(&sem_leader);
	sem_destroy(sem_Divsion);
	sem_destroy(sem_Elimination);

	gettimeofday(&end, NULL);
	seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0; 
	cout << "用时: " << seconds << " ms" << endl;
}
