#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

//Initialize constants for indexing the file descriptors properly
#define READ 0 
#define WRITE 1

int main(int argc, char* argv[]) {

    char *filename = "jobExecutorServer.txt";

    //Variable used to store (and initialize if needed) the proccess id of the jobExecutorServer
    int pid;

    int file_desc;

    // If the jobExecutorServer is not running (open returns < 0)then the proccess is forked and the child calls execvp to start executing the jobExecutorServer
    if ((file_desc=open("jobExecutorServer.txt", O_RDONLY)) < 0) {

        pid = fork() ;

        // If pid is -1 it means fork failed so handle it approprietly
        if(pid == -1) {
            perror("Failed to execute jobExecutorServer : fork() failed");

            exit(1);
        }

        //If it is the child proccess being excecuted use exec to to run the jobExecutorServer
        if( pid == 0 ) {
            
            execvp("./jobExecutorServer",NULL);

            // If the function returns an error has oocured 

            perror("Failed to exec jobExecutorServer : execvp error");

            exit(1);
        }
        //Sleep for 0.1 seconds to let the jobExecutor start and initialize it's signal handling so the signal from the commander can be properly received
        usleep(100000);

    }
    // If the jobExecutorServer is already running then read its proccess id through the jobExecutorServer text file

    else {

        //Read the proccess id of the jobExecutorServer from the text file if it opened succesfully
        char buffer[20];

        if ( ( read(file_desc, buffer, sizeof(buffer) ) ) < 0) {
            
            perror("read");
            exit(1);
        }

        pid = atoi(buffer);

    } 

    // If less than 2 arguments are given as input let the user know the required usage
    if (argc < 2) {

        printf("Usage requires 2 or more arguments\n");

        exit(1);
    }

    // Case where the input is a valid command
    if ((strcmp( argv[1] , "issueJob") == 0 ) || (strcmp(argv[1], "setConcurrency") == 0) || (strcmp(argv[1], "stop") == 0) || (strcmp(argv[1], "poll") == 0) || (strcmp(argv[1], "exit") == 0))   {
        
        //Find the pid of the commander and concatanate it at the end of the name of each of the two pipes so every commander communicates with the server 
        //through a unique pair of fifo pipes (Important for when multiple commanders are running simultaneously).

        int commander_pid = getpid();

        char commander_write_fifo[40] = "commander_write_fifo";
        char executor_server_write_fifo[40] = "executor_write_fifo";

        snprintf(commander_write_fifo + 20, 20, "%d", commander_pid );
        snprintf(executor_server_write_fifo + 19, 20, "%d", commander_pid  );

        int fd[2];

        //Create 2 FIFOs one for the commander to write and the ExecutorServer to read and one for the Executor-Server to write and the commander
        //to read

        if ( mkfifo (commander_write_fifo , 0666) == -1 ) {

            if ( errno != EEXIST ) {
                perror ( " transmiter : mkfifo " ) ;
                exit(1) ;
            }
        }
        if ( mkfifo (executor_server_write_fifo , 0666) == -1 ) {

            if ( errno != EEXIST ) {
                perror ( " receiver : mkfifo " ) ;
                exit(1) ;
            }
        }

        //Use the kill function to send a SIGUSR1 signal to the jobExecutorServer proccess
        kill(pid, SIGUSR1);

        // Open the fifo (pipe) for the commander to write.
        //The Commander  will wait until the pipe is open for read by the Executor-Server.

        if ((fd[WRITE] = open(commander_write_fifo,  O_WRONLY)) < 0)  {
            perror("commander : fifo open error");
            exit(1);
        }

        //Initialize a character variable to store a number (as a character) which will be representing each of the 5 commands 
        //that can be given as input to the job commander 
        // command_id = '1' -> issueJob
        // command_id = '2' -> setConcurrency
        // command_id = '3' -> stop
        // command_id = '4' -> poll
        // command_id = '5' -> exit 

        char command_id;

        if(strcmp( argv[1] , "issueJob") == 0 ) 
        {
            command_id = '1';
        }
        else if (strcmp(argv[1], "setConcurrency") == 0)
        {
            command_id = '2';
        }
        else if (strcmp(argv[1], "stop") == 0)
        {
            command_id = '3';
        }
        else if (strcmp(argv[1], "poll") == 0)
        {
            command_id = '4';
        }
        else if (strcmp(argv[1],"exit") == 0) 
        {
            command_id ='5';
        }

        // Write the command id in the pipe in order for the jobExecutorServer to recognize the commands easier
        if ( write( fd[WRITE] , &command_id , 1) != 1) {
            perror ("commander write" ) ;
            exit(1) ;
        }

        for(int i = 2; i < argc ; i++) { 

            // Write every argument of the arg vector (except the first 2) into the pipe for the 
            // job Executor Server to read. The arguments are seperated by a white space 
            if ( write( fd[WRITE] , argv[i] , strlen(argv[i])) < 0) {
                perror ( "transmiter : write failed" ) ;
                exit(1) ;
            }

            if (i < argc - 1) {

                if (write(fd[WRITE], " " , 1) < 0) {
                    perror ( "transmiter : write failed" ) ;
                    exit(1) ;
                }
            }
            else {
                //Write the null character at the end so the executor server stops reading (if there is garbage) afterwards.
                if (write(fd[WRITE], "\0" , 1) < 0) {
                    perror ( "commander : write" ) ;
                    exit(1) ;
                }
            }
        }
        //Close and delete the commander-writing pipe
        close(fd[WRITE]);
        unlink(commander_write_fifo);

        //if command is not setConcurrency read the response from the executor server
        if (command_id != '2') {

            if ((fd[READ] = open(executor_server_write_fifo,  O_RDONLY)) < 0)  {
                perror("receiver : fifo open error");
                exit(1);
            }
            
            //Initialize a buffer with 64 bytes and increase its size each time it fills up
            //Initialize an offset  to be able to read dynamically
            int buff_size = 64;
            char* buff = (char *)malloc(buff_size);
            int bytes_read;
            int offset = 0;

            // While there are characters in the pipe read them and increase the size of the buffer whenever it fills up
            while((bytes_read = read(fd[READ], buff + offset , buff_size - offset)) > 0)
            {
                offset += bytes_read;

                if(buff_size <= offset) {
                    buff_size += bytes_read;
                    buff = (char*)realloc(buff, buff_size); 
                }
            }
            //Close and delete the executor-server writing pipe
            close(fd[READ]);
            unlink(executor_server_write_fifo);
            
            //Print the response of the the executor server 
            printf("%s\n", buff);

            //Free the dynamically allocated memory
            free(buff);
        }
        //Delete the pipes in the case they weren't deleted previously
        unlink(executor_server_write_fifo);
        unlink(commander_write_fifo);
    }   

    // In the case where the input is not a valid command print an error
    else {

        printf("Invalid Command - Syntax Error \n"); 
        exit(1);

    }
}