#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <string.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>

typedef struct TASK
{
	char name[5];
	int  R , T;
	pid_t pid;
}task;
typedef struct QUEUE
{
	int id;
	int exec_time;
	struct QUEUE *next;
}queue;


FILE *open_file(char const *name);
int *get_share_var(char *filename);	//create a new shared variable with size = int (IPC)
void sort_readtime(int n, task task_set[]);	//sort task base on ready time
void set_priority(pid_t pid , int priority);
void do_nothing(int time);	//idle
void print_queue(queue *head );	//for debug purpose
queue *insert_queue(queue *head , int num);	//insert new task
queue *insert_queue_SJF(queue *head , int num , int exec_time);	//insert new task with para exec_time
queue *delete_queue(queue *head , int num);	//delete new task
queue *shortest_job(queue *head);	//select shortest task
void renew(queue *head ,int id ,int *runtime);	//renew the remain exc_time after context switch


pid_t do_task_FIFO(task task_set , int *timer , int *nextjob , int i , int *end){
	char start_time_buf[200];
	pid_t ppid = getpid();
	pid_t pid = fork();
	
	if(pid == 0){
		syscall(334 , start_time_buf);
		//initial
		volatile unsigned long j;
		for(int i = 0 ; i < task_set.T ; i++){
			for(j = 0 ; j < 1000000UL ; j++);
			*timer += 1;
		}
		//process
		*end = i;	
		//end status
		syscall(335 , getpid() , start_time_buf);
		fprintf(stdout,"%s %d\n",task_set.name , getpid());
		exit(0);
		//finish
	}else{
		set_priority(pid , 1);	
	}
	return pid;
}
pid_t do_task_RR(task task_set , int *timer , int *nextjob , int i , int *end){
	char start_time_buf[200];
	pid_t ppid = getpid();
	pid_t pid = fork();
	
	if(pid == 0){
		
		syscall(334 , start_time_buf);
		
		//initial
		volatile unsigned long j;
		for(int i = 0 ; i < task_set.T ; i++){
			for(j = 0 ; j < 1000000UL ; j++);
			*timer += 1;
			if( i != 0 && i % 500 == 0 && i != task_set.T - 1){
				set_priority(getpid() , 1);
			}//context switch
		}
		//process
		*end = i;	
		//end status
		syscall(335 , getpid() , start_time_buf);
		printf("%s %d\n",task_set.name , getpid());
		exit(0);
		//finish
	}
	else{
		set_priority(pid , 1);	
	}
	return pid;
}
pid_t do_task_PSJF(task task_set , int *timer , int *nextjob , int i , int *end , int *runtime){
	char start_time_buf[200];
	pid_t ppid = getpid();
	pid_t pid = fork();
	
	if(pid == 0){
		syscall(334 , start_time_buf);
		//initial
		volatile unsigned long j;
		for(int i = 0 ; i < task_set.T ; i++){
			for(j = 0 ; j < 1000000UL ; j++);
			*timer += 1;
			*runtime += 1;
			if( *timer == *nextjob && i != task_set.T - 1){
				set_priority(getpid() , 1);
			}
		}
		//process
		*end = i;	
		//end status
		syscall(335 , getpid() , start_time_buf);
		printf("%s %d\n",task_set.name , getpid());
		exit(0);
		//finish
	}else{
		set_priority(pid , 1);	
	}
	return pid;
}

int main(int argc, char const *argv[])
{
  	char mode[10];
  	int input_size , ppid = getpid();
  	scanf("%s%d" , mode , &input_size);
  	task task_set[input_size];
  	for(int i = 0 ; i < input_size ; i++){
  		scanf("%s%d%d" , task_set[i].name, &(task_set[i].R) , &(task_set[i].T));
  	}
	
  	set_priority(getpid() , 10);
	sort_readtime(input_size , task_set);
  	
  	if(strcmp(mode , "FIFO") == 0){	
		int *timer = get_share_var("timer") , *nextjob = get_share_var("nextjob");
  		int *end = get_share_var("end") , i = 0 , end_record = -1 , count = 0;
  		queue *head = NULL;
  		*timer = 0;
  		*nextjob = task_set[0].R;
  		*end = -1;
  		set_priority(getpid() , 50);

  		while(1){
  			while(*nextjob > *timer && head == NULL){
  				do_nothing(1);
  				*timer += 1;
  			}//idle
  			if(*end != end_record){		//new ending task
  				end_record = *end;
  				head = delete_queue(head , end_record);
  				count += 1;
  			}
  			if(count == input_size)	break;	//	finish every task
  			//clear ending task
  			while(i < input_size && task_set[i].R <= *timer){
				head = insert_queue_SJF(head , i , task_set[i].T);
  				task_set[i].pid = do_task_FIFO(task_set[i] , timer , nextjob , i , end);
  				if(i + 1 < input_size){
  					*nextjob = task_set[i + 1].R;
				}
				i += 1; 
  			}//create task
  			if(head == NULL)	continue;
  			else{
  				queue *selected = head;
  				int next = selected->id;
  				set_priority(task_set[next].pid , 99);
				sched_yield();
  			}//select next runnig task

  		}	
  		for(int i = 0 ; i < input_size ; i++){
  			wait(NULL);
  		}
  	}
  	else if(strcmp(mode , "RR") == 0){
  		int *timer = get_share_var("timer") , *nextjob = get_share_var("nextjob");
  		int *end = get_share_var("end") , i = 0 , end_record = -1 , count = 0;
  		queue *head = NULL;
  		*timer = 0;
  		*nextjob = task_set[0].R;
  		*end = -1;
  		set_priority(getpid() , 50);
		int flag = 1;
	
  		while(1){
  			while(*nextjob > *timer && head == NULL){
  				do_nothing(1);
  				*timer += 1;
  			}//idle
  			if(*end != end_record){		//new ending task
  				end_record = *end;
  				delete_queue(head , end_record);
  				count += 1;
  			}
  			if(count == input_size)	break;	//	finish every task
  			//clear ending task
  			while(i < input_size && task_set[i].R <= *timer){
				head = insert_queue(head , i);
  				task_set[i].pid = do_task_RR(task_set[i] , timer , nextjob , i , end);
  				if(i + 1 < input_size){
  					*nextjob = task_set[i + 1].R;
				}
				i += 1; 
  			}//create task
  			if(head == NULL){
				flag = 1;				
				continue;
			}
  			else{
				if(flag != 1)
  					head = head->next;
				flag = 0;
  				int next = head->id;
  				set_priority(task_set[next].pid , 99);
  				sched_yield();
  			}//select next runnig task

  		}	
  		for(int i = 0 ; i < input_size ; i++){
  			wait(NULL);
  		}
  	}
  	else if(strcmp(mode , "SJF") == 0){
  		int *timer = get_share_var("timer") , *nextjob = get_share_var("nextjob");
  		int *end = get_share_var("end") , i = 0 , end_record = -1 , count = 0;
  		queue *head = NULL;
  		*timer = 0;
  		*nextjob = task_set[0].R;
  		*end = -1;
  		set_priority(getpid() , 50);

  		while(1){
  			while(*nextjob > *timer && head == NULL){
  				do_nothing(1);
  				*timer += 1;
  			}//idle
  			if(*end != end_record){		//new ending task
  				end_record = *end;
  				head = delete_queue(head , end_record);
  				count += 1;
  			}
  			if(count == input_size)	break;	//	finish every task
  			//clear ending task
  			while(i < input_size && task_set[i].R <= *timer){
				head = insert_queue_SJF(head , i , task_set[i].T);
  				task_set[i].pid = do_task_FIFO(task_set[i] , timer , nextjob , i , end);
  				if(i + 1 < input_size){
  					*nextjob = task_set[i + 1].R;
				}
				i += 1; 
  			}//create task
  			if(head == NULL)	continue;
  			else{
  				queue *selected = shortest_job(head);
  				int next = selected->id;
  				set_priority(task_set[next].pid , 99);
				sched_yield();
  			}//select next runnig task

  		}	
  		for(int i = 0 ; i < input_size ; i++){
  			wait(NULL);
  		}
  	}
  	else if(strcmp(mode , "PSJF") == 0){
  		int *timer = get_share_var("timer") , *nextjob = get_share_var("nextjob");
  		int *end = get_share_var("end")  , *runtime = get_share_var("runtime")  ;
  		int i = 0 , end_record = -1 , count = 0;
  		queue *head = NULL;
  		*timer = 0;
  		*nextjob = task_set[0].R;
  		*end = -1;
  		*runtime = 0;
  		set_priority(getpid() , 50);

  		while(1){
  			while(*nextjob > *timer && head == NULL){
  				do_nothing(1);
  				*timer += 1;
  			}//idle
  			if(*end != end_record){		//new ending task
  				end_record = *end;
  				head = delete_queue(head , end_record);
  				count += 1;
  			}
  			if(count == input_size)	break;	//	finish every task
  			//clear ending task
  			while(i < input_size && task_set[i].R <= *timer){
				head = insert_queue_SJF(head , i , task_set[i].T);
  				task_set[i].pid = do_task_PSJF(task_set[i] , timer , nextjob , i , end , runtime);
  				if(i + 1 < input_size){
  					*nextjob = task_set[i + 1].R;
				}
				i += 1; 
  			}//create task
  			int next;
  			if(head == NULL)	continue;
  			else{
  				queue *selected = shortest_job(head);
  				next = selected->id;
  				set_priority(task_set[next].pid , 99);
  				sched_yield();
  			}//select next runnig task
  			renew(head , next , runtime);
			
  			//renew the task time
  		}	
  		for(int i = 0 ; i < input_size ; i++){
  			wait(NULL);
  		}
  	}
	return 0;
}

FILE *open_file(char const *name){
	char filename[100];
	sprintf(filename, "./%s" , name);
	printf("%s\n", filename);
  	FILE *file = fopen ( filename, "r" );
  	if(file == NULL)	printf("open err\n");
  	return file;
}

int *get_share_var(char *filename){
	int shfd = shm_open(filename , O_RDWR | O_CREAT, 0777);	
	if (shfd<0) { perror("shm_open"); exit(EXIT_FAILURE); };
	ftruncate(shfd , sizeof(int));
	int *ad = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shfd, (off_t)0);
	if (ad==MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); };
	return ad;
}

queue *insert_queue(queue *head , int num){
	queue *tmp =  (queue*)malloc(sizeof(queue));
	tmp->id = num;
	tmp->next = NULL;
	if(head == NULL){
		tmp->next = tmp;
		head = tmp;
	}else{
		queue *tail = head;
		while(tail->next != head)	tail = tail->next;
		tail->next = tmp;
		tmp->next = head;
	}
	return head;
}

queue *delete_queue(queue *head , int num){
	queue *target = head->next , *before = head , *stop = head;
	while(target->id != num ){
		if(target == stop)	break;
		before = before->next;
		target = target->next;
	}
	if(target->id != num || (target->next == target)){
		return NULL;
	}else if(target == head){
		before->next = target->next;
		return target->next;
	}else{
		before->next = target->next;
		return head;
	}
}

void renew(queue *head ,int id ,int *runtime){
	queue *target = head->next, *stop = head;
	while(target->id != id ){
		target = target->next;
	}
	target->exec_time -= *runtime;
	*runtime = 0;
	return;
}

void print_queue(queue *head ){
	if(head == NULL){
		printf("\n");
		return;
	}
	queue *now = head, *stop = head;
	do{
		printf("%d ", now->id);
		now = now->next;
	}while(now != stop);
	printf("\n");
	return;
}

void set_priority(pid_t pid , int priority){
	struct sched_param param;
    param.sched_priority = priority;
    if (sched_setscheduler(pid, SCHED_FIFO, &param) == -1){
	    perror("sched_setscheduler() failed");
		exit(1);
    }
    return;
}

void sort_readtime(int n, task task_set[]){
	for(int i = 0; i < n - 1; i++){
		for(int j = 0 ; j < n - i - 1 ; j++){
			if(task_set[j].R > task_set[j + 1].R){
				task tmp = task_set[j];
				task_set[j] = task_set[j + 1];
				task_set[j + 1] = tmp;
			}
		}
	}
	return;
}

void do_nothing(int time){
	for(int i = 0; i < time ; i++){
		volatile unsigned long j;
		for(j = 0 ; j < 1000000UL ; j++);
	}
	return;
}

queue *insert_queue_SJF(queue *head , int num , int exec_time){
	queue *tmp =  (queue*)malloc(sizeof(queue));
	tmp->id = num;
	tmp->exec_time = exec_time;
	tmp->next = NULL;
	if(head == NULL){
		tmp->next = tmp;
		head = tmp;
	}else{
		queue *tail = head;
		while(tail->next != head)	tail = tail->next;
		tail->next = tmp;
		tmp->next = head;
	}
	return head;
}

queue *shortest_job(queue *head){
	if(head == NULL)	return NULL;
	queue *now = head , *stop = head , *shortest = head;
	do{
		now = now->next;
		if(now->exec_time < shortest->exec_time){
			shortest = now;
		}
	}while(now != stop);
	return shortest;
}
