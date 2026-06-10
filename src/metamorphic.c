#include "../include/death.h"


void save_state(uint64_t state)
{
	FILE *f = fopen("state.txt", "w");
	if (!f)
		return;
	fprintf(f, "%016" PRIx64, state);
	fclose(f);
}

uint64_t look_state(void)
{
	FILE *f = fopen("state.txt", "r");
	if (!f)
		return 0x12345678;
	uint64_t state = 0;
	fscanf(f, "%" SCNx64, &state);
	fclose(f);
	return state;
}

void generate_metamorphe(int fd)
{
	uint64_t state = look_state();
	state = (state << 5) | (state >> (64 - 5));
	char meta[256] = {0};
	snprintf(meta, sizeof(meta), "META-%016" PRIx64, state);
	write(fd, meta, strlen(meta));
	save_state(state);
}
