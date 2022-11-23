#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/msg.h>
#include <getopt.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define SHMCLOCKKEY	86868            /* Sem clock.*/
#define MSGQUEUEKEY	68686            /*  Key for Msgqueue */
#define MAXRESOURCEKEY	71657            /* Key to check on resources.*/

#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

#define CONST_TERM 2 				/*  Percent chance that a child process will terminate instead of requesting/releasing a res */ 
#define CONST_REQ 50			/*  Percent chance that a child process will request a new resource */ 

typedef struct node {
    struct node *next;
    struct pcb *H1; 
} Queue;

typedef struct {
	int seconds;
	int nanosecs;
} clockStruct;

typedef struct {
	int util_Res[20];
} res_Struct;

typedef struct pcb {
	int pid;
	int resRequested;
	int totalBlockedTime;
	int blockedBurstSecond;
	int blockedBurstNano;
	res_Struct *resUsed;
} PCB;

static int ID_QUEUE;
static clockStruct *clockShared;
static clockStruct *forkTime;
static res_Struct *max_Res;
static res_Struct *alloc_Res;
static res_Struct *avail_Res;

int randForkTime;
int ResourceSeg_Max;
int num_Child;
int shmclock;
int len_Msg;

int signal_Handle();
static void killAllProc(int signo);
static void chk_ChildFinished(int signo);

int ProcessControlBlock();
void detach_Sch();

Queue *createNewProcess(int pid);
Queue *newBlockedProc(PCB *pcb);
void removeFromQ(int pidToDelete, Queue *ptr);
void printQueue(Queue * ptr);
PCB *PCB_New(int pid);
PCB *search_PCB(int pid, Queue * ptrHead);

int chk_FortTime();
void ForkTime();
int deadlockAvoidance(int res);
int bankersAlgorithm(int res, PCB * Process);
void release_Resource(res_Struct * res);

