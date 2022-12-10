#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdbool.h>
#include <signal.h>


#define int long long

int NUMTHREADS = 4;

typedef struct
{
	int output_index_a;
	int output_index_b;

	int length;
	int r_i;
	int r_j;
	int r_k;
} thread_details;

int i, j, k;
int *output_matrix;
int *array_1, *array_2, *counter_1, *counter_2;

void *runner(void *args)
{
	thread_details thread_values = *(thread_details *)args;
	int mul;
	int s_1 = thread_values.r_i * j + thread_values.r_j;
	int s_2 = thread_values.r_k + thread_values.r_j * k;
	for(int x = 0; x < thread_values.length; x++)
	{
		mul = (*(array_1 + s_1)) * (*(array_2 + s_2));
		*(output_matrix + thread_values.output_index_a * k + thread_values.output_index_b) += mul;
		s_1++;
		s_2 += k;
	}

}


int32_t main(int argc, char *argv[])
{
	if(argc != 5)
	{
		fprintf(stderr, "Usage for p2: ./p2 i j k out.txt\n");
		exit(EXIT_FAILURE);
	}
	clock_t start_t, row1_t, end_t;
	start_t = clock();
	double total;
	//arguments sent from P1/scheduler whatever
	i = atoll(argv[1]);
	j = atoll(argv[2]);
	k = atoll(argv[3]);
	output_matrix = malloc(i * k * sizeof(int));
	for(int x = 0; x < i; x++)
	{
		for(int y = 0; y < k; y++)
		{
			*(output_matrix + x * k + y) = 0;
		}
	}

	//ftok to generate unique keys for shm arrays and counter
	key_t arr1, arr2, c1, c2;
	if((arr1 = ftok(".", 1)) == -1 || (arr2 = ftok(".", 2)) == -1 || (c1 = ftok(".", 3)) == -1 || (c2 = ftok(".", 4)) == -1)
	{
		perror("ftok from p2");
		exit(1);
	}

	//indexes read
	int *index_read_1, *index_read_2; // what we did mult till
	index_read_1 = malloc(sizeof(int) * i);
	index_read_2 = malloc(sizeof(int) * k);
	for(int x = 0; x < i; x++)
	{
		index_read_1[x] = 0;
	}

	for(int x = 0; x < k; x++)
	{
		index_read_2[x] = 0;
	}

	FILE *out = fopen("out.txt", "w");
	setvbuf(out, 0, _IOLBF, BUFSIZ);

	if(out == NULL)
	{
		printf("File error\n");
		exit(0); ////////////
	}

	//counters
	int shmid_counter_1 = shmget(c1, i * sizeof(int), 0666); // TODO: check shmflg
	int shmid_counter_2 = shmget(c2, k * sizeof(int), 0666);
	//arrays
	int shmid_array_1 = shmget(arr1, sizeof(int) * i * j, 0666); // the values of the matrix
	int shmid_array_2 = shmget(arr2, sizeof(int) * j * k, 0666);

	if(shmid_counter_1 == -1 || shmid_counter_2 == -1 || shmid_array_1 == -1 || shmid_array_2 == -1)
	{
		perror("shmget from p2");
		exit(1);
	}

	//shared memories declared globally
	

	counter_1 = shmat(shmid_counter_1, NULL, 0); //array of counters of till where p1 read
	counter_2 = shmat(shmid_counter_2, NULL, 0);
	array_1 = shmat(shmid_array_1, NULL, 0);
	array_2 = shmat(shmid_array_2, NULL, 0);
	if(counter_1 == (void *)-1 || counter_2 == (void *)-1 || array_1 == (void *)-1 || array_2 == (void *)-1)
	{
		perror("shmat from p2");
		exit(1);
	}

	pthread_t tid[NUMTHREADS];
	int counter_1_final[i];
	int counter_2_final[k];
	while(*counter_1 != j); // if p1 hasn't read the first row yet

	for (int x = 0; x < i; ++x)
	{
		*(counter_1_final + x) = *(counter_1 + x);
	}
	for (int x = 0; x < k; ++x)
	{
		*(counter_2_final + x) = *(counter_2 + x);
	}

	int total_threads = 0, fir_thread = 0;


	bool mult_checker = true;

	int r = 0;
	while(mult_checker)
	{
	//traversing 2nd matrix columns
		mult_checker = false;

		for (int x = 0; x < i; ++x)
		{
			*(counter_1_final + x) = *(counter_1 + x);
		}
		for (int x = 0; x < k; ++x)
		{
			*(counter_2_final + x) = *(counter_2 + x);
		}

		for(int x = 0; x < k; x++)
		{
			//traversing values across individual columns of 2nd matrix
			thread_details *t_v;
			t_v = malloc(sizeof(thread_details));
			// t_v->num1 = array_1[(r * j) + y]; // TODO: Check for j, k
			// t_v->num2 = array_2[(y * k) + x]; // TODO: Check for j, k
			t_v->length = counter_2_final[x] - index_read_2[x];
			t_v->r_i = r;
			t_v->r_j = index_read_2[x];
			t_v->r_k = x;
			t_v->output_index_a = r;
			t_v->output_index_b = x;

			if(total_threads >= NUMTHREADS)
			{
				//make sure remaining threads finish and continue
				pthread_create(&tid[fir_thread], NULL, runner, t_v);
				pthread_join(tid[fir_thread], NULL);

				if(fir_thread == NUMTHREADS)
				{
					fir_thread = 0;
				}
				else
				{
					fir_thread++;
				}
			}
			else
			{
				pthread_create(&tid[total_threads], NULL, runner, t_v);
				total_threads++;
			}

			index_read_2[x] = counter_2_final[x];
			if(index_read_2[x] < j)
			{
				mult_checker = true;
			}
		}
	}
	row1_t = clock();
	total = (double)(row1_t - start_t);
	for(int rr = 1; rr < i; rr++)
	{
		mult_checker = true;
		while(mult_checker)
		{
			for (int x = 0; x < i; x++)
			{
				*(counter_1_final + x) = *(counter_1 + x);
			}
			for (int x = 0; x < k; ++x)
			{
				*(counter_2_final + x) = *(counter_2 + x);
			}
			mult_checker = false;
			for(int x = 0; x < k; x++)
			{
				
				thread_details *t_v;
				t_v = malloc(sizeof(thread_details));
				t_v->length = counter_1_final[rr] - index_read_1[rr];
				t_v->r_i = rr;
				t_v->r_j = index_read_1[rr];
				t_v->r_k = x;
				t_v->output_index_a = rr;
				t_v->output_index_b = x;
				if(total_threads >= NUMTHREADS)
				{
					//make sure remaining threads finish and continue
					pthread_join(tid[fir_thread], NULL);
					pthread_create(&tid[fir_thread], NULL, runner, t_v);
					if(fir_thread < NUMTHREADS - 1)
					{
						fir_thread++;
					}
					else
					{
						fir_thread = 0;
					}
				}
				else
				{
					pthread_create(&tid[total_threads], NULL, runner, t_v);
					total_threads++;
				}
			}
			index_read_1[rr] = counter_1_final[rr];
			if(index_read_1[rr] < j)
			{
				mult_checker = true;
			}
		}
	}

	for (int i = 0; i < NUMTHREADS; ++i)
	{
		pthread_join(tid[i], NULL);
	}

	printf("mult done\n");
	for(int x = 0; x < i; x++)
	{
		for(int y = 0; y < k; y++)
		{
			fprintf(out, "%lld", *(output_matrix + x * k + y));
			fprintf(out, " ");
		}
		fprintf(out, "\n");
	}
	//reminder to update prev_counter_1_final and 2(needed or not???)

	free(output_matrix);
	free(index_read_1);
	free(index_read_2);
	fclose(out);
	if(shmctl(shmid_array_1, IPC_RMID, NULL) == -1 || shmctl(shmid_array_2, IPC_RMID, NULL) == -1 || shmctl(shmid_counter_1, IPC_RMID, NULL) == -1 || shmctl(shmid_counter_2, IPC_RMID, NULL) == -1)
	{
		perror("shmctl from p2");
		exit(1);
	}
	return 0;
}