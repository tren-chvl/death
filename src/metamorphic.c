#include "../include/death.h"

#define COMMENT_OFFSET 0x3018
#define COMMENT_SIZE   0x1F

void patch_comment(const char *binary_path, const char *meta)
{
    FILE *f = fopen(binary_path, "r+b");
    if (!f)
        return;

    fseek(f, COMMENT_OFFSET, SEEK_SET);

    char buf[COMMENT_SIZE] = {0};
    strncpy(buf, meta, COMMENT_SIZE);

    fwrite(buf, 1, COMMENT_SIZE, f);
    fclose(f);
}
void save_state(t_meta *m)
{
	FILE *f = fopen("state.bin", "wb");
	if (!f)
		return;
	fwrite(m, sizeof(t_meta), 1, f);
	fclose(f);
}

void evolve_meta(t_meta *m)
{
	m->counter++;
	m->fingerprint = (m->fingerprint << 5) | (m->fingerprint >> (64 - 5));
	m->flags ^= 0xA5A5A5A5;
}

t_meta load_state(void)
{
	t_meta m = {0};
	FILE *f = fopen("state.bin", "rb");
	if (!f)
	{
		m.fingerprint = 0x12345678;
		m.counter = 1;
		m.version = 1;
		m.flags = 0;
		return m;
	}
	fread(&m, sizeof(t_meta), 1, f);
	fclose(f);
	return m;
}


void write_metamorphe(int fd, t_meta *m)
{
	char meta[256];
		snprintf(meta, sizeof(meta),
    "META-%016" PRIx64 "-C%" PRIu64 "-V%u-F%08x\n",
    m->fingerprint, m->counter, m->version, m->flags);
	write(fd, meta, strlen(meta));
}

uint64_t hash_name(const char *path)
{
	uint64_t hash = 1469598103934665603ULL;
	uint64_t prime = 1099511628211ULL;
	for (size_t i = 0; path[i];i++)
	{
		hash ^= (uint64_t)path[i];
		hash *= prime;
	}
	return hash;
}




void metamorphe(int fd, char *path)
{
    if (!path)
        path = "";

    t_meta m = load_state();
    evolve_meta(&m);

    uint64_t hash = hash_name(path);
    m.fingerprint ^= hash;
    write_metamorphe(fd, &m);
    char meta[256];
    snprintf(meta, sizeof(meta),
        "META-%016" PRIx64 "-C%" PRIu64 "-V%u-F%08x",
        m.fingerprint, m.counter, m.version, m.flags);
    patch_comment(path, meta);
	patch_meta_in_rx_segment("/home/marc/git_ub/42cursus/death/Death", meta);
    save_state(&m);
}
