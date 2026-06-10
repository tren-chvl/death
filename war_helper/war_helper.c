#include "../include/death.h"  

void fingerprint_one(const char *path)
{
	char old[FP_LEN + 1] = {0};
	char tab[FP_LEN + 1] = {0};
	unsigned int value;

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return;

	off_t off = -(SIGNATURE_LEN - FP_OFFSET - 1);
	if (lseek(fd, off, SEEK_END) < 0 || read(fd, old, FP_LEN) != FP_LEN) 
	{
		close(fd);
		return;
	}
	close(fd);
	value = strtoul(old, NULL, 16);
	value++;
	snprintf(tab, FP_LEN + 1, "%08X", value);
	char tmp_path[512];
	snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
	int fd_in = open(path, O_RDONLY);
	if (fd_in < 0)
		return;
	struct stat st;
	fstat(fd_in, &st);
	int fd_out = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
	if (fd_out < 0) 
	{
		close(fd_in);
		return;
	}
	char buf[4096];
	ssize_t n;
	while ((n = read(fd_in, buf, sizeof(buf))) > 0)
	if (write(fd_out, buf, n) != n)
		break;
	close(fd_in);
	if (lseek(fd_out, off, SEEK_END) >= 0)
	{
		if (write(fd_out, tab, FP_LEN) != FP_LEN)
    		return;
	}
	close(fd_out);
	rename(tmp_path, path);
}

int main(int ac, char **av)
{
	if (ac != 2)
		return 1;
	fingerprint_one(av[1]);
	return 0;
}