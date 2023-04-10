/*
    Created by Liam Anthian: 2023.04.05 for 
    University of Melbourne, COMP30023 Project 1 implementation
*/

#define MAX_NAME_LENGTH 50
#define MEM_UNASSIGNED -1

typedef struct {
    int read_time;
    char name[MAX_NAME_LENGTH];
    int service_time;
    int time_left;
    int mem_size;
    int assigned_loc;
} Process;


Process* readInProcess(FILE* fp);

void fprintProcess(FILE* output, Process* process);

void freeProcess(Process* process);

void workProcess(Process* process, int quantum);

int compareProcess(Process* p1, Process* p2);

int isDone(Process* process);


// Getters
int getTimeLeft(Process* process);
int getReadTime(Process* process);
int getServiceTime(Process* process);
int getMemSize(Process* process);
int getMemLoc(Process* process);

// Setters
void setMemLoc(Process* process, int loc);
