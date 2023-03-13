#include <stdio.h>
#include <stdlib.h>
#include "mthread.h"

extern ptask curr_trd;
extern ptask task[N_TASKS];
void fun1() 
{
  int i = 10;
  while(i--) {
    printf("fun1 is running\n");
    msleep(2);
  }
}
void fun2() 
{
  int i = 5;
  while(i--) {
    printf("fun2 is running\n");
    msleep(1);
  }
}
void fun3() 
{
  int i = 2;
  while(i--) {
    printf("fun3 is running\n");
    msleep(5);
  }
}
void fun4() 
{
  int i = 10;
  while(i--) {
    printf("fun4 is running\n");
	msleep(3);
  }
}

int main() 
{ 
  int tid1, tid2, tid3, tid4;
  printf("The main process is running\n");
  mthread_create(&tid1, fun1);
  printf("***Thread%d created***\n", tid1);
  mthread_create(&tid2, fun2);
  printf("***Thread%d created***\n", tid2);
  mthread_create(&tid3, fun3);
  printf("***Thread%d created***\n", tid3);
  mthread_create(&tid4, fun4);
  printf("***Thread%d created***\n", tid4);
  mthread_join(tid1);
  mthread_join(tid2);
  mthread_join(tid3);
  mthread_join(tid4);
  printf("The main process exited\n");
  return 0;
}
