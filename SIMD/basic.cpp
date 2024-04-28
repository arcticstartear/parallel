#include <sys/time.h>
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
        for (int j = k + 1; j < N; j++)
            m[k][j] = m[k][j] / m[k][k];
        m[k][k] = 1.0;
        for (int i = k + 1; i < N; i++)
        {
            for (int j = k + 1; j < N; j++)
                m[i][j] = m[i][j] - m[k][j] * m[i][k];
            m[i][k] = 0.0;
        }
    }
    gettimeofday(&end, NULL);
    double seconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000.0;
    cout << "用时: " << seconds << " ms" << endl;
}