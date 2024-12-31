#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

void launch(char *cmd)
{
	int inpipefd[2];
	int outpipefd[2];
	char buf[256];
	char msg[256];
	int status;

	pipe(inpipefd);
	pipe(outpipefd);
	pid_t pid = fork();
	if(pid == 0)
	{
		dup2(outpipefd[0], STDIN_FILENO);
		dup2(inpipefd[1], STDOUT_FILENO);
		dup2(inpipefd[1], STDERR_FILENO);
		prctl(PR_SET_PDEATHSIG, SIGTERM);
		close(outpipefd[1]);
		close(inpipefd[0]);
		execl(cmd, "tee", (char *)NULL);
		exit(1);
	}

	close(outpipefd[0]);
	close(inpipefd[1]);

	for(;;)
	{
		printf("Enter message to send\n");
		scanf("%s", msg);
		if(strcmp(msg, "exit") == 0) break;

		write(outpipefd[1], msg, strlen(msg));
		read(inpipefd[0], buf, 256);

		printf("Received answer: %s\n", buf);
	}

	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
}
