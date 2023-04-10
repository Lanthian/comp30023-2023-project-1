/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Local headers ---
// #include "proc.h"        proc.h included in ll.h todo
#include "ll.h"


// --- Constants ---
#define FILENAME 'f'
#define SCHEDULER 's'
#define MEMORY_STRATEGY 'm'
#define QUANTUM 'q'
// Schedulers
#define SJF "SJF"
#define RR "RR"
// Mem Strats
#define INFINITE "infinite"
#define INFINITE_I 0
#define BEST_FIT "best-fit"
#define BEST_FIT_I 1

#define STARTING_MEMORY 2048
#define MEM_EMPTY 0
#define MEM_FILLED 1

// Arg indexes
#define MINUS_INDEX 0
#define COMMAND_INDEX 1


int main(int argc, char* argv[]) {
    // --- Prepare program ---
    FILE* fp = NULL, *OUTPUT = stdout;
    int scheduler, mem_strat, quantum;
    int clock = 0, memory_left = STARTING_MEMORY;
    int memory[STARTING_MEMORY];
    for (int i=0; i<STARTING_MEMORY; i++) {
        memory[i] = MEM_EMPTY;
    }


    // --- Read in arguments ---
    char CURRENT_ARG;
    int arg_ready = 0;

    for (int i = 0; i < argc; i++) {
        // Check for new argument flag
        if (argv[i][MINUS_INDEX] == '-') {
            CURRENT_ARG = argv[i][COMMAND_INDEX];
            arg_ready = 1;

        } else if (arg_ready == 1) {
            // Otherwise read in current argument
            switch (CURRENT_ARG) {
                // - Assign filename -
                case FILENAME:
                    fp = fopen(argv[i], "r");
                    // Make sure file was opened successfully
                    if (fp == NULL) {
                        printf("Failed to open file [%s]\n", argv[i]);
                        exit(EXIT_FAILURE);
                    }
                    break;

                // - Assign scheduler -
                case SCHEDULER:
                    if (strcmp(argv[i], SJF) == 0) 
                        scheduler = SJF_I;
                    else if (strcmp(argv[i], RR) == 0)
                        scheduler = RR_I;
                    else {
                        printf("Invalid choice of scheduler: [%s]\n", argv[i]);
                        exit(EXIT_FAILURE);
                    } break;

                // - Assign memory-strategy -
                case MEMORY_STRATEGY:
                    if (strcmp(argv[i], INFINITE) == 0) 
                        mem_strat = INFINITE_I;
                    else if (strcmp(argv[i], BEST_FIT) == 0)
                        mem_strat = BEST_FIT_I;
                    else {
                        printf("Invalid choice of memory-strategy: [%s]\n", argv[i]);
                        exit(EXIT_FAILURE);
                    } break;

                // - Assign quantum -
                case QUANTUM:
                    quantum = atoi(argv[i]);
                    break;
                default:
                    printf("Invalid command line flag input: [-%c]\n", CURRENT_ARG);
            }

            // Toggle wait back on
            arg_ready = 0;
        }
    }

    // Assert file read in
    if (fp == NULL) exit(EXIT_FAILURE);


    // --- Read in processes from file ---
    Process* first_Proc = readInProcess(fp);
    // Abort now if no processes read in or any step fails
    if (first_Proc == NULL) return 0;
    // Generate linkedlist with first process
    linkedList* unloaded = createLinkedList(createNode(first_Proc));
    linkedList* ready = createEmptyLL();

    // Keep reading processes into the linkedlist
    Process* temp_Proc = readInProcess(fp);
    while (temp_Proc != NULL) {
        insertLLData(unloaded, temp_Proc);
        // Read in next line
        temp_Proc = readInProcess(fp);
    }


    // --- Work through processes ---
    float avg_turnaround = 0;
    float max_overhead = 0, avg_overhead = 0;
    int total_processes = unloaded->size;

    float temp_turnaround, temp_overhead;

    // Ordering works on the assumption that processes with zero work time cannot exist
    node* current_p_node = NULL;
    int can_store = 1;
    while((unloaded->head != NULL) || (ready->head != NULL) || (current_p_node != NULL)) {

         // - Check conditions of current process -
        if (current_p_node!=NULL) {
            // - Print out and remove completed processes - 
            if (getTimeLeft(current_p_node->process) == 0) {
                fprintf(OUTPUT, "%d,FINISHED,process_name=%s,proc_remaining=%d\n",
                    clock, current_p_node->process->name, ready->size);

                // Free simulated memory
                int loc = getMemLoc(current_p_node->process);
                int size = getMemSize(current_p_node->process);
                for (int i=loc; i<loc+size;i++) {
                    memory[i] = MEM_EMPTY;
                }
                memory_left += size;
                setMemLoc(current_p_node->process, MEM_UNASSIGNED);
                
                // Track turnaround
                temp_turnaround = clock - getReadTime(current_p_node->process);
                avg_turnaround += temp_turnaround;
                // Track overhead
                temp_overhead = temp_turnaround / (float)getServiceTime(current_p_node->process);
                avg_overhead += temp_overhead;
                if (temp_overhead > max_overhead) max_overhead = temp_overhead;

                freeNode(current_p_node);
                current_p_node = NULL;
            }

            // - Otherwise requeue process to ready queue if using RR approach -
            else if ((scheduler == RR_I) && ready->size>0) {
                insertLLNode(ready, current_p_node, scheduler);
                current_p_node = NULL;
            }
        }


        // - Switch all now ready and storable processes to the ready queue - 
        while((unloaded->size>0) && (getReadTime(unloaded->head->process) <= clock)) { 
            // - Best Fit memory code (skip all this if infinite) -
            if (mem_strat == BEST_FIT_I) {
                // Check there is enough memory to ready the process
                int process_size = getMemSize(unloaded->head->process);
                if (process_size > memory_left) {
                    // Firstly check if process will ever be assignable - print error message and quit program if no
                    if (process_size > STARTING_MEMORY) {
                        fprintf(OUTPUT, "Process <%s> has an unassignable size of <%d>. Program terminating.\n",
                            unloaded->head->process->name, process_size);
                        exit(EXIT_FAILURE);
                    }
                    // Otherwise just break and try assigning once freeing up some other processes
                    break;      // todo - might have to manage this differently
                }

                // Search for a continuous portion of memory to allocate the process
                int starting_point;
                for (starting_point=0; starting_point<STARTING_MEMORY;) {
                    can_store = 1;

                    for (int current_point=starting_point; (current_point<STARTING_MEMORY) && (current_point<starting_point+process_size); current_point++) {
                        // If filled memory found, range from starting_point will not suffice
                        if (memory[current_point] == MEM_FILLED) {
                            // Jump memory "pointer" ahead for efficiency
                            starting_point=current_point+1;
                            can_store = 0;
                            break;          
                        }
                    }

                    if (can_store) {
                        break;
                    }
                } 

                if (can_store) {
                    // Allocate memory and add process to ready queue.
                    for (int i=starting_point; i<starting_point+process_size; i++) {
                        memory[i] = MEM_FILLED;
                    }

                    setMemLoc(unloaded->head->process, starting_point);
                    memory_left -= process_size;
                    
                    fprintf(OUTPUT, "%d,READY,process_name=%s,assigned_at=%d\n", 
                        clock, unloaded->head->process->name, starting_point);
                } else if (!can_store) {
                    // If still not allocated - not enough continuous memory - store it later
                    break;
                }
            }
            insertLLNode(ready, pop(unloaded), scheduler);
        }


        // - Start working most ready process if not already working -
        if ((ready->size>0) && (current_p_node == NULL)) {
            current_p_node = pop(ready);
            // Print start update
            fprintf(OUTPUT, "%d,RUNNING,process_name=%s,remaining_time=%d\n",
                clock, current_p_node->process->name, 
                getTimeLeft(current_p_node->process));
        }

        // - Work process if ready and queued -
        if (current_p_node != NULL) {
            workProcess(current_p_node->process, quantum);
        }

        // - Tick clock, quitting before final quantum tick if done -
        if ((ready->size == 0) && (unloaded->size == 0) && (current_p_node == NULL)) break;
        clock+=quantum;
    }

    // Free data structures
    freeLinkedList(unloaded);
    unloaded = NULL;
    freeLinkedList(ready);
    ready = NULL;


    // --- Print summary ---
    fprintf(OUTPUT, "Turnaround time %.0f\n", avg_turnaround/(float)total_processes);
    fprintf(OUTPUT, "Time overhead %.2f %.2f\n", max_overhead, avg_overhead/(float)total_processes);
    fprintf(OUTPUT, "Makespan %d\n", clock);


    return 0;
}
