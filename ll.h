/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation

    Contains linkedlist and node implementation where nodes contain Process
    pointers from the proc.h specification.
*/

#include "proc.h"

// --- Constants ---
#define SJF_I 0
#define RR_I 1


typedef struct node_S {
    Process* process;
    struct node_S* next;
} node;

typedef struct {
    node* head;
    node* tail;
    int size;
} linkedList;

void freeLinkedList(linkedList* list);
void freeNode(node* n);

void insertLLNode(linkedList* list, node* n, int scheduler);
void insertLLData(linkedList* list, Process* p);

linkedList* createLinkedList(node* n);
linkedList* createEmptyLL();

node* createNode(Process* p);

int remainingTime(linkedList* list);

node* pop(linkedList* list);
