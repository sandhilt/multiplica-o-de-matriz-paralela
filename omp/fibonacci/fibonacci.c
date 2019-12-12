#include <stdio.h>
#include <omp.h>

#define N 2000

void main(int argc, char **argv)
{
    int F[N];
    double inicio, fim, duracao;

    const int T = atoi(argv[1]);

    int i, j;
    int sum;

    F[0] = 1;
    F[1] = 1;
    sum = 0;

    inicio = omp_get_wtime();
#pragma omp parallel private(i, j) shared(F) num_threads(T)
    {
#pragma omp sections
        {
#pragma omp section
            {
#pragma omp critial for ordered
                for (i = 2; i < N; i++)
                {
                    F[i] = F[i - 2] + F[i - 1];
                }
            }
#pragma omp section
            {
#pragma omp parallel for reduction(+ \
                                   : sum)
                for (j = 0; j < N; j++)
                {
                    sum += F[j];
                }
            }
        }
    }
#pragma omp barrier
    {
        fim = omp_get_wtime();
        duracao = fim - inicio;

#ifdef DEBUG
        printf("Soma dos %d termos de fibonacci eh %d\n", N, sum);
#endif
        printf("%.5f\n", duracao);
    }
}