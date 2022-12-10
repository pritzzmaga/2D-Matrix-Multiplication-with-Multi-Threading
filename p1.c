#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>


#define int long long

int THREADS_NUMBER = 4;

int i, j, k;
int *headPtr1, *headPtr2;
int *c1,*c2;

char *file1, *file2; // to store the file names

int *location1, *location2;

void filemilind(int *location, FILE *fp)
{
	int curr = 0;
	int start = 0;
	int counter = 0;
	while(1)
	{
		char a = fgetc(fp);
		if(a == '\n')
		{
			location[counter] = start;
			++counter;
			start = curr + 1;
		}
		curr++;
		if(a == EOF)
		{
			location[counter] = start;
			break;
		}
	}
}

void *readrunner2(void *args)
{
	int thread_number = *(int *)args;
	FILE *in2 = fopen(file2, "r"); // not in2.txt

	const int numread2 = k / THREADS_NUMBER; // k not j since we are reading columns
	const int rem2 = k % THREADS_NUMBER;

	int start2, end2;

	if (thread_number == 0)
	{
		start2 = 0;
		end2 = numread2 + rem2;
	}
	else
	{
		start2 = numread2 * thread_number + rem2;
		end2 = numread2 * (thread_number + 1) + rem2;
	}

	fseek(in2, location2[start2], SEEK_CUR);
	// read2 = row number
	for(int read2 = start2; read2 < end2; ++read2) // i'm at (p, read2), i want it at (read2, p)
	{
		for(int p = 0; p < j; ++p)
		{
			fscanf(in2, "%lld", headPtr2 + read2 + p * k);
			*(c2 + read2) += 1;
		}
	}
	fclose(in2);
}

void *readrunner1(void *args)
{
	int thread_number = *(int *)args;
	FILE *in1 = fopen(file1, "r");

	const int numread1 = i / THREADS_NUMBER;
	const int rem1 = i % THREADS_NUMBER;

	int start1, end1;

	if(thread_number == 0)
	{
		start1 = 1;
		end1 = numread1 + rem1;
	}
	else
	{
		start1 = numread1 * thread_number + rem1;
		end1 = numread1 * (thread_number + 1) + rem1;
	}

	fseek(in1, location1[start1], SEEK_CUR);
	// read1 = row number
	for(int read1 = start1; read1 < end1; ++read1)
	{
		for(int p = 0; p < j; ++p)
		{
			fscanf(in1, "%lld", headPtr1 + (read1 * j + p));
			*(c1 + read1) += 1;
		}
	}
	fclose(in1);
}

int32_t main(int argc, char *argv[])
{
	if(argc != 6)
	{
		printf("Usage for p1: ./p1 i j k in1.txt in2.txt\n");
		exit(1);
	}

	i = atoll(argv[1]), j = atoll(argv[2]), k = atoll(argv[3]);
	file1 = malloc(sizeof(argv[4]));
	file2 = malloc(sizeof(argv[5]));
	file1 = argv[4];
	file2 = argv[5];

	key_t key1, key2, keyc1, keyc2;
	if((key1 = ftok(".", 1)) == -1 || (key2 = ftok(".", 2)) == -1 || (keyc1 = ftok(".", 3)) == -1 || (keyc2 = ftok(".", 4)) == -1)
	{
		perror("ftok from p1");
		exit(1);
	}

	int shmid1 = shmget(key1, i * j * sizeof(int), 0666 | IPC_CREAT);
	int shmid2 = shmget(key2, j * k * sizeof(int), 0666 | IPC_CREAT);
	int shmcid1 = shmget(keyc1, sizeof(int) * i, 0666 | IPC_CREAT);
	int shmcid2 = shmget(keyc2, sizeof(int) * k, 0666 | IPC_CREAT);
	if(shmid1 == -1 || shmid2 == -1 || shmid1 == -1 || shmcid2 == -1)
	{
		perror("shmget from p1");
		exit(1);
	}

	headPtr1 = shmat(shmid1, NULL, 0);
	headPtr2 = shmat(shmid2, NULL, 0);
	c1 = shmat(shmcid1, NULL, 0);
	c2 = shmat(shmcid2, NULL, 0);
	if(headPtr1 == (void *)-1 || headPtr2 == (void *)-1 || c1 == (void *)-1 || c2 == (void *)-1)
	{
		perror("shmat from p1");
		exit(1);
	}

	location1 = malloc(sizeof(int) * (i + 1));
	location2 = malloc(sizeof(int) * (k + 1));

	FILE *in1 = fopen(argv[4], "r");
	FILE *in2 = fopen(argv[5], "r");
	filemilind(location1, in1);
	filemilind(location2, in2);
	fclose(in2);

	// Reading the first row of the first matrix
	fseek(in1, 0, SEEK_SET);
	for(int p = 0; p < j; ++p)
	{
		fscanf(in1, "%lld", headPtr1 + p);
		*c1 += 1;
	}
	fclose(in1);

	pthread_t thread_number[THREADS_NUMBER];
	int tnum[THREADS_NUMBER];

	// Reading the whole of 2nd matrix
	for(int p = 0; p < THREADS_NUMBER; ++p)
	{
		tnum[p] = p;
		if(pthread_create(&thread_number[p], NULL, readrunner2, (void *)&tnum[p]) != 0)
		{
			perror("pthread_create from p1");
			exit(1);
		}
	}

	for(int p = 0; p < THREADS_NUMBER; ++p)
	{
		if(pthread_join(thread_number[p], NULL) != 0)
		{
			perror("pthread_join from p1");
			exit(1);
		}
	}

	// Reading whole of 1st matrix except 1st row
	for(int p = 0; p < THREADS_NUMBER; ++p)
	{
		tnum[p] = p;
		if(pthread_create(&thread_number[p], NULL, readrunner1, (void *)&tnum[p]) != 0)
		{
			perror("pthread_create from p1");
			exit(1);
		}
	}

	for(int p = 0; p < THREADS_NUMBER; ++p)
	{
		if(pthread_join(thread_number[p], NULL) != 0)
		{
			perror("pthread_join from p1");
			exit(1);
		}
	}

	free(location1);
	free(location2);
	return 0;
}