#include <unistd.h>
#include <stdio.h>
#define RS 80

int main(int argc, char **argv, char **envp)
{
	dprintf(STDERR_FILENO, "* launched *\n");
	while (*envp)
	{
		dprintf(STDERR_FILENO, "%s\n", *envp);
		++envp;
	}
	ssize_t n;
	char buf[RS];
	while (1)
	{
		n = read(STDIN_FILENO, buf, RS);
		if (n <= 0) { break; }
		n = write(STDOUT_FILENO, buf, n);
		if (n <= 0) { break; }
	}
}
