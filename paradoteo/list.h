#include <stdio.h> 
#include "queue.h"

// A struct matching a triplet of a job : <jobID,job,position> to the cooresponding proccess ID
typedef struct job_pid_match {

    int pid;
    Triplet* triplet;

} Match ;

// A struct to represent the nodes of the list containing the running proccesses which contain a match and a pointer to the next node of the list
typedef struct list_node {
    Match* match;
    struct list_node* next;
} List_Node;

// A sruct to represent a list of "matches" 
typedef struct list {
    List_Node* first;
    List_Node* last;
    int size;
} List;

// A function initializing a match given a triplet and the proccess id
Match* match_create(Triplet* triplet ,int pid);

// A function that destroys the match given freeing all the memory
void match_destroy(Match* match);

// Creates an empty list allocating the needed memory
List* list_create();

// Appends the match given at the end of the list given (adjusting the triplet position field accordingly)
void list_append(List* list, Match* match) ;

// Finds, removes and returns the match with the jobID given from list (if it exists). If it doens't exist returns NULL
Match* list_remove_jobID(List* list, char* jobID) ;

// Finds, removes and returns the match with the pid given from list (if it exists). If it doens't exist returns NULL
Match* list_remove_pid(List* list, int pid);

//Destroys the list given freeing all its memory
void list_destroy(List* list);

//Prints the list given
void print_list(List list);