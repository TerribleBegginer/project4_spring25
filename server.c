#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

struct message {
    char source[50];
    char target[50];
    char msg[200];
};

void terminate(int sig) {
    printf("Exiting....\n");
    fflush(stdout);
    exit(0);
}

int main() {
    int server, dummyfd;
    struct message req;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, terminate);

    server = open("serverFIFO", O_RDONLY);
    dummyfd = open("serverFIFO", O_WRONLY); // keep open for writing

    while (1) {
        int n = read(server, &req, sizeof(struct message));
        if (n <= 0) continue;

        printf("Received a request from %s to send the message %s to %s.\n",
               req.source, req.msg, req.target);
        fflush(stdout);

        int targetfd = open(req.target, O_WRONLY);
        if (targetfd >= 0) {
            write(targetfd, &req, sizeof(struct message));
            close(targetfd);
        }
    }

    close(server);
    close(dummyfd);
    return 0;
}
