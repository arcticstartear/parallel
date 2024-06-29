#include <sys/time.h>
#include <arm_neon.h>
#include <iostream>
using namespace std;
int main()
{
    int N = 100;
    float m[N][N];
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
            m[i][j] = 0.0;
        m[i][i] = 1.0;
        for (int j = i + 1; j < N; j++)
            m[i][j] = rand();
    }
    for (int k = 0; k < N; k++)
        for (int i = k + 1; i < N; i++)
            for (int j = 0; j < N; j++)
                m[i][j] += m[k][j];
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int k = 0; k < N; k++)
    {
        float32x4_t vt = vmovq_n_f32(m[k][k]);
        int j;
        for (j = k + 1; j + 4 <= N; j += 4)
        {
            float32x4_t va = vld1q_f32(&(m[k][j]));
            va = vdivq_f32(va, vt);
            vst1q_f32(&(m[k][j]), va);
        }
        for (; j < N; j++)
            m[k][j] = m[k][j] / m[k][k];
        m[k][k] = 1.0;
        for (int i = k + 1; i < N; i++)
        {
            float32x4_t vaik = vmovq_n_f32(m[i][k]);
            for (j = k + 1; j + 4 <= N; j += 4)
            {
                float32x4_t vakj = vld1q_f32(&(m[k][j]));
                float32x4_t vaij = vld1q_f32(&(m[i][j]));
                float32x4_t vx = vmulq_f32(vakj, vaik);
                vaij = vsubq_f32(vaij, vx);
                vst1q_f32(&m[i][j], vaij);
            }
            for (; j < N; j++)
                m[i][j] = m[i][j] - m[k][j] * m[i][k];
            m[i][k] = 0.0;
        }
    }
    gettimeofday(&end, NULL);
    double seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0;
    cout << "用时: " << seconds << " ms" << endl;
}