#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#ifndef SIZE
#define SIZE 1000 /* Max Size of matrices */
#endif

int A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];

void fill_matrix(int m[SIZE][SIZE])
{
  static int n = 0;
  int i, j;
  for (i = 0; i < SIZE; i++)
    for (j = 0; j < SIZE; j++)
      m[i][j] = n++;
}

void print_matrix(int m[SIZE][SIZE])
{
  int i, j = 0;
  for (i = 0; i < SIZE; i++)
  {
    printf("\n\t| ");
    for (j = 0; j < SIZE; j++)
      printf("%5d ", m[i][j]);
    printf("|");
  }
}
int main(int argc, char **argv)
{
  int numtasks, rank, dest, source, tag;
  int *inmsg, *outmsg;

  double start, end;

  fill_matrix(A);
  fill_matrix(B);

  /**
   * Limpando o matriz
   * */
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    {
      C[i][j] = 0;
    }
  }

  MPI_Status State;
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (numtasks < 2)
  {
    MPI_Abort(MPI_COMM_WORLD, MPI_ERR_SIZE);
  }

  /**
   * Os processos serao sincronizados no primeiro processo
   * */
  if (rank == 0)
  {
    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    inmsg = (int *)malloc(3*sizeof(int));

    start = MPI_Wtime();

    for (int k = 0, r = SIZE * SIZE * (numtasks - 1); k < r; k++)
    {
      MPI_Recv(inmsg, 3, MPI_INT, source, tag, MPI_COMM_WORLD, &State);

      int value = inmsg[0];
      int i = inmsg[1];
      int j = inmsg[2];

#ifdef DEBUG
      printf("\nrank: %d C[%d,%d] = %d + %d\n",
             State.MPI_SOURCE, i, j, C[i][j], value);
#endif

      C[i][j] += value;
    }
    free(inmsg);
  }
  else
  {
    dest = 0;
    tag = rank;
    outmsg = (int *)malloc(3*sizeof(int));

    for (int i = 0; i < SIZE; i++)
    {
      for (int j = 0; j < SIZE; j++)
      {
        outmsg[0] = 0;
        outmsg[1] = i;
        outmsg[2] = j;

        for (int k = rank - 1; k < SIZE; k += numtasks - 1)
        {
          outmsg[0] += A[i][k] * B[k][j];
        }
        MPI_Send(outmsg, 3, MPI_INT, dest, tag, MPI_COMM_WORLD);
      }
    }
    free(outmsg);
  }

  if (rank == 0)
  {
    end = MPI_Wtime();
    printf("%.5f\n", end - start);
#ifdef DEBUG
  print_matrix(C);
#endif
  }

  MPI_Finalize();

  return 0;
}
