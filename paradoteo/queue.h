#pragma once

#include <stdio.h>

// A struct to represent the triplets of <jobID,job,queuePosition>
typedef struct triplet {
    char* job_ID;
    char* job;
    int position;
} Triplet;

// A struct for the nodes of the queue containing the job_ID and the job of the node (as strings), the position of the node in the queue (starting from 0)
// and a pointer to the next node 
typedef struct queue_node 
{
    Triplet* triplet;
    struct queue_node* next;

} Queue_Node;

// A struct for the queue containing a front and rear pointer to a queue node
typedef struct queue
{
    Queue_Node* front;
    Queue_Node* rear;

} Queue;

// Creates a triplet allocating the needed memory and copies the jobID ad the job given to the triplet and initializes the position as -1
Triplet* triplet_create(char* jobID, char* job);

//Destroys the triplet given freeing all the memory
void triplet_destroy(Triplet* triplet);

// Creates an empty queue of triplet structs allocating the needed memory
Queue* queue_create();

// Inserts the triplet at the end of the queue given (the position field is adjusted according to the position it was inserted)
void queue_insert_rear(Queue* queue , Triplet* triplet);

// Removes and returns the front (first) element (triplet) from queue. If the queue is empty returns NULL
Triplet* queue_remove_front(Queue* queue);

// Finds, removes and returns the triplet of queue (if it exists) with the jobID given. If a triplet with that JobID doesn't exist returns NULL
Triplet* queue_remove_jobID(Queue* queue,char* job_ID);

//Prints all the elements (triplets of the queue)
void print_queue(Queue queue);

//Destroys the queue freeing all the memory allocated for it
void queue_destroy(Queue* queue);