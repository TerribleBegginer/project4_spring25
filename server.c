// server.c - Message relay server for rsh
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAX_MSG 256

struct message {
    char source[32];
    char target[32];
    char msg[MAX_MSG];
};

int main() {
    struct message req;
    int server, target;

    unlink("serverFIFO");
    mkfifo("serverFIFO", 0666);

    server = open("serverFIFO", O_RDONLY);
    if (server < 0) {
        perror("open serverFIFO");
        exit(1);
    }

    while (1) {
        // Read the message from client
        if (read(server, &req, sizeof(req)) <= 0) {
            continue;
        }

        // Attempt to open the target FIFO and forward the message
        target = open(req.target, O_WRONLY);
        if (target < 0) {
            perror("Failed to open target FIFO");
            continue;
        }

        write(target, &req, sizeof(req));
        close(target);
    }

    close(server);
    return 0;
}
