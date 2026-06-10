#include "death.h"

int process_continue(char *target, char *pid)
{
	char	path[256];
	char	comm[256] = {0};

	snprintf(path, sizeof(path), "/proc/%s/comm", pid);
	FILE *f = fopen(path, "r");
	if (!f)
		return 0;
	if (!fgets(comm, sizeof(comm), f)) 
	{
		fclose(f);
		return 0;
	}
	fclose(f);
	comm[strcspn(comm, "\n")] = 0;
	return strcmp(comm, target) == 0;
}

int anti_process(char *path)
{
	DIR *dir = opendir("/proc");
	if (!dir)
		return 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		int is_pid = 1;
		for (int i = 0; entry->d_name[i]; i++) 
		{
			if (!isdigit((unsigned char)entry->d_name[i])) 
			{
				is_pid = 0;
				break;
			}
		}
		if (!is_pid)
			continue;
		if (process_continue(path, entry->d_name)) 
		{
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);
	return 0;
}
