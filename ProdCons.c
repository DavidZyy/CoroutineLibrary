#include<stdio.h>
#include<stdlib.h>
#include"mthread.h"
#include<unistd.h>

extern ptask task[N_TASKS];
int resource = 0;

void producer(){
  while(1){
    sleep(1);
    resource++;
    printf("The producer produce a resource!\n");
    thread_signal();
    RegRecallEvent();
    yield();
  }
}

void consumer(){
  while(1){
    sleep(1);
    if(resource == 0){
      thread_wait();
    }
    resource--;
    printf("The consumer consume a resource!\n");
  }
}

int main(){
  init_list();
  int tid1, tid2;
  mthread_create(&tid1, producer);
  mthread_create(&tid2, consumer );
  resume(task[tid2]);
  resume(task[tid1]);
  event_loop();
}
