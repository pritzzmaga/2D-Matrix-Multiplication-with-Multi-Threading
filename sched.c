#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	if(argc != 7)
	{
		printf("Usage: ./a.out i j k in1.txt in2.txt out.txt\n");
		exit(1);
	}

	pid_t p1, p2;

	signal(SIGCHLD,SIG_IGN);
	
	p1 = fork();
	if(p1 < 0)
	{
		perror("p1 fork");
	}
	else if(p1 == 0)
	{
		execl("p1", "./p1", argv[1], argv[2], argv[3], argv[4], argv[5], NULL);
	}
	else
	{
		// kill(p1, SIGSTOP);
		p2 = fork();
		if(p2 < 0)
		{
			perror("p2 fork");
		}
		else if(p2 == 0)
		{
			execl("p2", "./p2", argv[1], argv[2], argv[3], argv[6], NULL);
		}
		else
		{
			while(kill(p1, SIGSTOP) != -1 && kill(p2, SIGSTOP) != -1)
			{
				kill(p1, SIGCONT);

				usleep(1000);
				kill(p1, SIGSTOP);

				kill(p2, SIGCONT);
				usleep(1000);
				kill(p2, SIGSTOP);

			}
			// printf("lum\n");
			if(kill(p1, SIGSTOP) == -1)
			{
				kill(p2, SIGCONT);
			}
			else
			{
				kill(p1, SIGCONT);
			}

			// while(kill(p1, SIGCONT) != -1 || kill(p2, SIGCONT) != -1); // to keep the parent process running till both exit
			wait(NULL);
			wait(NULL);
		}
	}
}