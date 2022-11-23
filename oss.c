#include "oss.h"

typedef struct {
  long mtype;
  pid_t pid;
  int res;
}
userMsg;

Queue * blockedQ1;
Queue * blockedQ2;
Queue * procList1;
Queue * procList2;

static volatile sig_atomic_t doneFlag = 0;
static userMsg * msgSent;
int reqGrant;
int numDeadLock;

int main(int argc, char * argv[]) {

  FILE * logFile;
  logFile = fopen("logFile", "w");

  srand(time(NULL) + getpid());

  Queue * tempPtr;
  Queue * prevTempPtr;

  tempPtr = NULL;
  prevTempPtr = NULL;

  blockedQ1 = NULL;
  blockedQ2 = NULL;

  procList1 = NULL;
  procList2 = NULL;

int childPid;
  int timeElapsed = 2;
  int sharedRes;
  int verbose = 1; /* 1: verbose on */
  int procLim = 18;
  int totProc = 0;
  int log_Len = 0;

  num_Child = 0;
  reqGrant = 0;
  numDeadLock = 0;

  signal_Handle();
  ProcessControlBlock();

  alarm(timeElapsed);

  /*  init message struct  */
  msgSent = malloc(sizeof(userMsg));
  len_Msg = sizeof(userMsg) - sizeof(long);

  /* init allocated resources */
  alloc_Res = malloc(sizeof(res_Struct));
  avail_Res = malloc(sizeof(res_Struct));

  /*  init both res structs' res elements */
  int i;
  for (i = 0; i < 20; i++) {
    max_Res -> util_Res[i] = (rand() % 19) + 1;
    avail_Res -> util_Res[i] = max_Res -> util_Res[i];
    alloc_Res -> util_Res[i] = 0;

  }

  ForkTime();

  /*  Creation of Process */

  while (!doneFlag) {
    if (num_Child < procLim && chk_FortTime() == 1) {

      if ((childPid = fork()) == 0) {
        execlp("./user", "./user", (char * ) NULL);

        fprintf(stderr, "%sFailed to fork child process!\n", argv[0]);
        _exit(1);
      }

      if (procList1 == NULL) {
        procList1 = createNewProcess(childPid);
        procList2 = procList1;
      } else {
        procList2 = procList2 -> next = createNewProcess(childPid);
      }

      totProc += 1;
      num_Child += 1;
      printf("OSS: Process with PID %d is created at time %d:%d\n", childPid, clockShared -> seconds, clockShared -> nanosecs);
      ForkTime();
    }

    /*  When process terminates release the resourcse */

    if (msgrcv(ID_QUEUE, msgSent, len_Msg, 1, IPC_NOWAIT) != -1) {
      removeFromQ(msgSent -> pid, procList1);

      msgSent -> mtype = msgSent -> pid;
      msgsnd(ID_QUEUE, msgSent, len_Msg, 0);
    }

    /* Process is requesting res */

    if (msgrcv(ID_QUEUE, msgSent, len_Msg, 3, IPC_NOWAIT) != -1) {
      if (verbose == 1 && log_Len <= 100000) {
        fprintf(logFile, "Master has detected Process P%d requesting R%d at time %d:%d\n", msgSent -> pid, msgSent -> res, clockShared -> seconds, clockShared -> nanosecs);
        log_Len += 1;
      }

      /*  Banker's Algorithm to check the possible  deadlock condition.*/

      numDeadLock += 1;
      if (bankersAlgorithm(msgSent -> res, search_PCB(msgSent -> pid, procList1)) == 1) {
        if (verbose == 1 && log_Len <= 100000) {
          fprintf(logFile, "Master granting P%d request R%d at time %d:%d\n", msgSent -> pid, msgSent -> res, clockShared -> seconds, clockShared -> nanosecs);
          if (numDeadLock % 20 == 0) {
            int o;
            int n = 0;
            tempPtr = procList1;
            fprintf(logFile, "     ");
            for (o = 0; o < 20; o++) {

              fprintf(logFile, "Master running deadlock detection at time %d:%d\n", clockShared -> seconds, clockShared -> nanosecs);
            }
            log_Len += 1;
            fprintf(logFile, "\n");
            log_Len += 1;
            fprintf(logFile, "----------------Released Resources:------------------------\n");
            fprintf(logFile, "\n");
            while (tempPtr != NULL) {
              fprintf(logFile, "P%d", n);
              for (o = 0; o < 20; o++) {

                fprintf(logFile, " %d", tempPtr -> H1 -> resUsed -> util_Res[o]);

              }
              fprintf(logFile, "\n\n\n\n");
              tempPtr = tempPtr -> next;
              n += 1;
              log_Len += 1;
            }
          }
          log_Len += 1;
        }
        /*  Whether the res is granted to the process or it is deadlocked */

        reqGrant += 1;
        search_PCB(msgSent -> pid, procList1) -> resUsed -> util_Res[msgSent -> res] += 1;
        msgSent -> mtype = msgSent -> pid;
        msgsnd(ID_QUEUE, msgSent, len_Msg, 0);
      } else {

        log_Len += 1;

        /*  If the res is not available the process is put to sleep in queue */

        if (blockedQ1 == NULL) {
          blockedQ1 = newBlockedProc(search_PCB(msgSent -> pid, procList1));
          blockedQ2 = blockedQ1;
        } else {
          blockedQ2 = blockedQ2 -> next = newBlockedProc(search_PCB(msgSent -> pid, procList1));
        }
        blockedQ2 -> H1 -> resRequested = msgSent -> res;
      }
    }

    /*  Process is releasing res*/

    if (msgrcv(ID_QUEUE, msgSent, len_Msg, 2, IPC_NOWAIT) != -1) {
      if (verbose == 1 && log_Len <= 100000) {
        fprintf(logFile, "Master has acknowledged Process P%d releasing R%d at time %d:%d", msgSent -> pid, msgSent -> res, clockShared -> seconds, clockShared -> nanosecs);
        log_Len += 1;
      }
      alloc_Res -> util_Res[msgSent -> res] -= 1;
      avail_Res -> util_Res[msgSent -> res] += 1;
      search_PCB(msgSent -> pid, procList1) -> resUsed -> util_Res[msgSent -> res] -= 1;

      msgSent -> mtype = msgSent -> pid;
      msgsnd(ID_QUEUE, msgSent, len_Msg, 0);
    }

    /*  If the resources get available, wake up the process (remove from blocked queue) */

    tempPtr = blockedQ1;
    prevTempPtr = blockedQ1;
    while (tempPtr != NULL) {
      sharedRes = tempPtr -> H1 -> resRequested;
      if ((alloc_Res -> util_Res[sharedRes]) < (max_Res -> util_Res[sharedRes])) {
        if (tempPtr == prevTempPtr) {
          blockedQ1 = tempPtr -> next;
        } else {
          if (tempPtr -> next == NULL) {
            blockedQ2 = prevTempPtr;
          }
          prevTempPtr -> next = tempPtr -> next;
        }
        log_Len += 1;
        alloc_Res -> util_Res[sharedRes] += 1;
        avail_Res -> util_Res[sharedRes] -= 1;
        search_PCB(tempPtr -> H1 -> pid, procList1) -> resUsed -> util_Res[sharedRes] += 1;
        msgSent -> mtype = tempPtr -> H1 -> pid;
        msgsnd(ID_QUEUE, msgSent, len_Msg, 0);
        tempPtr = NULL;
        prevTempPtr = NULL;
      } else {
        if (prevTempPtr != tempPtr) {
          prevTempPtr = tempPtr;
        }
        tempPtr = tempPtr -> next;
        if (tempPtr != NULL) {}
      }
    }

    clockShared -> nanosecs += 1000000;
    if (clockShared -> nanosecs >= 1000000000) {
      clockShared -> seconds += 1;
      clockShared -> nanosecs = clockShared -> nanosecs % 1000000000;
    }
    if (clockShared -> seconds >= 1000) {
      clockShared -> nanosecs = 0;
      doneFlag = 1;
    }
    if (totProc >= 99) {
      doneFlag = 1;
    }
  }

  printf("\n");
  printf("----------------------Total number of process in the system: %d----------------------\n", totProc);
  printf("\nAvailable Resources to grant to the processes: \n[");
  int n;
  for (n = 0; n < 20; n++) {
    printf("%d,", avail_Res -> util_Res[n]);
  }
  printf("]\nNumber of Allocated Resources to the Processes: \n[");
  for (n = 0; n < 20; n++) {
    printf("%d,", alloc_Res -> util_Res[n]);
  }
  printf("]\n Total number of Resources in the system: \n[");
  for (n = 0; n < 20; n++) {
    printf("%d,", max_Res -> util_Res[n]);
  }
  printf("]\n\n\n\n");
  printf("----------------------Requests that have been granted: %d----------------------\n", reqGrant);
  printf("----------------------Number of times deadlocked detection is run : %d ----------------------\n", numDeadLock);
  fprintf(logFile, "\n\n");
  fprintf(logFile, "----------------------Number of times deadlocked detection is run : %d ----------------------\n", numDeadLock);
  printf("\n");
  printf("----------------------Percentage of Deadlock detection and the processes that are terminated sucessfully: %f----------------------\n", (float) reqGrant / (float) numDeadLock * 100);
  printf("\n");
  fprintf(logFile, "----------------------Percentage of Deadlock detection and the processes that are terminated sucessfully: %f----------------------\n", (float) reqGrant / (float) numDeadLock * 100);
  fprintf(logFile, "----------------------Requests that have been granted: %d----------------------\n", reqGrant);

  detach_Sch();
  return 0;
  fclose(logFile);
}

/*  Signal Handling  */

int signal_Handle() {

  /* Setting up Signal Handler to set up the alarm after some time limit */

  struct sigaction alarm_Time;
  alarm_Time.sa_handler = killAllProc;
  alarm_Time.sa_flags = 0;
  if ((sigemptyset( & alarm_Time.sa_mask) == -1) || (sigaction(SIGALRM, & alarm_Time, NULL) == -1)) {
    perror("Failed to set SIGALRM to handle timer alarm");
    return -1;
  }

  /* Setting up a handler for SIGINT */

  struct sigaction CTRLC;
  CTRLC.sa_handler = killAllProc;
  CTRLC.sa_flags = 0;
  if ((sigemptyset( & CTRLC.sa_mask) == -1) || (sigaction(SIGINT, & CTRLC, NULL) == -1)) {
    perror("Failed to set SIGINT to handle control-c");
    return -1;
  }

  /*  Signal Handler when the Process terminates  */

  struct sigaction finished_Worker;
  finished_Worker.sa_handler = chk_ChildFinished;
  finished_Worker.sa_flags = 0;
  if ((sigemptyset( & finished_Worker.sa_mask) == -1) || (sigaction(SIGCHLD, & finished_Worker, NULL) == -1)) {
    perror("Failed to set SIGCHLD to handle signal from child process");
    return -1;
  }
  return 1;
}

/*  Killing All the Processes if the system is in DEADLOCK */

static void killAllProc(int signo) {
  doneFlag = 1;
  if (signo == SIGALRM) {
    printf("]\n\n\nKILLING ALL PROCESSES!!!!!\n\n\n");
    killpg(getpgid(getpid()), SIGINT);
  }
}

/*  Functionnn to check when the child is finished */

static void chk_ChildFinished(int signo) {
  pid_t finished_Pid;
  while ((finished_Pid = waitpid(-1, NULL, WNOHANG))) {
    if ((finished_Pid == -1) && (errno != EINTR)) {
      break;
    } else {
      printf("Process with PID %d finished!\n", finished_Pid);
      num_Child -= 1;
    }
  }
}

/*  Shared Memory Clock */

int ProcessControlBlock() {
  shmclock = shmget(SHMCLOCKKEY, sizeof(clockStruct), 0666 | IPC_CREAT);
  clockShared = (clockStruct * ) shmat(shmclock, NULL, 0);
  if (shmclock == -1) {
    return -1;
  }
  clockShared -> seconds = 0;
  clockShared -> nanosecs = 0;

  /*  Initiating the time when to fork  the processes */

  forkTime = malloc(sizeof(clockStruct));

  /* res Initiation  */

  ResourceSeg_Max = shmget(MAXRESOURCEKEY, (sizeof(res_Struct) + 1), 0666 | IPC_CREAT);
  max_Res = (res_Struct * ) shmat(ResourceSeg_Max, NULL, 0);
  if (ResourceSeg_Max == -1) {
    return -1;
  }

  /* Message queues id */

  ID_QUEUE = msgget(MSGQUEUEKEY, PERMS | IPC_CREAT);
  if (ID_QUEUE == -1) {
    return -1;
  }

  return 0;
}

/*  Deallocating Shared Mem */

void detach_Sch() {
  shmdt(clockShared);
  shmctl(shmclock, IPC_RMID, NULL);
  shmdt(max_Res);
  shmctl(ResourceSeg_Max, IPC_RMID, NULL);
  msgctl(ID_QUEUE, IPC_RMID, NULL);
}

/*  Queues for the Processes */

Queue * createNewProcess(int pid) {
  Queue * Qnew;
  Qnew = malloc(sizeof(Queue));
  Qnew -> next = NULL;
  Qnew -> H1 = malloc(sizeof(PCB));
  Qnew -> H1 = PCB_New(pid);

  return Qnew;
}
Queue * newBlockedProc(PCB * pcb) {
  Queue * Qnew;
  Qnew = malloc(sizeof(Queue));
  Qnew -> next = NULL;
  Qnew -> H1 = malloc(sizeof(PCB));
  Qnew -> H1 = pcb;

  return Qnew;
}

/*  When the proceses are terminated release them from the queues or Process list */

void removeFromQ(int pidToDelete, Queue * ptr) {
  /* case of first element in queue */
  if (ptr -> H1 -> pid == pidToDelete) {
    release_Resource(ptr -> H1 -> resUsed);
    procList1 = ptr -> next;
    return;
  } else {
    while (ptr != NULL) {
      if (ptr -> next -> H1 -> pid == pidToDelete) {
        release_Resource(ptr -> next -> H1 -> resUsed);
        ptr -> next = ptr -> next -> next;
        if (ptr -> next == NULL) {
          procList2 = ptr;
        }
        return;
      } else {
        ptr = ptr -> next;
      }
    }
  }
}

/*  Process Control block to keep a track of the information for all the blocked and newly created process */

PCB * PCB_New(int pid) {
  PCB * newP;
  newP = malloc(sizeof(PCB));
  newP -> pid = pid;
  newP -> resRequested = 0;
  newP -> resUsed = malloc(sizeof(res_Struct));
  int n;
  for (n = 0; n < 20; n++) {
    newP -> resUsed -> util_Res[n] = 0;
  }
  return newP;
}

PCB * search_PCB(int pid, Queue * ptrHead) {
  while (ptrHead != NULL) {
    if (ptrHead -> H1 -> pid == pid) {
      return ptrHead -> H1;
    } else {
      ptrHead = ptrHead -> next;
    }
  }
  return NULL;
}

/*  Check to see if it is a time to fork off new process in the system */

int chk_FortTime() {
  if ((clockShared -> nanosecs >= forkTime -> nanosecs) && (clockShared -> seconds >= forkTime -> seconds)) {
    return 1;
  } else {
    if (clockShared -> seconds < 2 && clockShared -> nanosecs % 100000000 == 0) {}
    return 0;
  }
}

/*  Clock timing to fork new process in system */

void ForkTime() {
  randForkTime = (rand() % 500) * 1000000;
  forkTime -> nanosecs = clockShared -> nanosecs + randForkTime;
  forkTime -> seconds = clockShared -> seconds;
  if (forkTime -> nanosecs >= 1000000000) {
    forkTime -> seconds += 1;
    forkTime -> nanosecs = forkTime -> nanosecs % 1000000000;
  }
}

/*  Deadlock Avoidance */

int deadlockAvoidance(int res) {
  if ((alloc_Res -> util_Res[res]) < (max_Res -> util_Res[res])) {
    alloc_Res -> util_Res[res] += 1;
    avail_Res -> util_Res[res] -= 1;
    return 1;
  } else {
    return 0;
  }
}

/*  To check if there are sufficifient available resources for the process and they are not getting DEADLOCKED. */

int bankersAlgorithm(int res, PCB * Process) {
  int r;
  int s;

  if (avail_Res -> util_Res[res] > 1) {
    alloc_Res -> util_Res[res] += 1;
    avail_Res -> util_Res[res] -= 1;
    return 1;
  } else if (avail_Res -> util_Res[res] == 0) {
    return 0;
  } else {
    for (r = 0; r < 20; r++) {
      s = r;
      if (r == res) {
        s = res + 1;
      }
      if (avail_Res -> util_Res[s] + Process -> resUsed -> util_Res[s] < 1) {
        return 0;
      }
    }
    alloc_Res -> util_Res[res] += 1;
    avail_Res -> util_Res[res] -= 1;
    return 1;
  }
}

/*  Resources Released */

void release_Resource(res_Struct * res) {
  int r;
  for (r = 0; r < 20; r++) {
    if (res -> util_Res[r] > 0) {
      avail_Res -> util_Res[r] += res -> util_Res[r];
      alloc_Res -> util_Res[r] -= res -> util_Res[r];
    }
  }
}

