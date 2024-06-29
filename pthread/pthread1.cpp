#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include <arm_neon.h>
using namespace std;

const int n = 1000;
float m[n][n];
int worker_count = 7;

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
	int k;
	int t_id;
} threadParam_t;

void *threadFunc(void *param)
{
	threadParam_t *p = (threadParam_t *)param;
	int k = p->k;		  
	int t_id = p->t_id;	  
	int l = k + t_id + 1; 

	for (int i = k + 1 + t_id; i < n; i += worker_count)
	{
		float32x4_t vaik = vmovq_n_f32(m[l][k]);
		int j;
		for (j = k + 1; j + 4 <= n; j += 4)
		{
			float32x4_t vakj = vld1q_f32(&(m[k][j]));
			float32x4_t vaij = vld1q_f32(&(m[l][j]));
			float32x4_t vx = vmulq_f32(vakj, vaik);
			vaij = vsubq_f32(vaij, vx);
			vst1q_f32(&m[l][j], vaij);
		}
		for (; j < n; j++)
			m[i][j] = m[i][j] - m[k][j] * m[i][k];
		m[i][k] = 0.0;
	}

	pthread_exit(NULL);
}

int main()
{
	init();
	struct timeval start, end;
	double seconds;
	gettimeofday(&start, NULL); 

	for (int k = 0; k < n; k++)
	{
		float32x4_t vt = vmovq_n_f32(m[k][k]);
		int j;
		for (j = k + 1; j + 4 <= n; j += 4)
		{
			float32x4_t va = vld1q_f32(&(m[k][j]));
			va = vdivq_f32(va, vt);
			vst1q_f32(&(m[k][j]), va);
		}
		for (; j < n; j++)
			m[k][j] = m[k][j] * 1.0 / m[k][k];
		m[k][k] = 1.0;

		pthread_t *handles = new pthread_t[worker_count];		
		threadParam_t *param = new threadParam_t[worker_count]; 

		for (int t_id = 0; t_id < worker_count; t_id++)
		{
			param[t_id].k = k;
			param[t_id].t_id = t_id;
		}

		for (int t_id = 0; t_id < worker_count; t_id++)
			pthread_create(&handles[t_id], NULL, threadFunc, (void *)&param[t_id]);

		for (int t_id = 0; t_id < worker_count; t_id++)
			pthread_join(handles[t_id], NULL);
	}

	gettimeofday(&end, NULL);
	seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0; 
	cout << "用时: " << seconds << " ms" << endl;
}
