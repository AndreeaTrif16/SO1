#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define BASE_DIR "./treasure_hunts"
#define LOG_FILE "logged_hunt"
#define TREASURE_FILE "treasures.dat"

struct Treasure {
    int treasure_id;
    char user_name[50];
    float latitude;
    float longitude;
    char clue[200];
    int value;
};

void create_hunt_directory(const char *hunt_id) {
    char hunt_path[256];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s", BASE_DIR, hunt_id);

    if (mkdir(BASE_DIR, 0777) == -1 && errno != EEXIST) {
        perror("Error creating base directory");
        exit(1);
    }

    if (mkdir(hunt_path, 0777) == -1 && errno != EEXIST) {
        perror("Error creating hunt directory");
        exit(1);
    }
}

void log_action(const char *hunt_id, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s/%s", BASE_DIR, hunt_id, LOG_FILE);

    int fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Error opening log file");
        return;
    }
    dprintf(fd, "%s\n", action);
    close(fd);

    char symlink_path[256];
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);
    unlink(symlink_path);
    symlink(log_path, symlink_path);
}

void add_treasure(const char *hunt_id) {
    create_hunt_directory(hunt_id);

    struct Treasure t;
    printf("Enter Treasure ID: "); scanf("%d", &t.treasure_id);
    printf("Enter Username: "); scanf("%s", t.user_name);
    printf("Enter Latitude: "); scanf("%f", &t.latitude);
    printf("Enter Longitude: "); scanf("%f", &t.longitude);
    getchar();  

    printf("Enter Clue: "); fgets(t.clue, sizeof(t.clue), stdin);

    t.clue[strcspn(t.clue, "\n")] = 0;  
    printf("Enter Value: "); scanf("%d", &t.value);

    char treasure_path[256];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", BASE_DIR, hunt_id, TREASURE_FILE);

    int fd = open(treasure_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }
    if (write(fd, &t, sizeof(struct Treasure)) != sizeof(struct Treasure)) {
        perror("Error writing to treasure file");
    }
    close(fd);

    log_action(hunt_id, "Added a treasure");
    printf("Treasure added successfully!\n");
}


void list_treasures(const char *hunt_id) {
    char treasure_path[256];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", BASE_DIR, hunt_id, TREASURE_FILE);

    int fd = open(treasure_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    struct Treasure t;
    printf("Treasure List:\n");
    while (read(fd, &t, sizeof(struct Treasure)) == sizeof(struct Treasure)) {
        printf("ID: %d, User: %s, Location: (%.2f, %.2f), Clue: %s, Value: %d\n",
               t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
}


void view_treasure(const char *hunt_id, int treasure_id) {
    char treasure_path[256];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", BASE_DIR, hunt_id, TREASURE_FILE);

    int fd = open(treasure_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    struct Treasure t;
    while (read(fd, &t, sizeof(struct Treasure)) == sizeof(struct Treasure)) {
        if (t.treasure_id == treasure_id) {
            printf("Treasure Found: ID: %d, User: %s, Location: (%.2f, %.2f), Clue: %s, Value: %d\n",
                   t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }
    printf("Treasure not found.\n");
    close(fd);
}


void remove_hunt(const char *hunt_id) {
    char hunt_path[256];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s", BASE_DIR, hunt_id);

    
    DIR *dir = opendir(hunt_path);
    if (!dir) {
        perror("Error opening hunt directory");
        return;
    }

    struct dirent *entry;
    char file_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", hunt_path, entry->d_name);
        if (remove(file_path) == -1) {
            perror("Error deleting file");
        }
    }
    closedir(dir);

    if (rmdir(hunt_path) == -1) {
        perror("Error deleting hunt directory");
        return;
    }

    char symlink_path[256];
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);
    unlink(symlink_path);

    printf("Hunt '%s' removed successfully!\n", hunt_id);
}

void remove_treasure(const char *hunt_id, int treasure_id) {
    char treasure_path[256], temp_path[256];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", BASE_DIR, hunt_id, TREASURE_FILE);
    snprintf(temp_path, sizeof(temp_path), "%s/%s/temp.dat", BASE_DIR, hunt_id);

    int fd = open(treasure_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        perror("Error creating temp file");
        close(fd);
        return;
    }

    struct Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(struct Treasure)) == sizeof(struct Treasure)) {
        if (t.treasure_id == treasure_id) {
            found = 1;
            continue; 
        }
        write(temp_fd, &t, sizeof(struct Treasure));
    }

    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Treasure with ID %d not found in hunt '%s'.\n", treasure_id, hunt_id);
        remove(temp_path); 
        return;
    }

    if (rename(temp_path, treasure_path) == -1) {
        perror("Error replacing old treasure file");
    } else {
        printf("Treasure with ID %d removed successfully from hunt '%s'.\n", treasure_id, hunt_id);
        log_action(hunt_id, "Removed a treasure");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s --command hunt_id [args]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else {
        printf("Invalid command\n");
    }
    return 0;
}