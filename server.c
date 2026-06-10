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
    exit(1);
}

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr = {0};
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("ERROR opening socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        error("ERROR invalid address");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("Connected to Agent! Enter commands (or 'exit' to quit):\n");

    while (1)
    {
        printf("cmd> ");

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            break;
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0)
            continue;
        if (strcmp(buffer, "exit") == 0)
            break;

        if (write(sockfd, buffer, strlen(buffer)) < 0)
            error("ERROR writing to socket");

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = read(sockfd, buffer, BUFFER_SIZE - 1);

        if (n <= 0)
        {
            printf("\nConnection lost to Agent.\n");
            break;
        }

        printf("%s\n", buffer);
    }

    close(sockfd);
    return 0;
}