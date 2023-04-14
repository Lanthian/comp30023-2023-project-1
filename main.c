/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation

    main() implementation of the project.

    Makes use of ll.c and proc.c code.
    Additionally, utilises Steven Tang's process.c code.
*/

#define IMPLEMENTS_REAL_PROCESS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// process.c handling headers
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

// --- Local headers ---
// #include "proc.h"        Not needed as proc.h included in ll.h
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

// process.c togglers and constants
#define PRINT_32BIT_TIMES 0
#define NUM_BYTES 4


// Local function headers
uint8_t* intToBigEndian(int x);
uint8_t* send32bitTime(Process* process, int time, int print_endian_flag);
void verifyLeastSigByte(Process* process, uint8_t* time_32bit);


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
    linkedList* unloaded = createLinkedList(createNode(first_Proc));        // input queue
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

    // Byte array to store sha256 output of processes
    uint8_t sha256[64];

    // Ordering works on the assumption that processes with zero work time cannot exist
    node* current_p_node = NULL;
    int can_store = 1;
    while((unloaded->head != NULL) || (ready->head != NULL) || (current_p_node != NULL)) {
        // printf("%d -- clock\n", clock);      // todo - remove

        // - Print out and remove completed processes - 
        if ((current_p_node!=NULL) && (getTimeLeft(current_p_node->process) == 0)) {
            fprintf(OUTPUT, "%d,FINISHED,process_name=%s,proc_remaining=%d\n",
                clock, current_p_node->process->name, ready->size);
            
            // Terminate real process
            uint8_t* time_32bit = send32bitTime(current_p_node->process, clock, PRINT_32BIT_TIMES);
            free(time_32bit);
            time_32bit = NULL;

            kill(current_p_node->process->pid, SIGTERM);
            read(current_p_node->process->fd_from_c, &sha256, sizeof(sha256));
            fprintf(OUTPUT, "%d,FINISHED-PROCESS,process_name=%s,sha=%s\n",
                clock, current_p_node->process->name, sha256);
            

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

                        freeLinkedList(unloaded);
                        unloaded = NULL;
                        freeLinkedList(ready);
                        ready = NULL;
                        exit(EXIT_FAILURE);
                    }
                    // Otherwise just break and try assigning once freeing up some other processes
                    break;      
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


        // - Requeue current process to ready queue if using RR approach and another process is available -
        if ((current_p_node!=NULL) && ((scheduler == RR_I) && ready->size>0)) {
            // Suspend real process
            uint8_t* time_32bit = send32bitTime(current_p_node->process, clock, PRINT_32BIT_TIMES);
            free(time_32bit);
            time_32bit = NULL;
            kill(current_p_node->process->pid, SIGTSTP);

            // (making sure process is suspended before continuing - wait for SIGTSTP signal to take affect)
            int wstatus;
            while (1) {
                pid_t w = waitpid(current_p_node->process->pid, &wstatus, WUNTRACED);
                if (w==-1) {
                    fprintf(OUTPUT, "ERROR: Error waiting on pid [%d] to change.\n", current_p_node->process->pid);
                }
                if (WIFSTOPPED(wstatus)) break;
            }

            // Requeue simulated process
            insertLLNode(ready, current_p_node, scheduler);
            current_p_node = NULL;
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
            // If it's the first time a process is run, produce the actual process.exe of it and send it start time
            if (getServiceTime(current_p_node->process) == getTimeLeft(current_p_node->process)) {
                pid_t c_pid;
                int p_fd[2], c_fd[2];       // parent file descriptor, child file discripter
                pipe(p_fd);
                pipe(c_fd);

                c_pid = fork();
                // If child process - exec process
                if (c_pid == 0) {
                    // Link reading and writing of parent and child fd-s
                    close(c_fd[0]);                 // parent reads this - child doesn't need access
                    close(p_fd[1]);                 // parent writes here - child doesn't need access

                    dup2(p_fd[0], STDIN_FILENO);    // new stdin of child (reading from p-out)
                    dup2(c_fd[1], STDOUT_FILENO);   // new stdout of child (writing to c-out)

                    char* args[] = {"./process", current_p_node->process->name, NULL};
                    execv(args[0], args);

                    // If code reaches here, execv has failed
                    fprintf(OUTPUT, "Failure executing process. Aborting program.\n");
                    exit(EXIT_FAILURE);

                // If parent process - archive pid, fd-s and send start sim time
                } else if (c_pid > 0) {
                    // Close unused filedescriptors
                    close(p_fd[0]);                 // child reads this - parent doesn't need access
                    close(c_fd[1]);                 // child writes here - parent doesn't need access
                    // Store child pid and connections with child process
                    setProcessId(current_p_node->process, c_pid);
                    current_p_node->process->fd_from_c = c_fd[0];
                    current_p_node->process->fd_to_c = p_fd[1];

                    // Sent start simulation time and verify least significant byte parsed successfully
                    uint8_t* time_32bit = send32bitTime(current_p_node->process, clock, PRINT_32BIT_TIMES);
                    verifyLeastSigByte(current_p_node->process, time_32bit);

                } else {
                    // Fork error
                    fprintf(OUTPUT, "Failure forking. Exiting program.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Otherwise, update or resume process with new time
                uint8_t* time_32bit = send32bitTime(current_p_node->process, clock, PRINT_32BIT_TIMES);
                kill(current_p_node->process->pid, SIGCONT);
                verifyLeastSigByte(current_p_node->process, time_32bit);
            }
    
            // Work simulated process
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
    fprintf(OUTPUT, "Turnaround time %.0f\n", ceil(avg_turnaround/(float)total_processes));
    fprintf(OUTPUT, "Time overhead %.2f %.2f\n", max_overhead, avg_overhead/(float)total_processes);
    fprintf(OUTPUT, "Makespan %d\n", clock);


    return 0;
}


/**
  Converts a received @param x to / and @returns a uint8_t array of NUM_BYTES 
  bytes.
*/
uint8_t* intToBigEndian(int x) {
    uint8_t *num = (uint8_t*)malloc(sizeof(uint8_t)*NUM_BYTES);
    if (num == NULL) {
        // No space, print error and return NULL
        printf("Error - no space in memory to malloc byte array");
        return NULL;
    }

    uint8_t byte;
    // Iterate through number, breaking into bytes with bitwise operators
    for (int i = 0; i<NUM_BYTES; i++) {
        byte = (x>>(8*(NUM_BYTES-1-i)) & 0xFF);
        // Store bytes
        num[i] = byte;
    }

    return num;
}


/**
  Receives a Process* @param process and a simulation time @param time, plus a 
  boolean flag @param print_endian_flag that determines if a generated `uint8_t`
  Big Endian time is printed to terminal or not.
  @returns the simulation time as a uint8_t array of NUM_BYTES bytes. This value 
  is malloc-d and needs to be freed later or before any form of termination.
*/
uint8_t* send32bitTime(Process* process, int time, int print_endian_flag) {
    // Generate 32 bit simulation time in Big Endian format
    uint32_t utime = time;
    uint8_t* num = intToBigEndian(utime);
    // Print this value to stdout if desired
    if (print_endian_flag) {
        int i;
        for (i=0; i<NUM_BYTES-1; i++) printf("%02x ", num[i]);
        printf("%02x\n", num[i]);
    }
    
    // Send 32 bit start time to process
    int nbytes = write(process->fd_to_c, num, NUM_BYTES);
    if (nbytes == -1) {
        // Failure to write
        printf("Failed to write to process %s of pid [%d], terminating.\n", 
            process->name, process->pid);
        free(num);
        num = NULL;
        exit(EXIT_FAILURE);
    }

    return num;
}


/**
  Receives a Process* @param process (including it's link to an actual real 
  process.c process) and verifies that a 32bit time @param time_32bit sent to it
  was received correctly by checking a least significant byte returned by the 
  process.
  Frees `time_32bit` both if successful and if not. 
*/
void verifyLeastSigByte(Process* process, uint8_t* time_32bit) {
    // Veryify send success by reading back a least significant byte
    uint8_t verifying_byte;
    int nbytes = read(process->fd_from_c, &verifying_byte, 1);
    if (nbytes == -1) {
        // Failure to read
        printf("Failed to read from process %s of pid [%d], terminating.\n", 
            process->name, process->pid);

        // Time 32 bit is malloc-d in intToBigEndian() so free before terminating.
        free(time_32bit);
        time_32bit = NULL;

        exit(EXIT_FAILURE);
    } 
    else if (verifying_byte != time_32bit[NUM_BYTES-1]) {
        // Byte sent does not equal byte read back...
        printf("ERROR: Byte returned via child process stdout [%02x] does not match byte sent [%02x].\n", 
            verifying_byte, time_32bit[NUM_BYTES-1]);
        exit(EXIT_FAILURE);
    }

    // Otherwise, done with 32bit simulation time - free uint8_t array ...
    free(time_32bit);

    // ... and interaction / update was successful!
}
