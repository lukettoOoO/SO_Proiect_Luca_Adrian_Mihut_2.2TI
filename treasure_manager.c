#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#define MAX_USERNAME_LEN 32
#define MAX_CLUE_LEN 256

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;

void invalid_input_err() 
{
    fprintf(stderr, "(main): Invalid arguments!\n");
    exit(EXIT_FAILURE);
}

void add_hunt(char *hunt_id) 
{
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';
    if (mkdir(path, 0777) == -1) 
    {
        perror("(add_hunt - mkdir)");
        exit(EXIT_FAILURE);
    }

    char treasure_file[128];
    memset(treasure_file, 0, sizeof(treasure_file));
    strcat(treasure_file, path);
    strcat(treasure_file, "/treasure.dat");
    treasure_file[strlen(treasure_file)] = '\0';
    int treasure_fd = open(treasure_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (treasure_fd == -1) 
    {
        perror("(add_hunt - open treasure.dat)");
        exit(EXIT_FAILURE);
    }
    close(treasure_fd);

    char log_file[128];
    memset(log_file, 0, sizeof(log_file));
    strcat(log_file, path);
    strcat(log_file, "/logged_hunt.log");
    log_file[strlen(log_file)] = '\0';
    int log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd == -1) 
    {
        perror("(add_hunt - open logged_hunt.log)");
        exit(EXIT_FAILURE);
    }

    char log_msg[256];
    memset(log_msg, 0, sizeof(log_msg));
    strcat(log_msg, "Created hunt: ");
    strcat(log_msg, hunt_id);
    strcat(log_msg, "\n");
    log_msg[strlen(log_msg)] = '\0';
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) 
    {
        perror("(add_hunt - write)");
        exit(EXIT_FAILURE);
    }
    close(log_fd);

    char symlink_file[128];
    memset(symlink_file, 0, sizeof(symlink_file));
    strcat(symlink_file, "logged_hunt-");
    strcat(symlink_file, hunt_id);
    symlink_file[strlen(symlink_file)] = '\0';
    if (symlink(log_file, symlink_file) == -1) 
    {
        perror("(add_hunt - symlink)");
        exit(EXIT_FAILURE);
    }
}

void add_treasure(char *hunt_id, char *id, char *username, char *latitude, char *longitude, char *clue, char *value) 
{
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';

    char treasure_file[128];
    memset(treasure_file, 0, sizeof(treasure_file));
    strcat(treasure_file, path);
    strcat(treasure_file, "/treasure.dat");
    treasure_file[strlen(treasure_file)] = '\0';
    int treasure_fd = open(treasure_file, O_WRONLY | O_APPEND, 0666);
    if (treasure_fd == -1) 
    {
        perror("(add_treasure - open treasure.dat)");
        exit(EXIT_FAILURE);
    }

    Treasure treasure;
    treasure.id = atoi(id);
    strcpy(treasure.username, username);
    treasure.latitude = atof(latitude);
    treasure.longitude = atof(longitude);
    strcpy(treasure.clue, clue);
    treasure.value = atoi(value);

    if (write(treasure_fd, &treasure, sizeof(treasure)) == -1) 
    {
        perror("(add_treasure - write)");
        exit(EXIT_FAILURE);
    }
    close(treasure_fd);

    char log_file[128];
    memset(log_file, 0, sizeof(log_file));
    strcat(log_file, path);
    strcat(log_file, "/logged_hunt.log");
    log_file[strlen(log_file)] = '\0';
    int log_fd = open(log_file, O_WRONLY | O_APPEND, 0666);
    if (log_fd == -1) 
    {
        perror("(add_treasure - open logged_hunt.log)");
        exit(EXIT_FAILURE);
    }

    char log_msg[256];
    memset(log_msg, 0, sizeof(log_msg));
    strcat(log_msg, "Added treasure to hunt ");
    strcat(log_msg, hunt_id);
    strcat(log_msg, " - id=");
    strcat(log_msg, id);
    strcat(log_msg, ", username='");
    strcat(log_msg, username);
    strcat(log_msg, "', latitude=");
    strcat(log_msg, latitude);
    strcat(log_msg, ", longitude=");
    strcat(log_msg, longitude);
    strcat(log_msg, ", clue='");
    strcat(log_msg, clue);
    strcat(log_msg, "', value=");
    strcat(log_msg, value);
    strcat(log_msg, "\n");
    log_msg[strlen(log_msg)] = '\0';
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) 
    {
        perror("(add_treasure - write)");
        exit(EXIT_FAILURE);
    }
    close(log_fd);
}

void list(char *hunt_id)
{
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';

    char treasure_file[128];
    memset(treasure_file, 0, sizeof(treasure_file));
    strcat(treasure_file, path);
    strcat(treasure_file, "/treasure.dat");
    treasure_file[strlen(treasure_file)] = '\0';
    int treasure_fd = open(treasure_file, O_RDONLY | O_APPEND, 0666);
    if (treasure_fd == -1) 
    {
        perror("(list - open treasure.dat)");
        exit(EXIT_FAILURE);
    }

    printf("hunt name: %s\n", hunt_id);
    struct stat s;
    if(fstat(treasure_fd, &s) == -1)
    {
        perror("(list - fstat treasure_fd)");
        exit(EXIT_FAILURE);
    }
    printf("file size: %lld\n", s.st_size);
    time_t timestamp = s.st_mtimespec.tv_sec;
    struct tm *tm_info = localtime(&timestamp);
    char time_buffer[80];
    memset(time_buffer, 0, sizeof(time_buffer));
    strftime(time_buffer, 80, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("last modified: %s\n", time_buffer);
    
    int treasure_count = s.st_size / sizeof(Treasure);
    //printf("SIZE: %ld\n", sizeof(Treasure));
    Treasure *treasure = (Treasure*)malloc(treasure_count * sizeof(Treasure));
    if(treasure == NULL && treasure_count > 0)
    {
        perror("(list - malloc)");
        close(treasure_fd);
        exit(EXIT_FAILURE);
    }
    if(read(treasure_fd, treasure, s.st_size) == -1)
    {
        perror("(list - read)");
        close(treasure_fd);
        free(treasure);
        exit(EXIT_FAILURE);
    }
    printf("\n");
    for(int i = 0; i < treasure_count; i++)
    {
        printf("Treasure ID: %d\n", treasure[i].id);
        printf("Username: %s\n", treasure[i].username);
        printf("Latitude: %f\n", treasure[i].latitude);
        printf("Longitude: %f\n", treasure[i].longitude);
        printf("Clue: %s\n", treasure[i].clue);
        printf("Value: %d\n", treasure[i].value);
        printf("\n");
    }
    free(treasure);

    char log_file[128];
    memset(log_file, 0, sizeof(log_file));
    strcat(log_file, path);
    strcat(log_file, "/logged_hunt.log");
    log_file[strlen(log_file)] = '\0';
    int log_fd = open(log_file, O_WRONLY | O_APPEND, 0666);
    if (log_fd == -1) 
    {
        perror("(list - open logged_hunt.log)");
        exit(EXIT_FAILURE);
    }
    char log_msg[256];
    memset(log_msg, 0, sizeof(log_msg));
    strcat(log_msg, "Listed treasures from hunt: ");
    strcat(log_msg, hunt_id);
    strcat(log_msg, "\n");
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) 
    {
        perror("(list - write)");
        exit(EXIT_FAILURE);
    }
    close(log_fd);
    
    close(treasure_fd);
}

void view(char *hunt_id, char *id)
{
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';

    char treasure_file[128];
    memset(treasure_file, 0, sizeof(treasure_file));
    strcat(treasure_file, path);
    strcat(treasure_file, "/treasure.dat");
    treasure_file[strlen(treasure_file)] = '\0';
    int treasure_fd = open(treasure_file, O_RDONLY | O_APPEND, 0666);
    if (treasure_fd == -1) 
    {
        perror("(view - open treasure.dat)");
        exit(EXIT_FAILURE);
    }

    struct stat s;
    if(fstat(treasure_fd, &s) == -1)
    {
        perror("(view - fstat treasure_fd)");
        exit(EXIT_FAILURE);
    }
    int treasure_count = s.st_size / sizeof(Treasure);
    //printf("SIZE: %ld\n", sizeof(Treasure));
    Treasure *treasure = (Treasure*)malloc(treasure_count * sizeof(Treasure));
    if(treasure == NULL && treasure_count > 0)
    {
        perror("(view - malloc)");
        close(treasure_fd);
        exit(EXIT_FAILURE);
    }
    if(read(treasure_fd, treasure, s.st_size) == -1)
    {
        perror("(view - read)");
        close(treasure_fd);
        free(treasure);
        exit(EXIT_FAILURE);
    }
    printf("\n");
    for(int i = 0; i < treasure_count; i++)
    {
        if(treasure[i].id == atoi(id)) 
        {
            printf("Treasure ID: %d\n", treasure[i].id);
            printf("Username: %s\n", treasure[i].username);
            printf("Latitude: %f\n", treasure[i].latitude);
            printf("Longitude: %f\n", treasure[i].longitude);
            printf("Clue: %s\n", treasure[i].clue);
            printf("Value: %d\n", treasure[i].value);
            printf("\n");
        }
    }
    free(treasure);

    char log_file[128];
    memset(log_file, 0, sizeof(log_file));
    strcat(log_file, path);
    strcat(log_file, "/logged_hunt.log");
    log_file[strlen(log_file)] = '\0';
    int log_fd = open(log_file, O_WRONLY | O_APPEND, 0666);
    if (log_fd == -1) 
    {
        perror("(view - open logged_hunt.log)");
        exit(EXIT_FAILURE);
    }
    char log_msg[256];
    memset(log_msg, 0, sizeof(log_msg));
    strcat(log_msg, "Viewed treasure (id=");
    strcat(log_msg, id);
    strcat(log_msg, ") from hunt: ");
    strcat(log_msg, hunt_id);
    strcat(log_msg, "\n");
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) 
    {
        perror("(view - write)");
        exit(EXIT_FAILURE);
    }
    close(log_fd);

    close(treasure_fd);
}

void remove_treasure(char *hunt_id, char *id) {
    
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';

    char treasure_file[128];
    memset(treasure_file, 0, sizeof(treasure_file));
    strcat(treasure_file, path);
    strcat(treasure_file, "/treasure.dat");
    treasure_file[strlen(treasure_file)] = '\0';
    int treasure_fd = open(treasure_file, O_RDWR, 0666);
    if (treasure_fd == -1) 
    {
        perror("(remove_treasure - open treasure.dat)");
        exit(EXIT_FAILURE);
    }

    struct stat s;
    if (fstat(treasure_fd, &s) == -1) 
    {
        perror("(remove_treasure - fstat treasure_fd)");
        close(treasure_fd);
        exit(EXIT_FAILURE);
    }
    int treasure_count = s.st_size / sizeof(Treasure);
    Treasure *treasure = (Treasure *)malloc(treasure_count * sizeof(Treasure));
    if (treasure == NULL && treasure_count > 0) 
    {
        perror("(remove_treasure - malloc)");
        close(treasure_fd);
        exit(EXIT_FAILURE);
    }

    if (read(treasure_fd, treasure, s.st_size) == -1) 
    {
        perror("(remove_treasure - read)");
        close(treasure_fd);
        free(treasure);
        exit(EXIT_FAILURE);
    }

    int new_treasure_count = 0;
    for (int i = 0; i < treasure_count; i++) 
    {
        if (treasure[i].id != atoi(id)) {
            new_treasure_count++;
        }
    }

    Treasure *new_treasure = (Treasure *)malloc(new_treasure_count * sizeof(Treasure));
    if (new_treasure == NULL && new_treasure_count > 0) 
    {
        perror("(remove_treasure - malloc)");
        close(treasure_fd);
        free(treasure);
        exit(EXIT_FAILURE);
    }
    int j = 0;
    for (int i = 0; i < treasure_count; i++) 
    {
        if (treasure[i].id != atoi(id)) 
        {
            new_treasure[j] = treasure[i];
            j++;
        }
    }
    free(treasure);

    //go to beggining of file:
    lseek(treasure_fd, 0, SEEK_SET);

    for (int i = 0; i < new_treasure_count; i++) 
    {
        if (write(treasure_fd, &new_treasure[i], sizeof(Treasure)) == -1) 
        {
            perror("(remove_treasure - write)");
            close(treasure_fd);
            free(new_treasure);
            exit(EXIT_FAILURE);
        }
    }

    if (ftruncate(treasure_fd, new_treasure_count * sizeof(Treasure)) == -1) 
    {
        perror("(remove_treasure - ftruncate)");
        close(treasure_fd);
        free(new_treasure);
        exit(EXIT_FAILURE);
    }

    close(treasure_fd);
    free(new_treasure);

    char log_file[128];
    memset(log_file, 0, sizeof(log_file));
    strcat(log_file, path);
    strcat(log_file, "/logged_hunt.log");
    log_file[strlen(log_file)] = '\0';
    int log_fd = open(log_file, O_WRONLY | O_APPEND, 0666);
    if (log_fd == -1) {
        perror("(remove_treasure - open logged_hunt.log)");
        exit(EXIT_FAILURE);
    }

    char log_msg[256];
    memset(log_msg, 0, sizeof(log_msg));
    strcat(log_msg, "Removed treasure (id=");
    strcat(log_msg, id);
    strcat(log_msg, ") from hunt: ");
    strcat(log_msg, hunt_id);
    strcat(log_msg, "\n");
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) {
        perror("(remove_treasure - write)");
        close(log_fd);
        exit(EXIT_FAILURE);
    }
    close(log_fd);
}

void remove_hunt(char *hunt_id)
{
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, "./");
    strcat(path, hunt_id);
    path[strlen(path)] = '\0';

    DIR *d = opendir(path);
    if(d == NULL)
    {
        perror("remove_hunt - opendir");
        return;
    }
    //deleting files from directory:
    struct dirent *dir;
    //printf("PATH: %s\n", path);
    char unlink_path[128];
    while((dir = readdir(d)) != NULL)
    {
        if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") !=0)
        {
            //printf("%s\n", dir->d_name);
            memset(unlink_path, 0, sizeof(unlink_path));
            strcat(unlink_path, path);
            strcat(unlink_path, "/");
            strcat(unlink_path, dir->d_name);
            unlink_path[strlen(unlink_path)] = '\0';
            //printf("%s\n", unlink_path);
            unlink(unlink_path);
        }
    }
    closedir(d);
    if(rmdir(path) == -1)
    {
        perror("(remove_hunt - rmdir)");
        exit(EXIT_FAILURE);
    }
    //deleting symbolic link:
    memset(unlink_path, 0, sizeof(unlink_path));
    strcat(unlink_path, "logged_hunt-");
    strcat(unlink_path, hunt_id);
    unlink_path[strlen(unlink_path)] = '\0';
    unlink(unlink_path);
}

int main(int argc, char **argv) 
{
    if (argc >= 2 && strcmp("--add", argv[1]) == 0) 
    {
        if (argc != 9) 
        {
            invalid_input_err();
        }
        char path[128] = "./";
        strcat(path, argv[2]);
        path[strlen(path)] = '\0';
        DIR *dir = opendir(path);
        if (!dir) 
        {
            add_hunt(argv[2]);
        }
        if(dir) 
            closedir(dir);
        add_treasure(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
    } 
    else if (argc >= 2 && strcmp("--list", argv[1]) == 0) 
    {
        if (argc != 3) 
        {
            invalid_input_err();
        }
        list(argv[2]);
    } 
    else if (argc >= 2 && strcmp("--view", argv[1]) == 0) 
    {
        if (argc != 4) 
        {
            invalid_input_err();
        }
        view(argv[2], argv[3]);
    } 
    else if (argc >= 2 && strcmp("--remove_treasure", argv[1]) == 0) 
    {
        if (argc != 4) 
        {
            invalid_input_err();
        }
        remove_treasure(argv[2], argv[3]);
    } 
    else if (argc >= 2 && strcmp("--remove_hunt", argv[1]) == 0) 
    {
        if (argc != 3) 
        {
            invalid_input_err();
        }
        remove_hunt(argv[2]);
    } 
    else 
    {
        invalid_input_err();
    }
    return 0;
}