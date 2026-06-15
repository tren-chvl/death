#include "../include/death.h"


void metamorphe(int fd,char *path)
{
	if (!path)
		path = "";
	(void)fd;
	const char *meta = "META-D34TH\n";
	int r = patch_meta_in_rx_segment(path, meta);
	printf("patch_meta_in_rx_segment = %d\n", r);
}
