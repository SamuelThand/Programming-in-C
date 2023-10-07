#include <stdio.h>

#define MAX_PROCESSES 100

int number_of_processes;
int total_waiting_time;
int total_turnaround_time;
typedef struct {
    int pid;
    int burst_time;
    int waiting_time;
    int turnaround_time;
} Process;

void loadInput(Process *processes);
void accumulateWaitingTime(Process *processes);
void accumulateTurnaroundTime(Process *processes);
void printData(Process *processes);

int main()
{
    Process processes[MAX_PROCESSES];
    loadInput(processes);
    accumulateWaitingTime(processes);
    accumulateTurnaroundTime(processes);
    printData(processes);
}

void loadInput(Process *processes)
{
    printf("Number of processes: ");
    scanf("%d", &number_of_processes);
    for (int i = 0; i < number_of_processes; i++) {
        processes[i].pid = i + 1;
        printf("Burst time of P%d: ", i + 1);
        scanf("%d", &processes[i].burst_time);
    }
}

void accumulateWaitingTime(Process *processes)
{
    for (int i = 0; i < number_of_processes; i++) {
        processes[i].waiting_time = 0;
        if (i == 0) continue;
        for (int j = 0; j < i; j++)
            processes[i].waiting_time += processes[j].burst_time;

        total_waiting_time += processes[i].waiting_time;
    }
}

void accumulateTurnaroundTime(Process *processes)
{
    for (int i = 0; i < number_of_processes; i++) {
        processes[i].turnaround_time = processes[i].burst_time + processes[i].waiting_time;
        total_turnaround_time += processes[i].turnaround_time;
    }
}

void printData(Process *processes)
{
    printf("P	 BT	 WT	 TAT\n");
    for (int i = 0; i < number_of_processes; i++)
        printf("P%d	 %d	 %d	 %d\n",
               processes[i].pid,
               processes[i].burst_time,
               processes[i].waiting_time,
               processes[i].turnaround_time);
    printf("Average Waiting Time = %f", (float) total_waiting_time / number_of_processes);
    printf("\nAverage Turnaround Time = %f", (float) total_turnaround_time / number_of_processes);
}
