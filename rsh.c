// rsh.c - Client shell with sendmsg functionality
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_MSG 256

char uName[32];

struct message {
    char source[32];
    char target[32];
    char msg[MAX_MSG];
};

// Function to send a message to another user via server FIFO
void sendmsg(char *user, char *target, char *msg) {
    struct message m;
    strcpy(m.source, user);
    strcpy(m.target, target);
    strcpy(m.msg, msg);

    int fd = open("serverFIFO", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open serverFIFO");
        return;
    }

    write(fd, &m, sizeof(m));
    close(fd);
}

// Thread function to listen for incoming messages on the user's FIFO
void* messageListener(void *arg) {
    int fd;
    struct message m;
    char fifoName[64];
    sprintf(fifoName, "%s", uName);

    fd = open(fifoName, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open user FIFO");
        pthread_exit((void*)1);
    }

    while (1) {
        int bytes = read(fd, &m, sizeof(m));
        if (bytes > 0) {
            printf("Incoming message from %s: %s\n", m.source, m.msg);
            fflush(stdout);
        }
    }

    close(fd);
    pthread_exit((void*)0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <username>\n", argv[0]);
        exit(1);
    }

    strcpy(uName, argv[1]);
    mkfifo(uName, 0666);

    pthread_t tid;
    if (pthread_create(&tid, NULL, messageListener, NULL) != 0) {
        perror("Failed to create message listener thread");
        exit(1);
    }

    char line[256];
    while (1) {
        printf("rsh> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) break;

        line[strcspn(line, "\n")] = '\0';
        char *cmd = strtok(line, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "cd") == 0) {
            char *dir = strtok(NULL, " ");
            if (dir == NULL) {
                fprintf(stderr, "cd: missing argument\n");
                continue;
            }
            chdir(dir);
        } else if (strcmp(cmd, "exit") == 0) {
            unlink(uName);
            exit(0);
        } else if (strcmp(cmd, "help") == 0) {
            printf("cd <dir>\nexit\nhelp\nsendmsg <user> <message>\n");
        } else if (strcmp(cmd, "sendmsg") == 0) {
            char *target = strtok(NULL, " ");
            if (!target) {
                printf("sendmsg: you have to specify target user\n");
                continue;
            }
            char *messageStart = strtok(NULL, "");
            if (!messageStart) {
                printf("sendmsg: you have to enter a message\n");
                continue;
            }
            sendmsg(uName, target, messageStart);
        } else {
            pid_t pid;
            int status;
            char *args[64];
            int i = 0;
            args[i++] = cmd;
            while ((args[i] = strtok(NULL, " ")) != NULL) i++;

            if (posix_spawnp(&pid, cmd, NULL, NULL, args, environ) != 0) {
                perror("posix_spawnp");
                continue;
            }
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
