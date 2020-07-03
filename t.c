#include <stdio.h> 
#include "type.h"
PROC proc[NPROC]; // NPROC proc structures 
PROC *freeList; // free PROC list 
PROC *readyQueue; // ready proc priority queue 
PROC *sleepList; // sleep PROC list 
PROC *running; // running proc pointer 
#include "queue.c" // enqueue(), dequeue(), printList()
int create(void (*f)(),void *param);
void func(void *param);
int ksleep(int event);
int kawake(int event);

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
		kawake(running->pid);		
	}
	running->exitStatus=value; 	 
	tswitch(); 
}

int do_create() { 
	int pid = create(func, running->pid); // parm = pid 
}

int do_switch() { 
	tswitch(); 
}

int do_exit() { 
	texit(running->pid); // for simplicity: exit with pid value 
}

void func(void *parm) { 
	int c, mensaje;
	int status; 
	printf("task %d start: parm = %d\n", running->pid, (int)parm); 
	while(1){ 
		printf("task %d running status: %d\n", running->pid, running->status); 
		printList("readyQueue", readyQueue);
		printList("sleepList", sleepList); 
		printf("enter a key [c|s|q|l|w|j] : "); 
		c = getchar(); 
		getchar(); 
		switch (c){ 
			case 'c' : 
				do_create(); 
			break; 
			case 's' : 
				do_switch(); 
			break; 
			case 'q' : 
				do_exit(); 
			break; 
			case 'l' : 
				ksleep(0); 
			break; 
			case 'w' : 
				kawake(0); 
			break;
			case 'j' : 
				mensaje = join(2,&status); 
				switch(mensaje){
					case NOTHREAD:
						printf("Joel es puto\n");
					break;
					case DEADLOCK:
						printf("Ada es puto\n");
					break;
					default:
						printf("Operación exitosa\n");
					break;
				}

			break; 
		} 
	} 
}

int create(void (*f)(), void *parm) { 
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
// initialize new task stack for it to resume to f(parm) 
	for (i=1; i<13; i++) // zero out stack cells 
		p->stack[SSIZE-i] = 0; 
	p->stack[SSIZE-1] = (int)parm; // function parameter 
	p->stack[SSIZE-2] = (int)do_exit; // function return address 
	p->stack[SSIZE-3] = (int)f; // function entry 
	p->ksp = (int)&p->stack[SSIZE-12]; // ksp -> stack top 
	enqueue(&readyQueue, p); 
	printList("readyQueue", readyQueue); 
	printf("task %d created a new task %d\n", running->pid, p->pid); 
	return p->pid;
}

int main() { 
	printf("Welcome to the MT User-Level Threads System\n"); 
	init(); 
	create((void *)func, 0); 
	printf("P0 switch to P1\n"); 
	while(1){ 
		if (readyQueue) 
			tswitch(); 
	} 
}

int scheduler() { 
	if (running->status == READY) 
		enqueue(&readyQueue, running); 
	running = dequeue(&readyQueue); 
	printf("next running = %d\n", running->pid); 
}

int ksleep(int event)
{
  running->event=event;
  running->status = SLEEP;
  enqueue(&sleepList,running);
  tswitch();
}
int kawake(int event)
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
		PROC **LS = &readyQueue;
  		PROC *w = *LS;
		PROC *h;
		int taskwithtargetpid=0;
  		while(w)
     		{
      			PROC *q = w->next;
      			if(w->pid == targetPID){
      				taskwithtargetpid=1;
				h=w;
      			}
      			w=q;
     		}
		if(!taskwithtargetpid)
			return NOTHREAD;//No hay proceso disponible con ese pid en readyqueue	
		if(h->joinPid==running->pid){
			return DEADLOCK;//Deadlock mortal alv
		}
		running->joinPid = targetPID;
		running->joinPtr = h;
		if(h->status==ZOMBIE){
			*status = h->exitStatus;
			h->status=FREE;
			enqueue(&freeList,h);
			return h->pid;		
		}
		ksleep(targetPID);
	}

}		
