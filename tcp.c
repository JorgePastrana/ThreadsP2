/************ PART 2 of Programming Project ***********/
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
PROC proc[NPROC];
PROC *freeList;
PROC *sleepList;
PROC *readyQueue;
PROC *running;
#include "queue.c"
/****** implement these functions ******/
int tsleep(int event);
int twakeup(int event);
int texit(int status);
int join(int pid, int *status);
void func(void *parm);
/****** end of implementations *********/

int init() { 
	int i, j; 
	PROC *p; 
	for (i=0; i<NPROC; i++){ 
		p = &proc[i]; 
		p->pid = i; 
		p->priority = 0; 
		p->status = FREE; 
		p->event = 0; 
		p->joinPid = 0; 
		p->joinPtr = 0; 
		p->next = p+1; 
	} 
	proc[NPROC-1].next = 0; 
	freeList = &proc[0]; // all PROCs in freeList 
	readyQueue = 0; 
	sleepList = 0; // create P0 as initial running task 
	running = p = dequeue(&freeList); 
	p->status = READY; 
	p->priority = 0; 
	printList("freeList", freeList); 
	printf("init complete: P0 running\n"); 
}

int do_create() { 
	int pid = create(func, running->pid); // parm = pid 
}

int do_switch() { 
	tswitch(); 
}

int do_exit()
{
// for simplicity: exit with pid value as status
texit(running->pid);
}
int do_join(int pid, int *status){
	join(pid, &status);
}


void task1(void *parm) // task1: demonstrate create-join operations
{
int pid[2];
int i, status;
//printf("task %d create subtasks\n", running->pid);
for (i=0; i<2; i++){ // P1 creates P2, P3
pid[i] = create(func, running->pid);
}
pid[0]=2;
pid[1]=3;
join(5, &status); // try to join with targetPid=5
for (i=0; i<2; i++){ // try to join with P2, P3
join(pid[i], &status);
}
}
void func(void *parm) // subtasks: enter q to exit
{
char c;
printf("task %d start: parm = %d\n", running->pid, parm);
while(1){
int mensaje;
int status;
printList("readyQueue", readyQueue);
printList("freeList", freeList);
printList("sleepList", sleepList);
printf("task %d running\n", running->pid);
printf("enter a key [c|s|q|j]: ");
c = getchar(); getchar(); // kill \r
switch (c){
case 'c' : do_create(); break;
case 's' : do_switch(); break;
case 'q' : do_exit(); break;
case 'j' : mensaje = do_join(1,&status); break;
}
}
}
int create(void (*f)(), void *parm)
{
int i;
PROC *p = dequeue(&freeList);
if (!p){
printf("create failed\n");
return -1;
}
p->status = READY;
p->priority = 1;
p->joinPid = 0;
p->joinPtr = 0;
for (i=1; i<13; i++)
p->stack[SSIZE-i] = 0;
p->stack[SSIZE-1] = (int)parm;
p->stack[SSIZE-2] = (int)do_exit;
p->stack[SSIZE-3] = (int)f;
p->ksp = &p->stack[SSIZE-12];
enqueue(&readyQueue, p);
printList("readyQueue", readyQueue);
printf("task%d created a new task%d\n", running->pid, p->pid);
return p->pid;
}

int main()
{
int i, pid, status;
printf("Welcome to the MT User-Threads System\n");
init();
create((void *)task1, 0);
printf("P0 switch to P1\n");
tswitch();
printf("All tasks ended: P0 loops\n");
while(1);
}

int tsleep(int event)
{
  running->event=event;
  running->status = SLEEP;
  enqueue(&sleepList,running);
  tswitch();
}
int twakeup(int event)
{
  PROC **LS = &sleepList;
  PROC *p = *LS;
  while(p)
     {
      PROC *q = p->next;
      printf("Depertar = %d\n", p->pid);
      if(p->event == event){
      	*LS=(*LS)->next;
	p->status=READY;
	enqueue(&readyQueue,p);
      }
      p=q;
     }
}

int join(int targetPID,int *status){
	while(1){
		int taskwithtargetpid=0;
		PROC *p,*h; 
		for (int i=0; i<NPROC; i++){ 
		p = &proc[i]; 
		 if(p->pid == targetPID && (p->status == ZOMBIE || p->status == READY || p->status == SLEEP)){
			taskwithtargetpid=1;
			h=p;
		}
		} 
		
		if(!taskwithtargetpid)
			{
			printf("NOTHREAD Error, no hay un hilo disponible con el PID: %d\n",targetPID);
			return NOTHREAD;//No hay proceso disponible con ese pid en readyqueue	
			}
		if(h->joinPid==running->pid){
			printf("DEADLOCK error\n");
			return DEADLOCK;//Deadlock mortal alv
		}
		running->joinPid = targetPID;
		running->joinPtr = h;
		if(h->status==ZOMBIE){
			*status = h->exitStatus;
			h->status=FREE;
			enqueue(&freeList,h);
			printf("Operación exitosa La tarea: %d se unio a la tarea: %d\n", running->pid, targetPID);
			return h->pid;		
		}
		printf("Operación exitosa La tarea: %d se unio a la tarea: %d\n", running->pid, targetPID);
		tsleep(targetPID);
	}

}

int texit(int value) { 
	printf("task %d in texit value=%d\n", running->pid, running->pid);
		
	PROC **LS = &sleepList;
  	PROC *p = *LS;
	int tasktojoin=0;
  	while(p)
     	{
      		PROC *q = p->next;
      		if(p->joinPid == running->pid){
      			tasktojoin=1;
      		}
      		p=q;
     	}
	if(!tasktojoin){
		running->status = FREE;
		running->priority = 0;
		enqueue(&freeList, running); 
		printList("freeList", freeList);
	}
	else{
	 	running->status=ZOMBIE;
		twakeup(running->pid);
	}
	running->exitStatus=value;
	tswitch(); 
}

int scheduler() { 
	if (running->status == READY) 
		enqueue(&readyQueue, running);
	running = dequeue(&readyQueue); 
	printf("next running = %d\n", running->pid); 
}
