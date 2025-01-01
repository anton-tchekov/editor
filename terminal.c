/* TE: Terminal */
#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

typedef struct
{
	u32 FG, BG;
} TerminalChar;

typedef struct
{
	vec Lines;
	
} Terminal;

static TerminalChar tc_create(u32 c, u32 fg, u32 bg)
{
	return 
}

static u32 tc_char(TerminalChar tc)
{
	return tc.FG & 0xFF;
}

static u32 tc_color(TerminalChar tc)
{
	return tc.FG;
}

static u32 tc_bg(TerminalChar tc)
{
	return tc.BG;
}

static void tc_render(u32 x, u32 y, TerminalChar tc)
{
	render_char(x, y, tc_char(tc), tc_fg(tc), tc_bg(tc));
}

static void te_launch(Terminal *te, char *cmd)
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

	/* Make read non-blocking */
	int retval = fcntl(inpipefd[0], F_SETFL, fcntl(inpipefd[0], F_GETFL) | O_NONBLOCK);
	if(retval)
	{

	}

	for(;;)
	{
		printf("Enter message to send\n");
		scanf("%s", msg);
		if(strcmp(msg, "exit") == 0) break;

		write(outpipefd[1], msg, strlen(msg));
		read(inpipefd[0], buf, 256);

		printf("Received answer: %s\n", buf);
	}

}

static void te_print(Terminal *te)
{

}

static void te_xy(Terminal *te)
{
	
}

static void te_clear(Terminal *te)
{
	
}

static void te_render(Terminal *te)
{

}

static void te_kill()
{
	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
}
