#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "queue.h"


//////////////////////////////////////// Functions For the Match struct (To create and detroy) /////////////////////////////////////////////////

Match* match_create(Triplet* triplet ,int pid) {
    
    //Allocate the needed memory
    Match* match = (Match*)malloc(sizeof(Match));
    match->triplet = triplet;
    match->pid = pid;

    return match;
}

void match_destroy(Match* match) {
    // Free the allocated memory
    triplet_destroy(match->triplet);
    free(match);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Functions For the List /////////////////////////////////////////////////////////////////////////

//Creates an empty list struct 
List* list_create() {

    // Allocate the memory
    List* list =(List*)malloc(sizeof(List));
    if (list == NULL) 
    {
        printf("list_create : Failed to allocate memory\n");
        exit(1);
    }
    // Initialize the fields
    list->first = NULL;
    list->last = NULL;
    list->size = 0;

    return list;
}

// Inserts the match given at the end of the list given
void list_append(List* list, Match* match) {

    // Initialize the memory for the new node 
    List_Node* new_node = (List_Node*)malloc(sizeof(List_Node));

    // Set the match of the new node and the next node as NULL (Indicating the end of the list since insertions only happen at the end)
    new_node->match = match;
    new_node->next = NULL;

    // If list->first is NULL meaning the list is empty set both first and last as the new element
    if(list->first == NULL) {

        new_node->match->triplet->position = 0;
        list->first = new_node;
        list->last = new_node;

    }
    // If the list is not empty (list->first not NULL) then set the next node of the previous last node as the newly inserted and then
    // Set list->last as the new node.
    else {

        new_node->match->triplet->position = list->last->match->triplet->position + 1;
        list->last->next = new_node;
        list->last = new_node;

    }
    // Increase the size of the list by one 
    list->size++;
}

// Finds the node which contains the node with jobID and returns the coorespoding match. If it doesn't exist returns NULL
Match* list_remove_jobID(List* list, char* jobID) {
    
    // Find the node with jobID as well as its previous node (to change it's next pointer)
    List_Node* node = NULL;
    List_Node* prev_node = NULL ;

    for(node = list->first; node != NULL ; node = node->next) {
        if (strcmp(jobID, node->match->triplet->job_ID) == 0)
            break;
        prev_node = node;
    }
    
    // If a node with joibID didn't exist return NULL
    if(node == NULL) 
        return NULL;


    Match* to_remove = node->match;

    // If the previous node is NULL it means the node with jobID was the first one so just set first as the next from the one to be removed 
    if(prev_node == NULL) 
        list->first = node->next;
    else {
    // If the previous node exists set it's next as the next node of the "to be removed" node  
        prev_node->next = node->next;
    }
    
    //Decrease the position by 1 of all the nodes after the one to be removed.
    for(List_Node* temp = node->next ; temp!=NULL ; temp = temp->next) {
        temp->match->triplet->position--;
    }

    // Lastly check if the node was the last one in which case after its removal the list->last will point to it's previous node
    if(strcmp(jobID,list->last->match->triplet->job_ID) == 0) 
        list->last = prev_node;

    // Free the node that was deleted 
    free(node);

    list->size--;

    // Return the match 
    return to_remove;

}

// Finds the node which contains the node with pid and returns the coorespoding match. If it doesn't exist returns NULL

Match* list_remove_pid(List* list, int pid) {
    
    List_Node* node = NULL;
    List_Node* prev_node = NULL ;

    // Find the node with jobID as well as its previous node (to change it's next pointer)

    for(node = list->first; node != NULL ; node = node->next) {
        if (pid == node->match->pid) 
            break;
        prev_node = node;
    }

    // If a node with pid didn't exist return NULL
    if(node == NULL) 
        return NULL;

    Match* to_remove = node->match;

    // If the previous node is NULL it means the node with pid was the first one so just set first as the next from the one to be removed 
    if(prev_node == NULL) 
        list->first = node->next;

    // If the previous node exists set it's next as the next node of the "to be removed" node  
    else 
        prev_node->next = node->next;

    //Decrease the position by 1 of all the nodes after the one to be removed.
    for(List_Node* temp = node->next ; temp!=NULL ; temp = temp->next) {
        temp->match->triplet->position--;
    }

    // Lastly check if the node was the last one in which case after its removal the list->last will point to it's previous node
    if(pid == list->last->match->pid)
        list->last = prev_node;

    // Free the node that was deleted
    free(node);

    //Decrease the size by 1
    list->size--;

    // Return the match
    return to_remove;

}
//Destroys the list given
void list_destroy(List* list) {
    
    List_Node* next;
    
    // Access the nodes of the list and destroy them one by one
    for(List_Node* node = list->first; node != NULL; node = next) {

        match_destroy(node->match);

        next = node->next;

        free(node);
    }
    free(list);
}

//Prints the contents of the list given
void print_list(List list) {
     for(List_Node* node = list.first; node != NULL; node = node->next) {

        printf("JobID: %s\tJob:%s\tList Position %d\tPID: %d\n",node->match->triplet->job_ID,node->match->triplet->job,node->match->triplet->position,node->match->pid);

    }

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////// TEST CASES FOR LISTSTRUCTS AND FUNCTIONS ////////////////////////////////////////////////////////////

// int main() {

//     List* list = list_create();

//     char job_id[10] = "job_";

//     Match* match[10];
//     int i;
//     for(i = 0; i < 10; i++){

//         snprintf(job_id + 4, sizeof(job_id) , "%d" , i );

//         match[i] = match_create(job_id,i);QUEUE 

//         list_append(list,match[i]);
//     }
//     print_list(*list);

//     //Remove job_ID = job_7
//     char* tmp = "job_7";
//     list_remove_jobID(list,tmp);
//     printf("\nList with removed job_7\n");
//     print_list(*list);

//     //Remove job with pid = 5
//     list_remove_pid(list,5);
//     printf("\nList after job with pid=5 removed\n");
//     print_list(*list);
    

//     list_destroy(list);

//     return 0;
// }
