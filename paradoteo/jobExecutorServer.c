#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "list.h"
#include "queue.h" 
#include <sys/wait.h>
#include <linux/stat.h>
#include <sys/stat.h>

#define READ 0 
#define WRITE 1

void commander_signal_handler(int sig_num,siginfo_t* info);
void child_signal_handler(int sig_num);

// Initialize a global variable for the job IDs. The ids will start from 0 and for every new job its increased by 1.
int jobID = 0;

// Initialize a global variable for the concurrency (maximum number of jobs running at the same time)
int concurrency = 3;

// Initialize a global variable to see how many proccesses are running
int running = 0;

//Initialize the running list
List* running_list;

//Initialize the waiting queue
Queue* waiting_queue;

int main(int argc, char* argv[]) {

    // If the file already exists delete it
    unlink("jobExecutorServer.txt");

    //Create the text file jobExecutorServer used to determine wether the jobExecutorServer proccess is ruuning 

    int file_desc;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644

    if ( (file_desc = open("jobExecutorServer.txt", O_CREAT | O_TRUNC | O_WRONLY , mode)) == -1) {

        perror("Failed to create jobExecutorServer.txt ");
        exit(1);
    }
    
    // Get the id of the jobExecutorServer proccess in order to write it in the jobExecutorServer proccess file
    int jobExecutorServer_pid = getpid();
    
    //Initalize a buffer used to write to the file 
    char buffer[20];

    // Convert the proccess id from an integer to a string
    snprintf(buffer,sizeof(buffer),"%d",jobExecutorServer_pid);

    //Write the converted pid to the file. If the "write" system call returns a value <=0 then it has failed 
    int chars_written;

    if ( (chars_written = write(file_desc, buffer, sizeof(buffer) ) ) <= 0) {
        perror("executor-server : write");
    }

    //Close the text file once the proccess id is written

    close(file_desc);

    running_list = list_create();
    waiting_queue = queue_create();

    // Create a sigaction struct and set the handler as the commander_signal_handler function
    static struct sigaction action1;
    action1.sa_handler = commander_signal_handler;
    // Set the flags as SA_SIGINFO so the executor server has a way to get the proccess id of the commander that is sending the signal (so it can open the pipes correctly)
    action1.sa_flags = SA_SIGINFO;
    //Fill the signal set sa_flags with all the signals so they are all blocked while the proccess handles a signal
    sigfillset(&(action1.sa_mask));
    // Call sigaction function to handle the SIGUSR1 (1st User defined-signal) which is the signal the commander sends when there is a command waiting to be executed 
    sigaction(SIGUSR1,&action1,NULL);

    // Create a sigaction struct and set the handler as the child_signal_handler function
    static struct sigaction action2;
    action2.sa_handler = child_signal_handler;
    //Fill the signal set sa_flags with all the signals so they are all blocked while the proccess handles a signal
    sigfillset(&(action2.sa_mask));
    // Call sigaction function to handle the SIGCHLD which is the signal any child proccess will send to its parent (the executor-server ) when it exits
    sigaction(SIGCHLD,&action2,NULL);

    // Do an infinite loop so the executor-server is always active and ready to receive commands from the commander
    while(1) {
        sleep(1);
    }

    // Before the jobExecutorServer proccess is terminated it deletes the text file

    unlink("jobExecutorServer.txt");

    return 0;
}

// Signal Handler Function Used for communication between the jobCommander proccess and the job ExecutorServer proccess 
// Whenever a SIGUSR1 signal is received by the jobExecutorServer it means that a jobCommander proccess is writing the executor a 
// message through the fifo pipe

void commander_signal_handler(int sig_num,siginfo_t* info) {

    // Initialize the names of the pipes as well as their file descriptor array as global variables so the signal handler can access them
    int fd[2];

    // Initialize the names of the 2 pipes using the proccess id of the jobCommander that send the command (in order to have a unique pair of pipes to communicate with each commander)
    char commander_write_fifo[40] = "commander_write_fifo";
    char executor_server_write_fifo[40] = "executor_write_fifo";

    snprintf(commander_write_fifo + 20, 20, "%d", info->si_pid  );
    snprintf(executor_server_write_fifo + 19, 20, "%d", info->si_pid  );

    // Open the "commander write fifo" to read the message sent from the job Commander  
    if ((fd[READ] = open(commander_write_fifo,  O_RDONLY)) < 0)  {
        perror("executor-server : fifo open ");
        exit(1);
    }

    char command_id;

    // Read the first byte (character) from the fifo pipe in order to recognize the command (by the command id)
    if ( read( fd[READ] , &command_id , 1) != 1) {
        perror ( "executor-server : read" );
        exit(1);
    }
    
    //If the command read is 1 it means the command is issueJob so read the job and enqueue it in the waiting queue
    if(command_id == '1') {

        //Initialize a buffer with 64 bytes and increase its size each time it fills up as 
        //Initialize an offset to be able to read into the buffer dynamically
        int buff_size = 64;
        char* buffer = (char *)malloc(buff_size);
        int bytes_read;
        int offset = 0;

        // While there are characters in the pipe read them and increase the size of the buffer whenever it fills up
        while((bytes_read = read(fd[READ], buffer + offset , buff_size - offset)) > 0)
        {   
            offset += bytes_read;

            if(buff_size <= offset) {
                buff_size += bytes_read;
                buffer = (char *)realloc(buffer, buff_size); 
            }
        }
        // Close the commander write pipe and delete it
        close(fd[READ]);
        unlink(commander_write_fifo);

        // Assign a jobID to the job given (using the jobID variable,converting it to a string and concatinating it to "job_")
        char new_job_id[10] = "job_";

        snprintf(new_job_id + 4, sizeof(new_job_id) , "%d" , jobID );

        jobID++;
        
        // If the running proccess are less than the max concurrency the proccess starts immediatly running
        if(running < concurrency)
        {
            int child_pid;
            // Use fork and execvp to run the given job
            if ((child_pid = fork()) < 0) {
                perror("ExecutorServer fork");
                exit(1);
            }
            // If its the child proccess 
            if (child_pid == 0) {
                
                //Break down the job into its arguments and call execvp
                int argc = 0;
                char* argv[20];
                char* token = strtok(buffer, " ");

                while(token != NULL) {

                    argv[argc] = token;
                    argc++;
                    token = strtok(NULL, " \n");
                }

                argv[argc] = NULL;

                execvp(argv[0], argv );
                
                //If execvp failed print an error and exit
                printf("Failed to execvp : %s\n", buffer);

                exit(1);
            }
            //if its the parent proccess :

            //Initializes a triplet with the new jobID and the job and a match with that triplet and the pid of the child (that was just forked)
            Triplet* triplet = triplet_create(new_job_id,buffer);

            Match* match = match_create(triplet,child_pid);

            // Append the new match at the end of the running proccess and increase the number of running proccesses by 1
            list_append(running_list,match);
            running++;

            //Find the list position of the newly appended match and convert it to a string

            char list_pos[5];
            snprintf(list_pos,5,"%d",running_list->last->match->triplet->position);

            // Write the new triplet on the executor-server write pipe for the commander to read

            if ((fd[WRITE] = open(executor_server_write_fifo,  O_WRONLY)) < 0)  {
                perror("executor-server : fifo open error");
                exit(1);
            }
            
            if(write(fd[WRITE],"\nJob Issued:\n",strlen("\nJob Issued:\n")) < 0) 
            {
                perror("executor-server//Find the list position of the newly appended match and convert it to a string : write failed");
            }

            if(write(fd[WRITE],new_job_id,strlen(new_job_id)) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],"\t",1) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],buffer, strlen(buffer)) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],"\t",1) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],list_pos, strlen(list_pos)) < 0) 
            {
                perror("executor-server : write failed");
            }

            // Close and delete the executor-server write pipe
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);

        }
        else 
        // if the number of running proccesses is equal to the max concurrency then the newly issued job is inserted in the waiting queue
        {   
            //Create a new triplet with the new JobID and the job
            Triplet* triplet = triplet_create(new_job_id,buffer);

            //Insert the triplet at the rear of the queue 
            queue_insert_rear(waiting_queue,triplet);
            
            //Find the queue position of the newly appended triplet and convert it to a string
            char queue_position[5] ;
            snprintf(queue_position,5,"%d",waiting_queue->rear->triplet->position);
            
            // Write the new triplet on the executor-server write pipe for the commander to read

            if ((fd[WRITE] = open(executor_server_write_fifo,  O_WRONLY )) < 0)  {
                perror("executor-server : fifo open error");
                exit(1);
            }

            if(write(fd[WRITE],"Job Issued:\n",strlen("Job Issued:\n")) < 0) 
            {
                perror("executor-server : write failed");
            }

            if(write(fd[WRITE],new_job_id,strlen(new_job_id)) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],"\t",1) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],buffer, strlen(buffer)) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],"\t",1) < 0) 
            {
                perror("executor-server : write failed");
            }
            if(write(fd[WRITE],queue_position, strlen(queue_position)) < 0) 
            {
                perror("executor-server : write failed");
            }

            // Close and delete the executor-server write pipe
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);

        }
        // Free the buffer used to read from the pipe
        free(buffer);
        
    }
    // If the command_id read is 2 then the command is setConcurrency so change the concurrency accordingly
    else if(command_id == '2') {
        
        //Initialize a buffer to read the new concurrency
        //Initialize an offset variable so the pipe contents can be read dynamically
        char* buffer = calloc(5,1);
        int offset=0;

        while(read(fd[READ], buffer+offset, 1) > 0) {
            offset++;
        }
        //Close the commander write pipe
        close(fd[READ]);
        unlink(commander_write_fifo);

        //Convert the new concurrency given to an integer 
        int new_concurrency = atoi(buffer);

        //Free the buffer used to read the pipe
        free(buffer);

        //if the new concurrency is greater than the previous one start executing jobs from the waiting queue until the maximum concurrent jobs are running
        Triplet* temp_triplet;
        Match* temp_match; 
        for(int i = concurrency; i < new_concurrency; i++ ) {
            
            //Remove the first job from the queue and add it to the running list
            temp_triplet = queue_remove_front(waiting_queue) ;

            //If the waiting queue is empty (queue_remove_front returns NULL) then break the loop since there arent any proccesses waiting to be executed
            if( temp_triplet == NULL)
                break;

            int child_pid;
            // Use fork and execvp to run the job
            if ((child_pid = fork()) < 0) {
                perror("ExecutorServer fork");
                exit(1);
            }
            //if its the child proccess :
            if (child_pid == 0) {

                 //Break down the job into its arguments and call execvp
                int argc = 0;
                char* argv[20];
                char* token = strtok(temp_triplet->job, " ");

                while(token != NULL) {

                    argv[argc] = token;
                    argc++;
                    token = strtok(NULL, " \n");
                }

                argv[argc] = NULL;

                execvp(argv[0], argv );
                
                //If the exec failed then print an error
                printf("Failed to execvp : %s\n", temp_triplet->job);

                exit(1);
            }

            //if its the parent proccess:

            //Create a match for the new proccess 
            temp_match = match_create(temp_triplet , child_pid);
            
            //Append the match to the running list and increase the number of running proccesses by 1
            list_append(running_list, temp_match);
            running++;
            
        }
        //Update the concurrency accordingly
        concurrency = new_concurrency;
        

        
    }
    //If the command_id is 3 then the command is stop and read the jobID from the pipe and remove it
    else if(command_id == '3') {
        
        //Initialize a buffer to read the jobID that should be removed
        //Initialie an offset to be able to read from the pipe dynamically
        char* buffer = calloc(10,1);
        int offset = 0;

        while(read(fd[READ], buffer+offset, 1) > 0) {
            offset++;
        }
        
        //Close the commander write pipe and delete it 
        close(fd[READ]);
        unlink(commander_write_fifo);

        //Try removing the jobID from the waiting queue : If it returns NULL it means the job is not in the waiting queue so check the runnin list 
        //                                                If it returns != NULL then the job was in the queue so it was removed

        Triplet* temp_triplet = queue_remove_jobID(waiting_queue,buffer);

        //Open the executor-server writing pipe
        if ((fd[WRITE] = open(executor_server_write_fifo,  O_WRONLY )) < 0)  {
                perror("executor-server : fifo open error");
                exit(1);
        }
        //If it existed in the waiting queue
        if(temp_triplet != NULL)
        {
            //Write the response on the pipe 
            if(write(fd[WRITE],temp_triplet->job_ID,strlen(temp_triplet->job_ID)) < 0 )
            {
                perror("executor-server : write error");
                exit(1);
            }
            
            if(write(fd[WRITE]," removed" , 9) < 0 )
            {
                perror("executor-server : write error");
                exit(1);
            }

            //Close the executor-server writing pipe 
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);
            
            //Destroy the triplet of the job that was removed
            triplet_destroy(temp_triplet);
        }
        //If it didn't exist in the waiting queue
        else {
            
            // Try removing the jobID from the running list : If it returns NULL it means the job doesn't exist
            //                                                If it returns != NULL then the job was in the list so it was removed               

            Match* temp_match = list_remove_jobID(running_list,buffer);
            //The job didn't exist so let the commander know by writting a response in the executor-server write pipe
            if (temp_match == NULL) {

                if(write(fd[WRITE],"JobID Doesn't Exist" , 20) < 0 )
                {
                    perror("executor-server : write error");
                    exit(1);
                }
                close(fd[WRITE]);
                unlink(executor_server_write_fifo);
            }
            //The job existed in the list so it was removed 
            else {
                //Re-insert the match removed to the running list so the child_signal_handler can find which child proccess was killed.
                list_append(running_list,temp_match);
                
                //Kill the child proccess running the job (use SIGIKILL so it can't be ignored)
                kill(temp_match->pid,SIGKILL);

                //Write the response on the pipe 
                if(write(fd[WRITE],temp_match->triplet->job_ID,strlen(temp_match->triplet->job_ID)) < 0 )
                {
                    perror("executor-server : write error");
                    exit(1);
                }
                if(write(fd[WRITE]," terminated" , 12) < 0 )
                {
                    perror("executor-server : write error");
                    exit(1);
                }
                //Close the executor-server writing pipe 
                close(fd[WRITE]);
                unlink(executor_server_write_fifo);

                
            }
        }
        //Free the buffer used to read the jobID
        free(buffer);
    }
    //if the command id is 3 then the command is poll to read the next argument to know if its "queued" or "running" and act accordingly
    else if(command_id == '4') {

        //Initialize a buffer to read the argument from the pipe
        //Initialize an offset to be able to read dynamically
        char* buffer = calloc(10,1);
        int offset = 0;

        while(read(fd[READ], buffer+offset, 1) > 0) {
            offset++;
        }
        //Close and delete the commander write pipe
        close(fd[READ]);
        unlink(commander_write_fifo);

        //Open the executor-server write pipe
        if ((fd[WRITE] = open(executor_server_write_fifo,  O_WRONLY)) < 0)  {
            perror("executor-server : fifo open error");
            exit(1);
        }

        //if the command was poll running access the running list from (first to last) and copy it in the pipe for the commander to read
        if(strncmp(buffer,"running",7) == 0)
        {
            if(write(fd[WRITE], "\nRunning:\n" , strlen("\nRunning:\n")) < 0 ){

                perror("executor-server: write failed");
                exit(1);
            }
            //For every node of the list write its contents in the pipe
            for(List_Node* node = running_list->first ; node != NULL; node = node->next) {

                if(write(fd[WRITE], node->match->triplet->job_ID , strlen(node->match->triplet->job_ID)) < 0 ){

                    perror("executor-server: write failed");
                    exit(1);
                }
                
                if(write(fd[WRITE], "\t" , 1) < 0 ) {
                    perror("executor-server: write failed");
                    exit(1);
                }

                if(write(fd[WRITE], node->match->triplet->job , strlen(node->match->triplet->job)) < 0){

                    perror("executor-server: write failed");
                    exit(1);
                }

                if(write(fd[WRITE], "\t" , 1) < 0) {
                    perror("executor-server: write failed");
                    exit(1);
                }

                char pos_str[5];
                snprintf(pos_str,sizeof(pos_str), "%d", node->match->triplet->position);


                if(write(fd[WRITE], pos_str , strlen(pos_str)) < 0 ){

                    perror("executor-server: write failed");
                    exit(1);
                }
                
                if(write(fd[WRITE], "\n" , 1) < 0) {
                    perror("executor-server: write failed");
                    exit(1);
                }

            }
            //Close the executor-server pipe
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);

        }
        //if the command was poll queued access the waiting queue from (front to rear) and copy it in the pipe for the commander to read
        else if ( strncmp(buffer , "queued",6) == 0) {

            if(write(fd[WRITE], "\nQueued:\n" , strlen("\nQueued:\n")) < 0 ){

                perror("executor-server: write failed");
                exit(1);
            }
            //For every node of the queue write its contents in the pipe
            for(Queue_Node* node =  waiting_queue->front ; node != NULL; node = node->next) {
                
                if(write(fd[WRITE], node->triplet->job_ID , strlen(node->triplet->job_ID)) < 0){

                    perror("executor-server: write failed");
                    exit(1);
                }
                
                if(write(fd[WRITE], "\t" , 1) < 0) {
                    perror("executor-server: write failed");
                    exit(1);
                }

                if(write(fd[WRITE], node->triplet->job , strlen(node->triplet->job)) < 0){

                    perror("executor-server: write failed");
                    exit(1);
                }

                if(write(fd[WRITE], "\t" , 1) <0) {
                    perror("executor-server: write failed");
                    exit(1);
                }

                char pos_str[5];
                snprintf(pos_str,sizeof(pos_str), "%d", node->triplet->position);


                if(write(fd[WRITE], pos_str , strlen(pos_str)) <0 ){

                    perror("executor-server: write failed");
                    exit(1);
                }
                
                if(write(fd[WRITE], "\n" , 1) < 0) {
                    perror("executor-server: write failed");
                    exit(1);
                }

            }
            //Close the executor-server pipe
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);

        }
        //if the command was invalid (neither "queued" nor running) then write an error in the pipe for the commander to read
        else {

            if(write(fd[WRITE], "Error : Invalid Command: ", strlen("Error : Invalid Command ")) < 0 ){

                    perror("executor-server: write failed");
                    exit(1);
            }
            
            if(write(fd[WRITE],buffer, strlen(buffer)) < 0 ){

                    perror("executor-server: write failed");
                    exit(1);
            }
            //Close the executor-server pipe
            close(fd[WRITE]);
            unlink(executor_server_write_fifo);

        }
        //Free the buffer used to read the argument
        free(buffer);

    }
    //If the command id 5 exit the executor-server and inform the commander
    else if(command_id == '5') {

        //Open the executor-server write pipe
        if ((fd[WRITE] = open(executor_server_write_fifo,  O_WRONLY)) < 0)  {
            perror("executor-server : fifo open error");
            exit(1);
        }
        
        if(write(fd[WRITE], "\nExecutor-Server Terminated\n", strlen("\nExecutor-Server Terminated\n")) < 0 ) {

            perror("executor-server: write failed");
            exit(1);
        }
        //Destroy the waiting queue and the running list
        queue_destroy(waiting_queue);
        list_destroy(running_list);

        //Close and destroy the executor-server write pipe
        close(fd[WRITE]);
        unlink(executor_server_write_fifo);
        //Delete the text file with the pid of the jobExecutorServer
        unlink("jobExecutorServer.txt");

        exit(0);

    }
    //If the command id wasn't 1,2,3,4 or 5 print an error for invalid command
    else {
        printf("Invalid Command Id\n");
        exit(1);
    }

}

//This handler is responsible to receive the SIGCHLD that every child proccess sents to it's parent (the executor-server) when it exits
//It removes the job from the running list and executes proccesses from the waiting list
void child_signal_handler(int sig_num) {

    //Use a loop and waitpid (with WNOHANG flag) to find out which proccess from the ruuning queue was the one that sent the signal
    int status;
    List_Node* node;
    for(node = running_list->first ; node != NULL ; node = node->next) {

        if (waitpid(node->match->pid,&status,WNOHANG) != 0 ) {
            break;
        }

    }

    //if no job is found, it means something went wrong so print an error
    if (node == NULL) {
        printf("SIGCHLD handler : Could not find child proccess that exited\n");
        exit(1);
    }

    // Remove the job that ended from the running list and decrease the number of proccesses running by 1
    Match* match = list_remove_pid(running_list,node->match->pid);
    running--;

    //Destroy the cooresponding match freeing the memory
    if(match != NULL) 
        match_destroy(match);
    
    //If the number of running proccesses is less than the max concurrency then try running proccesses until its reached

    Triplet* temp_triplet;
    Match* temp_match; 
    for(int i = running; i < concurrency; i++ ) {
        
        //Remove the first job from the queue and add it to the running list
        temp_triplet = queue_remove_front(waiting_queue) ;

        //If the queue was empty (the triplet is NULL) then break the loop since there not any proccesses waiting to be executed
        if(temp_triplet == NULL) {
            break;
        }
        else {
            int child_pid;
            // Use fork and execvp to run the job
            if ((child_pid = fork()) < 0) {
                perror("ExecutorServer fork");
                exit(1);
            }
            //if its the child proccess :
            if (child_pid == 0) {

                //Break down the job into its arguments and call execvp
                int argc = 0;
                char* argv[20];
                char* token = strtok(temp_triplet->job, " ");

                while(token != NULL) {

                    argv[argc] = token;
                    argc++;
                    token = strtok(NULL, " \n");
                }

                argv[argc] = NULL;

                execvp(argv[0], argv );
                
                printf("Failed to execvp : %s\n", temp_triplet->job);

                exit(1);
            }

            //if its the parent : Create a new match for the job (proccess), append it in the running list and increase the number of running proccesses by 1

            temp_match = match_create(temp_triplet , child_pid);

            list_append(running_list, temp_match);
            
            running++;
        }
    }

}