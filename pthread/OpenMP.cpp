#include <iostream>
#include <omp.h>
#include <sys/time.h>
#include <arm_neon.h>
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

void openmp()
{
#pragma omp parallel num_threads(NUM_THREADS)
	for (int k = 0; k < n; k++)
	{
#pragma omp single
		{
			float tmp = m[k][k];
			for (int j = k + 1; j < n; j++)
			{
				m[k][j] = m[k][j] / tmp;
			}
			m[k][k] = 1.0;
		}

#pragma omp for schedule(static)
		for (int i = k + 1; i < n; i++)
		{
			float tmp = m[i][k];
			for (int j = k + 1; j < n; j++)
				m[i][j] = m[i][j] - tmp * m[k][j];
			m[i][k] = 0.0;
		}
	}
}

int main()
{
	init();
	struct timeval start, end;
	double seconds;
	gettimeofday(&start, NULL);
	openmp();
	gettimeofday(&end, NULL);
	seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0;
	cout << "用时: " << seconds << " ms" << endl;
}
