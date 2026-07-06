#include "../include/death.h"

int ready_infect(char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;
    off_t size = lseek(fd, 0, SEEK_END);
    char buf[SIGNATURE_BASIC_LEN + 1];
    if (size < (off_t)SIGNATURE_TOTAL_LEN)
    {
        close(fd);
        return 0;
    }
    if (lseek(fd, -(SIGNATURE_LEN + 16), SEEK_END) < 0)
    {
        close(fd);
        return 0;
    }
    if (read(fd, buf,  SIGNATURE_BASIC_LEN) != SIGNATURE_BASIC_LEN)
    {
        close(fd);
        return 0;
    }
    buf[SIGNATURE_BASIC_LEN] = '\0';
    close(fd);
    if (!memcmp(buf, SIGNATURE_BASIC, SIGNATURE_BASIC_LEN))
        return 1;
    return 0;
}

