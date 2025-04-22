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

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;

pid_t monitor_pid = 0;

char cmd_hunt_id[MAX_HUNT_ID_LEN];

void exec_list(char *hunt_id)
{
    pid_t list_pid = 0;
    if((list_pid = fork()) == -1)
    {
        perror("(list_hunts) fork:");
        exit(1);
    }
    if(list_pid == 0)
    {
        execlp("./treasure_manager", "./treasure_manager", "--list", hunt_id, NULL);
        //the program shouldn't reach this part only if exec failed
        perror("(exec_list) execlp:");
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
        perror("(list_hunts) opendir:");
        exit(1);
    }
    struct dirent *d;
    d = readdir(dir);
    while(d != NULL)
    {
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
    exec_list(cmd_hunt_id);
}

void start_monitor()
{
    if(monitor_pid != 0)
    {
        printf("Monitor already running\n");
        return;
    }
    if((monitor_pid = fork()) < 0) //creating monitor process
    {
        perror("(start_monitor) Error creating monitor process:");
        exit(1);
    }
    if(monitor_pid == 0)
    {
        printf("Monitor process started\n");

        struct sigaction sa_list_hunts;
        sa_list_hunts.sa_handler = list_hunts; //sa_handler - pointer to the function that will be called when the process receives the specific signal
        sigemptyset(&sa_list_hunts.sa_mask); //ensure no other signals are blocked while list_hunts is running
        sa_list_hunts.sa_flags = 0; //using default behavior of sa_flags
        if(sigaction(SIGUSR1, &sa_list_hunts, NULL) == -1) //action to be taken upon the incoming specific signal
        {
            perror("(start_monitor) sigaction SIGUSR1:");
            exit(1);
        }

        struct sigaction sa_list_treasures;
        sa_list_treasures.sa_handler = list_treasures;
        sigemptyset(&sa_list_hunts.sa_mask);
        sa_list_hunts.sa_flags = 0;
        if(sigaction(SIGUSR2, &sa_list_treasures, NULL) == -1)
        {
            perror("(start_monitor) sigaction SIGUSR2:");
            exit(1);
        }

        while(1); //infinite loop - monitor process only responds to signals
        exit(0);
    }
}

int main(void)
{
    int cmd = -1;
    int status;

    while(1)
    {
        printf("---------------------------------------------------\n");
        printf("Treasure Hub Menu:\n");
        printf("1 - Start monitor\n");
        printf("2 - List hunts\n");
        printf("3 - List treasures\n");
        printf("4 - View treasure\n");
        printf("5 - Stop monitor\n");
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
                    perror("(main) kill SIGUSR1:");
                //list_hunts();
            }
        }
        else if(cmd == 3)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                printf("Sending command signal to monitor...\n");
                printf("Please enter a hunt ID:\n");
                fgets(cmd_hunt_id, MAX_HUNT_ID_LEN, stdin);
                getchar();
                cmd_hunt_id[strcspn(cmd_hunt_id, "\n")] = '\0';
                if(kill(monitor_pid, SIGUSR2) == -1)
                    perror("(main) kill SIGUSR2:");
            }
        }
        else if(cmd == 4)
        {
            if(monitor_pid == 0)
                printf("Monitor not running. Please start it first!\n");
            else
            {
                printf("Sending command signal to monitor..\n");
            }
        }
        else if(cmd == 5)
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
                        printf("Montior process exited with status %d\n", WEXITSTATUS(status));
                    else if(WIFSIGNALED(status))
                        printf("Monitor process terminated by signal %d\n", WTERMSIG(status));
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