#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* CITS2002 Project 1 2019
   Name(s):             student-name1 (, student-name2)
   Student number(s):   student-number-1 (, student-number-2)
 */


//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20
#define MAX_PROCESSES           50
#define MAX_EVENTS_PER_PROCESS  100
#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5
#define MAX_INTEGER_LENGTH      9       //THE MAXIMUM LENGTH OF AN INT IS 9 DIGITS

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum                = 0;
int total_process_completion_time       = 0;

//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20
char device_names[MAX_DEVICES][MAX_DEVICE_NAME];
int device_speeds[MAX_INTEGER_LENGTH];
int process_start_finish_times[MAX_PROCESSES][2];
int process_io_times[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int io_transfer_sizes[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
char io_devices[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS][MAX_DEVICE_NAME];
int device_count = 0;
int process_count = 0;

void parse_tracefile(char program[], char tracefile[])
{
    //  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(tracefile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int  lc     = 0;
    int io_count = 0;

    //  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

        //  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

        //  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char    word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);
        //printf("%i = %s", nwords, line);


        //  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }
        
        //  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
        if(nwords == 4 && strcmp(word0, "device") == 0) {
            // FOUND A DEVICE DEFINITION, WE'LL NEED TO STORE THIS SOMEWHERE
            strcpy(device_names[device_count], word1);
            device_speeds[device_count] = atoi(word2);
            printf("%s %d \n", device_names[device_count], device_speeds[device_count]);
            device_count++;
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            ;   // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }

        else if(nwords == 4 && strcmp(word0, "process") == 0) {
            ;   // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            process_start_finish_times[process_count][0] = atoi(word2);
        }

        else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
            ;   //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
            process_io_times[process_count][io_count] = atoi(word1);
            strcpy(io_devices[process_count][io_count], word2);
            io_transfer_sizes[process_count][io_count] = atoi(word3);
            printf("%d %s %d \n", process_io_times[process_count][io_count], io_devices[process_count][io_count], io_transfer_sizes[process_count][io_count]);
            io_count++;
        }

        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            ;   //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            process_start_finish_times[process_count][1] = atoi(word1);
            //printf("%d %d \n", process_start_finish_times[process_count][0], process_start_finish_times[process_count][1]);
        }

        else if(nwords == 1 && strcmp(word0, "}") == 0) {
            ;   //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
            process_count++;
            io_count = 0;
        }
        else {
            printf("%s: line %i of '%s' is unrecognized",
                        program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

#undef  MAXWORD
#undef  CHAR_COMMENT

//  ----------------------------------------------------------------------

//  Helper function to recollect space in a single-ended block representation of a queue

void fix_queue(int *arr) {
    int current_item = 0;
    while(current_item < (50-1) && (arr[current_item] != 0 || arr[current_item+1] != 0)) {
        arr[current_item] = arr[current_item+1];
        current_item++;
    }
}

//  ----------------------------------------------------------------------

//  Helper function to sort devices in order of their transfer speeds

void sort_devices() {

}


//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    int completed_processes = 0;
    int queue[MAX_DEVICES+1][MAX_PROCESSES] = {0};
    int queue_count[MAX_DEVICES+1] = {0};
    int current_CPU_time = 0;
    int data_transfer_time = 0;
    int process_running_times[MAX_PROCESSES] = {0};
    int global_time = 0;
    int current_running_process = 0;
    int current_databus_process = 0;
    int CPU_wait_time = TIME_CONTEXT_SWITCH;
    bool finished = false;
    bool CPU_running = false;
    bool databus_running = false;

    printf("running simulate_job_mix( time_quantum = %i usecs )\n",
                time_quantum);   
    while(!finished) {
        // Check to see if any processes should start up -- push onto queue if yes
        for(int process_no = 0; process_no < process_count; process_no++) { //Iterate through all processes
            if(process_start_finish_times[process_no][0] == global_time) { //A process has began execution
                queue[0][queue_count[0]] = process_no+1;
                queue_count[0]++;
            }
        }

        // Check to see if databus is finished being used -- push process back onto queue if yes, check databus queue
        if(data_transfer_time == 0 && databus_running) {
            queue[0][queue_count[0]] = current_databus_process+1;
            queue_count[0]++;
            for(int device_no = 1; device_no < device_count+1; device_no++) {
                if(queue[device_no][0] != 0) {
                    current_databus_process = queue[device_no][0]-1;
                    queue[device_no][0] = 0;
                    fix_queue(queue[device_no]);

                }
            }
            databus_running = false;
        }


        // Check if running process has finished or has an i/o request
        if(CPU_running) {
            if(process_running_times[current_running_process] == process_start_finish_times[current_running_process][1]) {
                completed_processes++;
                CPU_running = false;
                CPU_wait_time = TIME_CONTEXT_SWITCH;
                current_CPU_time = 0;
            }
            else {
                for(int process_event = 0; process_event < MAX_EVENTS_PER_PROCESS; process_event++) {
                    if(process_io_times[current_running_process][process_event] == 0) {
                        break;
                    }
                    else if(process_running_times[current_running_process] == process_io_times[current_running_process][process_event]) {
                        char requested_device[MAX_DEVICE_NAME];
                        strcpy(requested_device, io_devices[current_running_process][process_event]);
                        for(int requested_device_no = 0; requested_device_no < MAX_DEVICES; requested_device_no++){
                            if(strcmp(requested_device, device_names[requested_device_no]) == 0) {
                                printf("%d \n", requested_device_no);
                                queue[requested_device_no+1][queue_count[requested_device_no+1]] = current_running_process;
                                queue_count[requested_device_no+1]++;
                                CPU_running = false;
                                CPU_wait_time = TIME_CONTEXT_SWITCH;
                                current_CPU_time = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // If TQ is expired, check ready queue, if its empty, refresh TQ otherwise push back onto queue
        if(current_CPU_time == time_quantum) {
            if(queue_count[0] == 0) {
                current_CPU_time = 0;
            }
            else {
                current_CPU_time = 0;
                queue[0][queue_count[0]] = current_running_process+1;
                queue_count[0]++;
                CPU_running = false;
                CPU_wait_time = TIME_CONTEXT_SWITCH;
            }
        }
        

        // Check if the CPU can run a process if it is not currently running one
        if(!CPU_running && queue[0][0] != 0) {
            if(CPU_wait_time > 0) {
                CPU_wait_time--;
            }
            else {
                current_running_process = queue[0][0]-1;
                queue[0][0] = 0;
                fix_queue(queue[0]);
                queue_count[0]--;
                CPU_running = true;
            }
        }      

        // We're finished if all processes have completed, otherwise increment global time and simulate 1 more microsecond
        if(completed_processes == process_count) {
            finished = true;
        }
        else {
            global_time++;
            if(CPU_running) {
                current_CPU_time++;
                process_running_times[current_running_process]++;

            }
            if(databus_running) {
                data_transfer_time--;
            }
        }
        printf("this is the global time: %d \n", global_time);
    }
    int current_total_process_completion_time = global_time - process_start_finish_times[0][0];
    if(total_process_completion_time == 0 || current_total_process_completion_time <= total_process_completion_time) {
        optimal_time_quantum = time_quantum;
        total_process_completion_time = current_total_process_completion_time;
    }

}

//  ----------------------------------------------------------------------

void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

    //  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES
    if(argcount == 5) {
        TQ0     = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc   = atoi(argvalue[4]);

        if(TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
            usage(argvalue[0]);
        }
    }
    //  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if(argcount == 3) {
        TQ0     = atoi(argvalue[2]);
        if(TQ0 < 1) {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc   = 1;
    }
    //  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else {
        usage(argvalue[0]);
    }

    //  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);

    //  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
    //  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
    //  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED


    for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
    }

    //  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);
    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4