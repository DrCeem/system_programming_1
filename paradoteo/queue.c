#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"


//////////////////////////////////////// Functions For the Triplet struct (To create and detroy) //////////////////////////////////////////////////

//Triplet Create : Takes the jobID and the job as parametres, allocates the required memory and creates an object of type "Triplet" 
// The initial value for position is -1 indicating that the triplet is not inserted in the queue yet. 
Triplet* triplet_create(char* jobID, char* job) {

    Triplet* triplet = (Triplet*)malloc(sizeof(Triplet));

    triplet->job_ID = (char*)malloc(strlen(jobID));
    triplet->job = (char*)malloc(strlen(job));

    strcpy(triplet->job_ID , jobID);
    strcpy(triplet->job,job);

    triplet->position = -1;

}

//Triplet Destroy : Frees all the memory for the given triplet
void triplet_destroy(Triplet* triplet) {
    
    free(triplet->job);
    free(triplet->job_ID);

    free(triplet);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Functions For the Queue /////////////////////////////////////////////////////////////////////////

//Queue create
Queue* queue_create() {

    //Allocate memory for the queue
    Queue* queue = (Queue*)malloc(sizeof(Queue));   


    //If allocation failed print error message
    if (queue == NULL) {

        printf("queue_create : Memory Allocation Failed\n");
        exit(1);
    }

    //Initialize the front and rear pointers to be NULL (as the queue is initialized empty)

    queue->front = NULL;
    queue->rear = NULL;

    return queue;

}

// A function that takes a queue, the job id and the job and inserts a node to the queue
void queue_insert_rear(Queue* queue , Triplet* triplet ) {
    
    // Allocate memory for the new node
    Queue_Node* node = (Queue_Node*)malloc(sizeof(Queue_Node));

    if (node == NULL) {

        printf("queue_insert_rear : Memory Allocation Failed\n");
        exit(1);
    }

    // Initialize the node accordingly 

    node->next = NULL;
    node->triplet = triplet;
    
    // If "front" is NULL (meaning the queue is empty), then "front" and "rear" become the newly enqueued node and the position (in queue) is 0 
    if (queue->front == NULL) {
        queue->front = node;
        node->triplet->position = 0;
    }
    // if "rear" is not NULL set the next node off the previous "rear" the node inserted and set the position of the node inserted as the previous rear's + 1
    if (queue->rear != NULL) {
        queue->rear->next = node;
        node->triplet->position = queue->rear->triplet->position + 1 ;
    }
    
    // Rear will now point to the new node 
    queue->rear = node;

}

//Removes the front element of a queue and returns it if it succeeds or NULL if it fails 
Triplet* queue_remove_front(Queue* queue) {

    //If the queue is empty print an error
    if(queue->front == NULL) {
        return NULL;
    }

    Queue_Node* to_remove = queue->front;

    //Set the next node of the front as the new front 
    queue->front = to_remove->next;
    
    //If there was only 1 element in the queue after removing it set the rear pointer to NULL as well
    if(queue->front == NULL)
        queue->rear = NULL;
    
    //If there were still elements in the queue after removing the front reduce all their positions by 1 (since the front was removed)
    else {
        for(Queue_Node* temp = queue->front ; temp != NULL ; temp = temp->next)
        {
            temp->triplet->position--;
        }
    }

    // Get the triplet of the front node 
    Triplet* triplet = to_remove->triplet;

    //Free the node removed
    free(to_remove);

    return triplet;
        
}

//Remove the node with job_ID and return it or NULL if it wasnt found.
Triplet* queue_remove_jobID(Queue* queue,char* job_ID) {
        //Initialize 2 node pointers to get the node that has to be removed and its previous one (to modify its "next" pointer)
        Queue_Node* previous_node = NULL;
        Queue_Node* node;

        // Access the nodes (from front to rear) until either the node with that job_ID is found or the end of the queue is reached
        for( node = queue->front ; node != NULL ; node = node->next)
        {
            if(strcmp(job_ID,node->triplet->job_ID)==0) {
                break;
            }
            previous_node = node;
        }

        //If the job_ID wasn't found return NULL
        if(node == NULL) {
            return NULL;
        }
        //If previous node is not NULL make the previous node point to the next one.
        if(previous_node != NULL) {
            previous_node->next = node->next;
        
        }
        //If the previous node is NULL it means the first one is the one being removed so set front as the next one.
        else {
            queue->front = node->next;
        }
        //For all the nodes that were "after" the "to be removed" node have their position decreased by 1
        for (Queue_Node* temp = node->next; temp != NULL; temp = temp->next) {
            temp->triplet->position--;
        }
        //Finally if it is the rear node that has to be removed (and the list has more than 1 jobs) then make rear point to the previous node 
        if(strcmp(queue->rear->triplet->job_ID,job_ID) == 0 ) {
            queue->rear = previous_node;
        }

        //Find the triplet that has to be removed
        Triplet* triplet = node->triplet;
        //Free the cooresponding node 
        free(node);

        return triplet;
}


// A function that prints the contents of each node (from front to back)
void print_queue(Queue queue) {
    for(Queue_Node* node = queue.front ; node != NULL ; node = node->next) {
        printf("Job ID: %s \tJob %s \tPos :%d \n",node->triplet->job_ID,node->triplet->job, node->triplet->position);
    }
}

// Destroys the queue and frees the memory
void queue_destroy(Queue* queue) {

    Queue_Node* next;
    // Access all the queue nodes from front to rear and free them one by one
    for(Queue_Node* node = queue->front; node != NULL ; node = next ) {
        next = node->next;
        triplet_destroy(node->triplet);
        free(node);
    }
    free(queue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////// TEST CASES FOR QUEUE STRUCTS AND FUNCTIONS ///////////////////////////////////////////////////////////

// int main() {

//     Queue* queue = queue_create();

//     char job_id[10] = "job_";
//     char job[12] = "Nothing_";

//     for(int i = 0; i < 10; i++){

//         snprintf(job_id + 4, sizeof(job_id) , "%d" , i );
//         snprintf(job + 8, sizeof(job) , "%d" , i );

//         queue_insert_rear(queue,job_id,job);
//     }
//     print_queue(*queue);
    
//     //Dequeue the front 
//     queue_remove_front(queue);
//     printf("\nQueue after remove fronnt\n");
//     print_queue(*queue);
//     //Remove job with job_ID = job_1
//     char* a_job = "job_1" ;
//     queue_remove_jobID(queue,a_job);
//     printf("\nQueue after remove job_1\n");
//     print_queue(*queue);
//     queue_destroy(queue);


//     return 0;
// }



