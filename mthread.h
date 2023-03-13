
#ifndef __THREAD_H__
#define __THREAD_H__
//the max counts of threads
#define N_TASKS 10


//the status of a thread
enum{
  RUNNING = 0,
  SLEEP = 1,
  EXIT = 2,
};

//64 bits register
enum{
  RDI = 7,
  RSI = 8,
  RETADDR = 9,
  RSP = 13,
};

//the context of a thread, 64 bits
struct context{
  void *regs[14];
  char *stack_pointer;
  unsigned long long stack_size;
};

struct stack_mem{
  int stack_size;
  char *stack_bp;
  char *stack_buffer;
};

typedef struct task_struct{
  //thread index
  int id;
  //thread function pointer
  void (*th_fn)();
  int wakeuptime;
  int status;
  int timeslice;
  int priority;
  struct stack_mem* stackmem;
  struct context ctx; 
}mtask, *ptask;



typedef void* (*ctx_fn)(void* param1);
//function declaration
static void start(ptask tsk);
int mthread_create(int *tid, void (*th_fn)());
int ctx_make(struct context* ctx, ctx_fn fn, const void* parm1);
struct stack_mem* alloc_stackmem(unsigned int stack_size);
void schedule();
void mthread_join(int tid);
void msleep(int s);
void yield();
void thread_signal();
void thread_wait();
void event_loop();
void resume(ptask task);
void init_list();
void RegRecallEvent();
#endif
