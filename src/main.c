#define _GNU_SOURCE
#include "../include/death.h"
#include <sys/mman.h>
#include <linux/memfd.h>


int main(void)
{
	if (anti_process("test"))
		return 1;
	// if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
	// 	return 1;
	death();
	return(0);
}