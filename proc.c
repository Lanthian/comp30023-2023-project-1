/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation
*/

#include <stdlib.h>
#include <stdio.h>

// Local header
#include "proc.h"


/*
  Reads a process in, returning a malloc-d Process pointer. Format read in is: 
  "{time in} {process name} {duration of task} {memory required}".
*/
Process* readInProcess(FILE* fp) {
    // -- Create and allocate space for Process instance --
    Process* proc = (Process*)malloc(sizeof(Process));
    if (proc == NULL) {
        // memory could not be assigned - no space for new point
        return NULL;
    }

    // Scan in and store values before returning
    if (fscanf(fp, "%d %s %d %d", &proc->read_time, proc->name, &proc->service_time, &proc->mem_size) == 4) {
        proc->time_left = proc->service_time;
        proc->assigned_loc = MEM_UNASSIGNED;
        return proc;
    }

    // freeing up memory and return null if not enough information parsed to create a Process
    free(proc);
    return NULL;
}


/*
  Prints a Process pointer's data to output stream `output` in the same format
  that it is read in.
*/
void fprintProcess(FILE* output, Process* process) {
    fprintf(output, "%d %s %d %d\n", process->read_time, process->name, process->time_left, process->mem_size);
}


/*
  Frees a Process pointer `process`, also setting the pointer to NULL.
*/
void freeProcess(Process* process) {
    free(process);
    process = NULL;
}


/*
  Decreases the time left on a Process `process` by a work time defined 
  `quantum`. Time left can never decrease below 0.
*/
void workProcess(Process* process, int quantum) {
    process->time_left -= quantum;
    if (process->time_left <= 0) {
        process->time_left = 0;
    }
}


/*
  Returns a positive integer if first process length is longer than second, 
  negative if reverse, and 0 if equal.
*/
int compareProcess(Process* p1, Process* p2) {
    return (p1->time_left - p2->time_left);
}

/*
  Getter used to verify if process has any time left to be worked.
*/
int isDone(Process* process) {
    return (process->time_left==0);
}


// Remaining getters

int getTimeLeft(Process* process) {
    return process->time_left;
}
int getReadTime(Process* process) {
    return process->read_time;
}
int getServiceTime(Process* process) {
    return process->service_time;
}
int getMemSize(Process* process) {
    return process->mem_size;
}
int getMemLoc(Process* process) {
    return process->assigned_loc;
}

// Setter(s)

void setMemLoc(Process* process, int loc) {
    process->assigned_loc = loc;
}
