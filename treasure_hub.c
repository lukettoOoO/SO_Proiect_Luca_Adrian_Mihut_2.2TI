#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_USERNAME_LEN 32
#define MAX_CLUE_LEN 256
#define MAX_HUNT_ID_LEN 128
#define MAX_TREASURE_ID_LEN 32
#define BUFFER_SIZE 1024

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;

pid_t monitor_pid = 0;
int pipe_fd[2];

void exec_calculate_score()
{
    pid_t calculate_score_pid = 0;
    if((calculate_score_pid = fork()) == -1)
    {
        perror("(exec_calculate_score) fork");
        exit(1);
    }
    if(calculate_score_pid == 0)
    {
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        execlp("./calculate_score", "./calculate_score", NULL);
        perror("(exec_calculate_score) execlp");
        exit(1);
    }
    int status;
    waitpid(calculate_score_pid, &status, 0);
}

void exec_list(char *hunt_id) //lists all treasures from a hunt
{
    pid_t list_pid = 0;
    if((list_pid = fork()) == -1)
    {
        perror("(list_hunts) fork");
        exit(1);
    }
    if(list_pid == 0)
    {
        //redirect stdout to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO); //send stdout to the pipe
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        execlp("./treasure_manager", "./treasure_manager", "--list", hunt_id, NULL);
        //the program shouldn't reach this part only if exec failed
        perror("(exec_list) execlp");
        exit(1);
    }
    int status;
    waitpid(list_pid, &status, 0);
}

//signal handler:
void list_hunts(int sig)
{
    //going through every file of the current directory and listing the hunts using --list from the previous phase by creating a new process to execute it
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        perror("(list_hunts) opendir");
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
            exec_list(d->d_name);
        }
        d = readdir(dir);
    }

}

void list_treasures(int sig)
{
    char temp_filename[64];
    snprintf(temp_filename, sizeof(temp_filename), "list_treasures_%d", getppid());

    //printf("%s\n", temp_filename);
    int fd = open(temp_filename, O_RDONLY);
    if(fd == -1)
    {
        perror("(list_treasures) open");
        exit(1);
    }

    char hunt_id[MAX_HUNT_ID_LEN];
    if(read(fd, hunt_id, MAX_HUNT_ID_LEN - 1) > 0)
    {
        hunt_id[strlen(hunt_id)] = '\0';
        exec_list(hunt_id);
    }
    else
    {
        perror("(list_treasures) read");
        close(fd);
        unlink(temp_filename);
        exit(1);
    }
    close(fd);
    unlink(temp_filename); //deleting the temp file
}

void exec_view(char *hunt_id, char *id)
{
    pid_t view_pid = 0;
    if((view_pid = fork()) == -1)
    {
        perror("(exec_view) fork");
        exit(1);
    }
    if(view_pid == 0)
    {
        //redirect stdout to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        execlp("./treasure_manager", "./treasure_manager", "--view", hunt_id, id, NULL);
        //the program shouldn't reach this part only if exec failed
        perror("(exec_view) execlp");
        exit(1);
    }
    int status;
    waitpid(view_pid, &status, 0);
}

void view_treasure(int sig)
{
    char hunt_id[MAX_HUNT_ID_LEN];
    char id[MAX_TREASURE_ID_LEN];

    char temp_filename[64];
    snprintf(temp_filename, sizeof(temp_filename), "view_treasure_hunt_id_%d", getppid());
    int fd = open(temp_filename, O_RDONLY);
    if(fd == -1)
    {
        perror("(view_treasure) open");
        exit(1);
    }
    if(read(fd, hunt_id, MAX_HUNT_ID_LEN - 1) > 0)
    {
        hunt_id[strlen(hunt_id)] = '\0';
    }
    else
    {
        perror("(list_treasures) read");
        close(fd);
        unlink(temp_filename);
        exit(1);
    }
    close(fd);
    unlink(temp_filename); //deleting the temp file

    char temp_filename1[64];
    snprintf(temp_filename1, sizeof(temp_filename1), "view_treasure_id_%d", getppid());
    int fd1 = open(temp_filename1, O_RDONLY);
    if(fd1 == -1)
    {
        perror("(view_treasure) open");
        exit(1);
    }
    if(read(fd1, id, MAX_HUNT_ID_LEN - 1) > 0)
    {
        id[strlen(id)] = '\0';
    }
    else
    {
        perror("(list_treasures) read");
        close(fd1);
        unlink(temp_filename1);
        exit(1);
    }

    close(fd1);
    unlink(temp_filename1); //deleting the temp file

    exec_view(hunt_id, id);
}

void start_monitor()
{
    if(monitor_pid != 0)
    {
        printf("Monitor already running\n");
        return;
    }
    //creating pipe to send output from child processes back to the parent process
    if(pipe(pipe_fd) < 0)
    {
        perror("(start_monitor) Error creating pipe:");
        exit(1);
    }
    if((monitor_pid = fork()) < 0) //creating monitor process
    {
        perror("(start_monitor) Error creating monitor process:");
        exit(1);
    }
    if(monitor_pid == 0)
    {
        printf("Monitor process started\n");

        //close both ends of the pipe in the monitor since in this function, the child process only reacts to signals, so we don't actually need the pipe here
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        struct sigaction sa_list_hunts;
        sa_list_hunts.sa_handler = list_hunts; //sa_handler - pointer to the function that will be called when the process receives the specific signal
        sigemptyset(&sa_list_hunts.sa_mask); //ensure no other signals are blocked while list_hunts is running
        sa_list_hunts.sa_flags = 0; //using default behavior of sa_flags
        if(sigaction(SIGUSR1, &sa_list_hunts, NULL) == -1) //action to be taken upon the incoming specific signal
        {
            perror("(start_monitor) sigaction SIGUSR1");
            exit(1);
        }

        struct sigaction sa_list_treasures;
        sa_list_treasures.sa_handler = list_treasures;
        sigemptyset(&sa_list_treasures.sa_mask);
        sa_list_treasures.sa_flags = 0;
        if(sigaction(SIGUSR2, &sa_list_treasures, NULL) == -1)
        {
            perror("(start_monitor) sigaction SIGUSR2");
            exit(1);
        }

        struct sigaction sa_view_treasure;
        sa_view_treasure.sa_handler = view_treasure;
        sigemptyset(&sa_view_treasure.sa_mask);
        sa_view_treasure.sa_flags = 0;
        if(sigaction(SIGPIPE, &sa_view_treasure, NULL) == -1)
        {
            perror("(start_monitor) sigaction SIGPIPE");
            exit(1);
        }

        while(1); //infinite loop - monitor process only responds to signals
        exit(0);
    }
    //close the write end in the parent
    close(pipe_fd[1]);
}

int main(void)
{
    int cmd = -1;
    int status;
    char buffer[BUFFER_SIZE];

    while(1)
    {
        printf("---------------------------------------------------\n");
        printf("Treasure Hub Menu:\n");
        printf("1 - Start monitor\n");
        printf("2 - List hunts\n");
        printf("3 - List treasures\n");
        printf("4 - View treasure\n");
        printf("5 - Calculate score\n");
        printf("6 - Stop monitor\n");
        printf("0 - Exit\n");
        printf("\n");

        scanf("%d", &cmd);
        if(cmd == 1)
        {
            start_monitor();
        }
        else if(cmd == 2)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                printf("Sending command signal to monitor...\n\n");
                if(kill(monitor_pid, SIGUSR1) == -1) //sending SIGUSR1 signal to monitor process
                    perror("(main) kill SIGUSR1");
                //list_hunts();
                
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytes_read;
                while((bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1)) > 0)
                {
                    buffer[bytes_read] = '\0';
                    printf("\n%s\n", buffer);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                if(bytes_read == -1)
                {
                    perror("(main) read from pipe");
                }       
            }
        }
        else if(cmd == 3)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                char cmd_hunt_id[MAX_HUNT_ID_LEN];
                printf("Sending command signal to monitor...\n");
                printf("Please enter a hunt ID:\n");
                getchar();
                fgets(cmd_hunt_id, MAX_HUNT_ID_LEN, stdin);
                cmd_hunt_id[strcspn(cmd_hunt_id, "\n")] = '\0';

                char temp_filename[64];
                strcat(temp_filename, "/tmp/list_treasures_");
                snprintf(temp_filename, sizeof(temp_filename), "list_treasures_%d", getpid()); //using the current process pid for a unique file name
                int fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if(fd == -1)
                {
                    perror("(main) open temp");
                    exit(1);
                }
                //printf("STRING: %s\n", cmd_hunt_id);
                write(fd, cmd_hunt_id, strlen(cmd_hunt_id));
                close(fd);


                if(kill(monitor_pid, SIGUSR2) == -1)
                    perror("(main) kill SIGUSR2");
                
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytes_read;
                while((bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1)) > 0)
                {
                    buffer[bytes_read] = '\0';
                    printf("\n%s\n", buffer);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                if(bytes_read == -1)
                {
                    perror("(main) read from pipe");
                }
            }
        }
        else if(cmd == 4)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                printf("Sending command signal to monitor..\n");

                char hunt_id[MAX_HUNT_ID_LEN];
                char id[MAX_TREASURE_ID_LEN];
                printf("Please enter a hunt ID:\n");
                getchar();
                fgets(hunt_id, MAX_HUNT_ID_LEN, stdin);
                hunt_id[strcspn(hunt_id, "\n")] = '\0';
                printf("Please enter treasure ID\n");
                fgets(id, MAX_TREASURE_ID_LEN, stdin);
                id[strcspn(id, "\n")] = '\0';

                char temp_filename[64];
                snprintf(temp_filename, sizeof(temp_filename), "view_treasure_hunt_id_%d", getpid());
                int fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (fd == -1)
                {
                    perror("(main) open");
                    exit(1);
                }
                write(fd, hunt_id, strlen(hunt_id));
                close(fd);

                char temp_filename1[64];
                snprintf(temp_filename1, sizeof(temp_filename1), "view_treasure_id_%d", getpid());
                int fd1 = open(temp_filename1, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (fd1 == -1)
                {
                    perror("(main) open");
                    exit(1);
                }
                write(fd1, id, strlen(id));
                close(fd1);

                if (kill(monitor_pid, SIGPIPE) == -1)
                    perror("(main) kill SIGPIPE");

                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytes_read;
                while((bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1)) > 0)
                {
                    buffer[bytes_read] = '\0';
                    printf("\n%s\n", buffer);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                if(bytes_read == -1)
                {
                    perror("(main) read from pipe");
                }
            }
        }
        else if(cmd == 5)
        {
            printf("Calculating score...\n\n");
            exec_calculate_score();
        }
        else if(cmd == 6)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                printf("Sending command signal to monitor...\n");
                kill(monitor_pid, SIGTERM); //signal for monitor process termination
                if(waitpid(monitor_pid, &status, 0) == -1) //waiting for monitor process to exit
                    perror("(main) waitpid:");
                else
                {
                    if(WIFEXITED(status))
                    {
                        printf("Monitor process exited with status %d\n", WEXITSTATUS(status));
                        close(pipe_fd[0]);
                    }
                    else if(WIFSIGNALED(status))
                    {
                        printf("Monitor process terminated by signal %d\n", WTERMSIG(status));
                        close(pipe_fd[0]);
                    }
                }
                monitor_pid = 0;
            }
        }
        else if(cmd == 0)
        {
            if(monitor_pid != 0)
                printf("Monitor process is stil running. Please terminate it first!\n");
            else
                exit(0);
        }
        else
        {
            printf("Invalid command!\n");
        }
    }

    return 0;
}