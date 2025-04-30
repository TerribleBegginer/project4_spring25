#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {
    "cp", "touch", "mkdir", "ls", "pwd", "cat", "grep", "chmod", "diff",
    "cd", "exit", "help", "sendmsg"
};

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

void sendmsg(char *target, char *msg) {
    struct message m;
    strcpy(m.source, uName);
    strcpy(m.target, target);
    strcpy(m.msg, msg);

    int fd = open("serverFIFO", O_WRONLY);
    if (fd >= 0) {
        write(fd, &m, sizeof(struct message));
        close(fd);
    }
}

void* messageListener(void *arg) {
    int fd;
    struct message m;
    char fifoName[50];
    strcpy(fifoName, uName);

    fd = open(fifoName, O_RDONLY);
    if (fd < 0) {
        perror("open user FIFO failed");
        pthread_exit(NULL);
    }

    while (1) {
        int bytes = read(fd, &m, sizeof(struct message));
        if (bytes > 0) {
            printf("Incoming message from %s: %s\n", m.source, m.msg);
            fflush(stdout);
        }
    }

    close(fd);
    pthread_exit(NULL);
}

int isAllowed(const char *cmd) {
    for (int i = 0; i < N; i++) {
        if (strcmp(cmd, allowed[i]) == 0) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: ./rsh <username>\n");
        exit(1);
    }

    strcpy(uName, argv[1]);
    signal(SIGINT, terminate);

    pthread_t tid;
    pthread_create(&tid, NULL, messageListener, NULL);

    char line[256];

    while (1) {
        fprintf(stderr, "rsh>");
        if (fgets(line, sizeof(line), stdin) == NULL) continue;
        if (strcmp(line, "\n") == 0) continue;

        line[strcspn(line, "\n")] = 0;

        char *cmd = strtok(line, " ");
        if (!cmd || !isAllowed(cmd)) {
            printf("NOT ALLOWED!\n");
            continue;
        }

        if (strcmp(cmd, "sendmsg") == 0) {
            char *target = strtok(NULL, " ");
            if (!target) {
                printf("sendmsg: you have to specify target user\n");
                continue;
            }
            char *msg = strtok(NULL, "");
            if (!msg) {
                printf("sendmsg: you have to enter a message\n");
                continue;
            }
            sendmsg(target, msg);
            continue;
        }

        if (strcmp(cmd, "exit") == 0) {
            terminate(SIGINT);
        } else if (strcmp(cmd, "cd") == 0) {
            char *dir = strtok(NULL, " ");
            if (strtok(NULL, " ") != NULL) {
                printf("-rsh: cd: too many arguments\n");
            } else {
                if (dir == NULL) dir = getenv("HOME");
                if (chdir(dir) != 0) {
                    perror("cd failed");
                }
            }
        } else if (strcmp(cmd, "help") == 0) {
            printf("The allowed commands are:\n");
            for (int i = 0; i < N; i++) {
                printf("%d: %s\n", i + 1, allowed[i]);
            }
        } else {
            // Execute external command
            char *args[21];
            int argc = 0;
            args[argc++] = cmd;

            char *arg = strtok(NULL, " ");
            while (arg && argc < 20) {
                args[argc++] = arg;
                arg = strtok(NULL, " ");
            }
            args[argc] = NULL;

            pid_t pid;
            posix_spawnattr_t attr;
            posix_spawnattr_init(&attr);
            if (posix_spawnp(&pid, cmd, NULL, &attr, args, environ) != 0) {
                perror("spawn failed");
            } else {
                waitpid(pid, NULL, 0);
            }
            posix_spawnattr_destroy(&attr);
        }
    }

    return 0;
}
