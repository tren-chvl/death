#include "../include/death.h"

unsigned char get_seed(char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return 1;
	unsigned char c = 0;
	fread(&c, 1, 1, f);
	fclose(f);
	if (c == 0)
		return 1;
	return c;
}
void metamorphe(size_t stub_size, unsigned char *stub, unsigned char seed)
{
	unsigned int nb = seed;

	for (size_t i = stub_size - 1; i > 0; i--)
	{
		nb = nb * 1103515245 + 123435;
		size_t j = nb % (i + 1);
		unsigned char tmp = stub[i];
		stub[i] = stub[j];
		stub[j] = tmp;
	}
}
