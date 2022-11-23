#include "oss.h"

static volatile sig_atomic_t proc_CmpltFlg = 0;

typedef struct {
  long mtype;
  pid_t pid;
  int msg;
}
userMsg;

static userMsg * msgSent;
int signal_Handle();
int ProcessControlBlock();
static void kill_Proc(int signo);
void detach_Sch();

int main(int argc, char * argv[]) {

  srand(time(NULL) + getpid());
  pid_t pid = getpid();
  int i;
  int res_Index;
  int res_Selection; /*  Random Selection of Resources By the Process*/
  int res_Activity;
  int res_Cnt = 0;

  signal_Handle();
  ProcessControlBlock();

  while (!proc_CmpltFlg) {
    res_Activity = rand() % 100;

    /*  The Process terminates without requesting res*/

    if (res_Activity < CONST_TERM) {
      proc_CmpltFlg = 1;

      /* Notify the OSS about the process termination */

      msgSent -> mtype = 1;
      msgSent -> pid = pid;
      msgSent -> msg = 0;
      msgsnd(ID_QUEUE, msgSent, len_Msg, 1);
      if (msgrcv(ID_QUEUE, msgSent, len_Msg, pid, 0) != -1) {
        alloc_Res -> util_Res[res_Selection] += 1;
      }

      /* The process is requesting resources*/

    } else if (res_Activity >= CONST_TERM && res_Activity < CONST_REQ) {
      res_Selection = rand() % 20;
      if ((alloc_Res -> util_Res[res_Selection]) < (max_Res -> util_Res[res_Selection]) && alloc_Res -> util_Res[res_Selection] < 1) {
        msgSent -> mtype = 3;
        msgSent -> pid = pid;
        msgSent -> msg = res_Selection;
        msgsnd(ID_QUEUE, msgSent, len_Msg, 3);
        if (msgrcv(ID_QUEUE, msgSent, len_Msg, pid, 0) != -1) {
          alloc_Res -> util_Res[res_Selection] += 1;
          res_Cnt += 1;
        }
      }
      /*  Release the resources*/

    } else {
      if (res_Cnt > 0) {
        res_Index = 0;
        res_Selection = rand() % res_Cnt;
        for (i = 0; i < 20; i++) {
          if (alloc_Res -> util_Res[i] > 0) {
            if (res_Index == res_Selection) {
              msgSent -> mtype = 2;
              msgSent -> pid = pid;
              msgSent -> msg = i;
              msgsnd(ID_QUEUE, msgSent, len_Msg, 2);
              if (msgrcv(ID_QUEUE, msgSent, len_Msg, pid, 0) != -1) {
                alloc_Res -> util_Res[i] -= 1;
                res_Cnt -= 1;
                i = 20;
              }
            } else {
              res_Index += 1;
            }
          }
        }
      }
    }

    if (clockShared -> seconds >= 1000) {
      proc_CmpltFlg = 1;
    }
  }

  printf("Process with PID %d is going to exit, the resources which were available to it were: [", pid);
  for (i = 0; i < 20; i++) {
    printf("%d,", alloc_Res -> util_Res[i]);
  }
  printf("]\n");
  exit(1);
  return 1;
}
int ProcessControlBlock() {

  /*  initiating shared mem clock*/
  shmclock = shmget(SHMCLOCKKEY, sizeof(clockStruct), 0666 | IPC_CREAT);
  clockShared = (clockStruct * ) shmat(shmclock, NULL, 0);

  /* initiating resources */

  ResourceSeg_Max = shmget(MAXRESOURCEKEY, (sizeof(res_Struct) + 1), 0666 | IPC_CREAT);
  max_Res = (res_Struct * ) shmat(ResourceSeg_Max, NULL, 0);
  if (ResourceSeg_Max == -1) {
    return -1;
  }

  /*  init allocated resources */

  alloc_Res = malloc(sizeof(res_Struct));
  int r;
  for (r = 0; r < 20; r++) {
    alloc_Res -> util_Res[r] = 0;
  }

  /* queues */

  ID_QUEUE = msgget(MSGQUEUEKEY, PERMS | IPC_CREAT);
  if (ID_QUEUE == -1) {
    return -1;
  }
  msgSent = malloc(sizeof(userMsg));
  len_Msg = sizeof(userMsg) - sizeof(long);
  return 0;
}

/* Shared Memory Deletion */

void detach_Sch() {
  shmdt(clock);
  shmdt(max_Res);
  msgctl(ID_QUEUE, IPC_RMID, NULL);
}

/*  Interrupt Handling */

int signal_Handle() {

  /* Interrupt handling for ctrl-C */

  struct sigaction CTRLC;
  CTRLC.sa_handler = kill_Proc;
  CTRLC.sa_flags = 0;

  if ((sigemptyset( & CTRLC.sa_mask) == -1) || (sigaction(SIGINT, & CTRLC, NULL) == -1)) {
    perror("Failed to set SIGINT to handle control-c");
    return -1;
  }

  /* Interrupt when the process terminates */

  struct sigaction sigParent;
  sigParent.sa_handler = kill_Proc;
  sigParent.sa_flags = 0;
  if ((sigemptyset( & sigParent.sa_mask) == -1) || (sigaction(SIGCHLD, & sigParent, NULL) == -1)) {
    perror("Failed to set SIGCHLD to handle signal from child process");
    return -1;
  }
  return 1;
}

static void kill_Proc(int signo) {
  proc_CmpltFlg = 1;
}

