#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_USERNAME_LEN 32
#define MAX_USERS_PER_HUNT 64
#define MAX_CLUE_LEN 256
#define MAX_HUNT_ID_LEN 128
#define MAX_TREASURE_ID_LEN 32

pid_t pid = 0;

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;

typedef struct {
    char username[MAX_USERNAME_LEN];
    int score;
} User;

void get_score(char *hunt_id)
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
        perror("(get_score - open treasure.dat)");
        exit(EXIT_FAILURE);
    }

    struct stat s;
    if(fstat(treasure_fd, &s) == -1)
    {
        perror("(get_score - fstat treasure_fd)");
        exit(EXIT_FAILURE);
    }
    int treasure_count = s.st_size / sizeof(Treasure);
    Treasure *treasure = (Treasure*)malloc(treasure_count * sizeof(Treasure));
    if(treasure == NULL && treasure_count > 0)
    {
        perror("(get_score - malloc)");
        close(treasure_fd);
        exit(EXIT_FAILURE);
    }
    if(read(treasure_fd, treasure, s.st_size) == -1)
    {
        perror("(get_score - read)");
        close(treasure_fd);
        free(treasure);
        exit(EXIT_FAILURE);
    }

    int user_count = 0;
    User user[MAX_USERS_PER_HUNT];
    for(int i = 0; i < treasure_count; i++)
    {
        int not_found = 1;
        for(int j = 0; j < user_count; j++)
        {
            if(strcmp(treasure[i].username, user[j].username) == 0)
            {
                not_found = 0;
                user[j].score += treasure[i].value;
                break;
            }
        }
        if(not_found)
        {
            //printf("%s\n", treasure[i].username);
            strcpy(user[user_count].username, treasure[i].username);
            user[user_count].score = 0;
            user[user_count].score += treasure[i].value;
            user_count++;
        }
    }
    free(treasure);

    for(int i = 0; i < user_count; i++)
    {
        printf("\n");
        printf("user: %s\n", user[i].username);
        printf("total score: %d\n", user[i].score);
        printf("hunt id: %s", hunt_id);
        printf("\n");
    }
    //printf("%d\n", user_count);
}

int main(void)
{
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        perror("(main) opendir:");
        exit(1);
    }
    struct dirent *d;
    d = readdir(dir);
    while(d != NULL)
    {
        //in the current directory, only hunts are directories
        if(d->d_type == DT_DIR && strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0 && strcmp(d->d_name, ".git") != 0)
        {
            
            //printf("%s\n", d->d_name);
            //creating a process for each existing hunt that calculates and outputs the scores of users in that hunt
            pid = fork();
            if(pid == -1)
            {
                perror("(main) fork:");
            }
            else if(pid == 0)
            {
                //printf("Score for treasure: %s\n", d->d_name);
                get_score(d->d_name);
                exit(0);
            }
            wait(NULL);
        }
        d = readdir(dir);
    }
    closedir(dir);

    return 0;
}