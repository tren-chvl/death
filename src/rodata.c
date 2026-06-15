#include "../include/death.h"

int patch_meta_in_rx_segment(const char *path, const char *meta)
{
    printf("\n[DEBUG] patch_meta_in_rx_segment('%s')\n", path);

    FILE *f = fopen(path, "r+b");
    if (!f) {
        printf("[ERROR] fopen failed\n");
        return -1;
    }

    Elf64_Ehdr eh;
    if (fread(&eh, 1, sizeof(eh), f) != sizeof(eh)) {
        printf("[ERROR] fread ELF header failed\n");
        fclose(f);
        return -1;
    }

    if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0 ||
        eh.e_ident[EI_CLASS] != ELFCLASS64) {
        printf("[ERROR] Not a 64-bit ELF\n");
        fclose(f);
        return -1;
    }

    printf("[DEBUG] e_entry = 0x%lx\n", eh.e_entry);

    if (fseek(f, eh.e_phoff, SEEK_SET) != 0) {
        printf("[ERROR] fseek to PHDR failed\n");
        fclose(f);
        return -1;
    }

    if (eh.e_phnum == 0 || eh.e_phnum > 64) {
        printf("[ERROR] Invalid e_phnum = %d\n", eh.e_phnum);
        fclose(f);
        return -1;
    }

    Elf64_Phdr phdr[64];
    if (fread(phdr, sizeof(Elf64_Phdr), eh.e_phnum, f) != (size_t)eh.e_phnum) {
        printf("[ERROR] fread PHDR failed\n");
        fclose(f);
        return -1;
    }

    int rx_idx = -1;
    Elf64_Addr entry = eh.e_entry;

    for (int i = 0; i < eh.e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && (phdr[i].p_flags & PF_X)) {
            Elf64_Addr start = phdr[i].p_vaddr;
            Elf64_Addr end   = phdr[i].p_vaddr + phdr[i].p_memsz;

            printf("[DEBUG] Checking RX segment %d: vaddr 0x%lx - 0x%lx\n",
                   i, start, end);

            if (entry >= start && entry < end) {
                rx_idx = i;
                printf("[DEBUG] -> Selected RX segment %d\n", i);
                break;
            }
        }
    }

    if (rx_idx == -1) {
        printf("[ERROR] No RX segment containing entry point\n");
        fclose(f);
        return -1;
    }

    Elf64_Off rx_end = phdr[rx_idx].p_offset + phdr[rx_idx].p_filesz;
    printf("[DEBUG] rx_end = 0x%lx (offset end of RX segment)\n", rx_end);

    Elf64_Off next_off = (Elf64_Off)-1;
    for (int i = 0; i < eh.e_phnum; i++) {
        if (i == rx_idx)
            continue;
        if (phdr[i].p_offset > rx_end) {
            if (next_off == (Elf64_Off)-1 || phdr[i].p_offset < next_off)
                next_off = phdr[i].p_offset;
        }
    }

    printf("[DEBUG] next segment offset = 0x%lx\n", next_off);

    Elf64_Off limit = next_off;

    if (eh.e_shoff > rx_end && eh.e_shoff < limit) {
        printf("[DEBUG] e_shoff = 0x%lx is before next segment, using it\n", eh.e_shoff);
        limit = eh.e_shoff;
    }

    if (limit == (Elf64_Off)-1) {
        fseek(f, 0, SEEK_END);
        long end = ftell(f);
        limit = (Elf64_Off)end;
        printf("[DEBUG] limit = EOF = 0x%lx\n", limit);
    } else {
        printf("[DEBUG] limit = 0x%lx\n", limit);
    }

    size_t meta_len = strlen(meta);
    printf("[DEBUG] meta_len = %zu\n", meta_len);

    const Elf64_Off align = 0x10;
    Elf64_Off meta_off = (rx_end + (align - 1)) & ~(align - 1);

    printf("[DEBUG] meta_off (aligned) = 0x%lx\n", meta_off);

    if (meta_off + meta_len > limit) {
        printf("[ERROR] Not enough space: meta_end=0x%lx > limit=0x%lx\n",
               meta_off + meta_len, limit);
        fclose(f);
        return -1;
    }

    printf("[DEBUG] Writing meta at file offset 0x%lx\n", meta_off);

    if (fseek(f, meta_off, SEEK_SET) != 0) {
        printf("[ERROR] fseek to meta_off failed\n");
        fclose(f);
        return -1;
    }

    if (fwrite(meta, 1, meta_len, f) != meta_len) {
        printf("[ERROR] fwrite meta failed\n");
        fclose(f);
        return -1;
    }

    Elf64_Off new_end = meta_off + meta_len;
    phdr[rx_idx].p_filesz = new_end - phdr[rx_idx].p_offset;
    phdr[rx_idx].p_memsz  = phdr[rx_idx].p_filesz;

    printf("[DEBUG] New p_filesz = 0x%lx\n", phdr[rx_idx].p_filesz);
    printf("[DEBUG] New p_memsz  = 0x%lx\n", phdr[rx_idx].p_memsz);

    if (fseek(f, eh.e_phoff, SEEK_SET) != 0) {
        printf("[ERROR] fseek rewrite PHDR failed\n");
        fclose(f);
        return -1;
    }

    if (fwrite(phdr, sizeof(Elf64_Phdr), eh.e_phnum, f) != (size_t)eh.e_phnum) {
        printf("[ERROR] fwrite PHDR failed\n");
        fclose(f);
        return -1;
    }

    printf("[DEBUG] Patch applied successfully\n");

    fclose(f);
    return 0;
}

