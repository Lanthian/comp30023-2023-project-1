/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation
*/

#include <stdlib.h>
#include <stdio.h>

// Local header
#include "ll.h"

// --- Constants ---
#define TIME_WEIGHT_I 2


// -- Local Function headers --
void iterativeFreeNode(node* n);


/*
  Calls `iterativeFreeNode` to recursively free the nodes in a linked list. 
  Linked list `list` still needs to be set to NULL on call.
*/
void freeLinkedList(linkedList* list) {
    // Only free head if not empty
    if (list->head != NULL) iterativeFreeNode(list->head);
    list->head = NULL;
    list->tail = NULL;
    free(list);
}


/*
  Recursively steps through nodes, freeing them (and setting them to NULL) start
  to back. Node initially called on still needs to be set to NULL.
*/
void iterativeFreeNode(node* n) {
    // Step through nodes freeing all sequential nodes before itself
    if (n->next != NULL) {
        iterativeFreeNode(n->next);
        n->next = NULL;
    }
    freeNode(n);
}


/*
  Frees a node pointer `n`. Remember to set `n` to NULL wherever this is called.
*/
void freeNode(node* n) {
    freeProcess(n->process);
    n->process = NULL;
    free(n);
}


linkedList* createLinkedList(node* n) {
    // Assumes n->next == NULL to work
    linkedList* ll = (linkedList*)malloc(sizeof(linkedList));
    if (ll == NULL) {
        // No space for linkedlist to be malloced
        return NULL;
    }
    ll->head = n;
    ll->tail = n;
    ll->size = 1;

    return ll;
}


linkedList* createEmptyLL() {
    // Assumes n->next == NULL to work
    linkedList* ll = (linkedList*)malloc(sizeof(linkedList));
    if (ll == NULL) {
        // No space for linkedlist to be malloced
        return NULL;
    }
    ll->head = NULL;
    ll->tail = NULL;
    ll->size = 0;

    return ll;
}


node* createNode(Process* p) {
    node* n = (node*)malloc(sizeof(node));
    if (n == NULL) {
        // No space for node to be malloced
        return NULL;
    }
    n->process = p;
    n->next = NULL;

    return n;
}


// TODO - better strat
void insertLLNode(linkedList* list, node* n, int scheduler) {
    list->size += 1;

    // If empty linked list format accordingly
    if (list->head == NULL) {
        list->head = n;
        list->tail = n;
        return;
    }

    // Add to end of the list if non SJF scheduler
    if (scheduler != SJF_I) {
        list->tail->next = n;
        list->tail = n;
    }   
    else {
        // Check if list->head needs to be replaced
        if (compareProcess(list->head->process, n->process) > 0) {
            n->next = list->head;
            list->head = n;
            return;
        }

        // Step through list until end reached or larger run time process found
        node* temp_node = list->head;
        while ((temp_node->next != NULL) && (compareProcess(temp_node->next->process, n->process) <= 0)) {
            temp_node = temp_node->next;
        }

        // Insert point or end reached.
        if (temp_node->next == NULL) {
            // Insert at end
            list->tail->next = n;
            list->tail = n;
            return;
        }
        else {
            // Insert mid list
            n->next = temp_node->next;
            temp_node->next = n;
            return;
        }
    }
}


// TODO - currently just appends at list end
void insertLLData(linkedList* list, Process* p) {
    // If no space for new process just return, having done nothing
    node* n = createNode(p);
    // todo -- add notice here that nothing has happened
    if (n==NULL) return;

    insertLLNode(list, n, TIME_WEIGHT_I);
}


/*
  Takes a linkedlist of current processes and sums their total time left, 
  returning this integer.
*/
int remainingTime(linkedList* list) {
    int remaining = 0;
    node* step = list->head;

    // Increment through list
    while (step != NULL) {
       remaining += step->process->time_left;
       step = step->next;
    }

    return remaining;
}


/*
  Pops the first element out of a linked list and returns this node pointer.
*/
node* pop(linkedList* list) {
    // Check list isn't empty (return NULL if so)
    if (list->head == NULL) return NULL;

    node* output = list->head;
    // If last element in list set head and tail to NULL
    if (list->head == list->tail) {
        list->tail = NULL;
    }
    list->head = list->head->next;
    list->size -= 1;

    output->next = NULL;
    return output;
}
