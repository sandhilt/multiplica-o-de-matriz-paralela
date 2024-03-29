/*
 * The Game of Life
 *
 * a cell is born, if it has exactly three neighbours 
 * a cell dies of loneliness, if it has less than two neighbours 
 * a cell dies of overcrowding, if it has more than three neighbours 
 * a cell survives to the next generation, if it does not die of loneliness 
 * or overcrowding 
 *
 * In this version, a 2D array of ints is used.  A 1 cell is on, a 0 cell is off.
 * The game plays a number of steps (given by the input), printing to the screen each time.  'x' printed
 * means on, space means off.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
typedef unsigned char cell_t;

cell_t **allocate_board_partial(int width, int height)
{
	cell_t **board = (cell_t **)calloc(width, sizeof(cell_t *));
	int i;
	for (i = 0; i < width; i++)
		board[i] = (cell_t *)calloc(height, sizeof(cell_t));
	return board;
}

cell_t **allocate_board(int size)
{
	return allocate_board_partial(size, size);
}

void free_board(cell_t **board, int size)
{
	int i;
	for (i = 0; i < size; i++)
		free(board[i]);
	free(board);
}

/* return the number of on cells adjacent to the i,j cell */
int adjacent_to(cell_t **board, int width, int height, int i, int j)
{
	int k, l, count = 0;

	int sk = (i > 0) ? i - 1 : i;
	int ek = (i + 1 < height) ? i + 1 : i;
	int sl = (j > 0) ? j - 1 : j;
	int el = (j + 1 < width) ? j + 1 : j;

	/**
	 * TODO Preciso de um recorte da tabela
	 * sk ek
	 * sl el
	 * */

	/**
	 * Pior caso 3 * 3 = 9
	 * */
	for (k = sk; k <= ek; k++)
		for (l = sl; l <= el; l++)
			count += board[k][l];
	count -= board[i][j];

	return count;
}

void print_partial(cell_t **board, int width, int height)
{
	int i, j;
	/* for each row */
	for (j = 0; j < height; j++)
	{
		/* print each column position... */
		for (i = 0; i < width; i++)
			printf("%c", board[i][j] ? 'x' : ' ');
		/* followed by a carriage return */
		printf("\n");
	}
}

void play(cell_t **board, cell_t **newboard, int width, int height, int rank, int numtasks)
{
	int i, j, a;

	int x, y;

	y = rank < numtasks - 1 ? height - 1 : height;
	x = rank > 0 ? 1 : 0;

	if (rank == 0)
	{
		print_partial(board, width, height);
		printf("rank %d width %d height %d (%d,%d).\n", rank, width, height, x, y);
	}

	/* for each cell, apply the rules of Life */
	for (i = 0; i < width; i++)
		for (j = 0; j < height; j++)
		{
			a = adjacent_to(board, width, height, i, j);
			// if (rank == 0 && a > 0)
			// 	printf("rank %d (%2d,%2d)=%d\n", rank, i, j, a);
			if (a == 2)
				newboard[i][j] = board[i][j];
			if (a == 3)
				newboard[i][j] = 1;
			if (a < 2)
				newboard[i][j] = 0;
			if (a > 3)
				newboard[i][j] = 0;
		}

	if (rank == 0)
		print_partial(newboard, width, height);
}

/* print the life board */
void print(cell_t **board, int size)
{
	print_partial(board, size, size);
}

/* read a file into the life board */
void read_file(FILE *f, cell_t **board, int size)
{
	int i, j;
	char *s = (char *)malloc(size + 10);
	char c;
	for (j = 0; j < size; j++)
	{
		/* get a string */
		fgets(s, size + 10, f);
		/* copy the string to the life board */
		for (i = 0; i < size; i++)
		{
			//c=fgetc(f);
			//putchar(c);
			board[i][j] = s[i] == 'x';
		}
		//fscanf(f,"\n");
	}
}

int main(int argc, char **argv)
{
	double inicio, fim;
	int numtasks, rank, root = 0;

	int part, rest, offset = 0, roffset = 0;

	int *sendcount, *displs;
	int *recvcount, *rdispls;

	int i, j, size, steps;
	cell_t **prev, **next, **tmp, **aux;

#ifdef DEBUG
	printf("Initial \n");
	print(prev, size);
	printf("----------\n");
#endif
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (numtasks < 2)
	{
		MPI_Abort(MPI_COMM_WORLD, MPI_ERR_SIZE);
	}

	/**
	 * Rank lee o arquivo
	 * */
	if (rank == root)
	{
		FILE *f = stdin;
		fscanf(f, "%d %d", &size, &steps);
		prev = allocate_board(size);
		read_file(f, prev, size);
		fclose(f);
	}

	/**
	 * Envia o tamanho para os outros
	 * */
	MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	part = size / numtasks;
	rest = size % numtasks;

	sendcount = (int *)calloc(numtasks, sizeof(int));
	displs = (int *)calloc(numtasks, sizeof(int));

	recvcount = (int *)calloc(numtasks, sizeof(int));
	rdispls = (int *)calloc(numtasks, sizeof(int));

	for (i = 0; i < numtasks; i++)
	{
		sendcount[i] = part;

		/**
		 * Resto da divisao fica nas partes do inicio
		 * */
		if (rest > 0)
		{
			sendcount[i]++;
			rest--;
		}

		recvcount[i] = sendcount[i];

		/*
		 * Compartilhando area externa 
		*/
		if (i > 0 && i < numtasks - 1)
		{
			sendcount[i] += 2;
		}
		else if (i == 0)
		{
			sendcount[i]++;
		}

		displs[i] = offset;
		rdispls[i] = roffset;

		/**
		 * Offset tem que comecar na primeira
		 * parte compartilhada
		 * */
		offset += sendcount[i];
		roffset += recvcount[i];

		if (i > 0 && i < numtasks - 1)
		{
			offset -= 2;
		}
		else
		{
			offset--;
		}

		printf("rank %d i=%d S=%2d D=%2d O=%2d\n", rank, i, recvcount[i], rdispls[i], roffset);
	}

	tmp = allocate_board_partial(size, sendcount[rank]);
	next = allocate_board_partial(size, sendcount[rank]);

	if (rank == root)
	{
		aux = allocate_board(size);
		inicio = MPI_Wtime();
		// print(prev, size);
	}
	else
	{
		prev = allocate_board(size);
	}

	for (i = 0; i < size; i++)
	{
		MPI_Scatterv(prev[i], sendcount, displs, MPI_UNSIGNED_CHAR, tmp[i], sendcount[rank], MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);
	}
	free_board(prev, size);

	// print_partial(tmp, size, sendcount[rank]);

	for (i = 0; i < 1; i++)
	{

		play(tmp, next, size, sendcount[rank], rank, numtasks);
		if (rank == 0)
		{
			// print_partial(tmp, size, sendcount[rank]);
			// print_partial(next, size, sendcount[rank]);
			// print(next, size);
		}

		for (i = 0; i < size; i++)
		{
			MPI_Gatherv(next[i], recvcount[rank], MPI_UNSIGNED_CHAR, aux[i], recvcount, rdispls, MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);
		}

		if (rank == root)
		{
			print(aux, size);
		}

		/*
		 * Rank 0 precisa sincronizar as matrizes
		 * */

#ifdef DEBUG
		printf("%d ----------\n", i);
		print(next, size);
#endif
		// if (rank == root)
		// {
		// 	tmp = next;
		// 	next = prev;
		// 	prev = tmp;
		// }
	}
	if (rank == root)
	{
		fim = MPI_Wtime();
		// printf("%.5f\n", fim - inicio);
	}
	MPI_Finalize();
	// print(prev, size);
	// free_board(prev, size);
	// free_board(next, size);
}
