/* Previously used, currently unused code */
static size_t revstrlen(char *p)
{
	size_t cnt;

	cnt = 0;
	--p;
	do
	{
		--p;
		++cnt;
	}
	while(*p);
	return cnt;
}

static void path_dir(char *s)
{
	u32 c;
	char *slash;

	slash = s;
	for(; (c = *s); ++s)
	{
		if(c == '/')
		{
			slash = s;
		}
	}

	strcpy(slash, "/");
}

static u32 starts_with_ic(char *str, char *prefix)
{
	while(*prefix)
	{
		if(tolower(*prefix++) != tolower(*str++))
		{
			return 0;
		}
	}

	return 1;
}

static void reverse(char *s, u32 len)
{
	u32 i, j, c;

	for(i = 0, j = len - 1; i < j; ++i, --j)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static void rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
	u32 x0;
	u32 *line;

	assert(x < _gfx_width);
	assert(y < _gfx_height);
	assert(w < _gfx_width);
	assert(h < _gfx_height);
	assert((x + w) <= _gfx_width);
	assert((y + h) <= _gfx_height);

	line = _pixels + y * _gfx_width + x;
	while(h)
	{
		for(x0 = 0; x0 < w; ++x0)
		{
			line[x0] = color;
		}

		line += _gfx_width;
		--h;
	}
}

static void *_calloc(size_t num, size_t size)
{
	void *p = calloc(num, size);
	if(!p) { _alloc_fail(num * size); }
	++_alloc_cnt;
	return p;
}

static u32 is_cursor(u32 x, u32 y)
{
	return y == _vcursor.y && x == _vcursor.x;
}

static u32 is_text(u8 *s, size_t len)
{
	for(u8 *end = s + len; s < end; ++s)
	{
		u32 c = *s;
		if(!isprint(c) && c != '\n' && c != '\t')
		{
			return 0;
		}
	}

	return 1;
}

static char *padchr(char *s, u32 c, u32 count)
{
	while(count)
	{
		--count;
		*s++ = c;
	}

	return s;
}

static char *append(char *dst, char *src, size_t count)
{
	memcpy(dst, src, count);
	return dst + count;
}

/* TE: Terminal */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

typedef struct
{
	u32 FG, BG;
} TerminalChar;

typedef struct
{
	vec Lines;
	pid_t PID;
	
} Terminal;

static TerminalChar tc_create(u32 c, u32 fg, u32 bg)
{
	TerminalChar tc = { c | (fg & ~0xFF), bg};
	return tc;
}

static u32 tc_char(TerminalChar tc)
{
	return tc.FG & 0xFF;
}

static u32 tc_fg(TerminalChar tc)
{
	return tc.FG | 0xFF;
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

static void te_kill(Terminal *te)
{
	int status;
	kill(te->PID, SIGKILL);
	waitpid(te->PID, &status, 0);
}

static void te_key(Terminal *te, u32 key, u32 chr)
{

}

static char *str_heapcopy(char *s)
{
	size_t len  = strlen(s) + 1;
	char *p = _malloc(len);
	memcpy(p, s, len);
	return p;
}
