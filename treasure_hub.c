#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

pid_t monitor_pid = -1;
int monitor_running = 0;

void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("\nMonitor process ended.\n");
        monitor_running = 0;
    }
}

void send_command(const char* command) {
    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    write(fd, command, strlen(command));
    close(fd);

    kill(monitor_pid, SIGUSR1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    char input[256];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running.\n");
                continue;
            }

            monitor_pid = fork();

            if (monitor_pid == -1) {
                perror("fork");
                exit(1);
            }

            if (monitor_pid == 0) {
                execl("./treasure_manager", "treasure_manager", "--monitor", NULL);
                perror("execl");
                exit(1);
            }

            monitor_running = 1;
            printf("Monitor started with PID %d.\n", monitor_pid);

        } 
        else if (strcmp(input, "list_hunts") == 0) {
                if (!monitor_running) {
                    printf("Error: Monitor not running!\n");
                    continue;
                }
                send_command("list_hunts");
            } 
        else if (strcmp(input, "stop_monitor") == 0) {
                if (!monitor_running) {
                    printf("Error: Monitor not running!\n");
                    continue;
                }
                send_command("stop_monitor");
                printf("Stopping monitor...\n");

            } 
        else if (strcmp(input, "exit") == 0) {
                if (monitor_running) {
                    printf("Error: Monitor is still running!\n");
                } else {
                    printf("Exiting hub.\n");
                    break;
                }
            } 
        else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
