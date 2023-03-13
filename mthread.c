#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/time.h>
#include"mthread.h"
#include<string.h>

//#define SYM
#define ASYM

typedef void* (*ctx_fn)(void* param1);

//the current thread
static mtask main_task = {0, NULL, 0, RUNNING, 15, 15};
ptask curr_trd = &main_task;
ptask nxt_trd = NULL;
ptask task[N_TASKS] = {&main_task, };


static unsigned int getmstime(); 
static ptask pick() //linus
{
  int i, next, c;
  for (i = 0; i < N_TASKS; ++i) {
    if (task[i] && task[i]->status != EXIT && getmstime() > task[i]->wakeuptime){
      task[i]->status = RUNNING;
    }
  }
  while(1) {
    c = -1;
    next = 0;
    for (i = 0; i < N_TASKS; ++i) {
      if (!task[i]) continue;
      if (task[i]->status == RUNNING && task[i]->timeslice > c) {
        c = task[i]->timeslice;
        next = i;
      }
    }
    if (c) break;
    if (c == 0) {                       
      for (i = 0; i < N_TASKS; ++i) {
        if(task[i]) {
          task[i]->timeslice = task[i]->priority + (task[i]->timeslice >> 1);
        }
      }
    }
  }
  return task[next];
}

void closealarm();
void openalarm();
void ctx_swap(struct context*, struct context*);
void schedule() 
{
    ptask next= pick();
    //nxt_trd = next;
    if (next) {
      ptask curr_tmp = curr_trd;
      curr_trd = next;
      closealarm();
      ctx_swap(&(curr_tmp->ctx), &(next->ctx));
      openalarm();
    }
    else{
      printf("pick null!\n");
      return;
    }
      //curr_trd = nxt_trd;
      //nxt_trd = NULL;
}
// the function to start a coroutine
static void start(ptask tsk) 
{
  openalarm();
  //curr_trd = nxt_trd;
  //nxt_trd = NULL;
  tsk->th_fn();
  tsk->status = EXIT;
  printf("***Thread%d exited***\n", tsk->id);
  #ifdef SYM
  schedule();
  #endif
}

//allocate stack memory
struct stack_mem* alloc_stackmem(unsigned int stack_size){
  struct stack_mem* a = (struct stack_mem*)malloc(sizeof(struct stack_mem));
  a->stack_buffer = (char*)malloc(stack_size);
  a->stack_size = stack_size;
  a->stack_bp = a->stack_buffer + stack_size;
  return a;
}

//initialize a coroutine's context which has not start
int ctx_make(struct context* ctx, ctx_fn fn, const void* parm1){
  char* sp = ctx->stack_pointer + ctx->stack_size - sizeof(void*);
  //align 2bytes
  sp = (char*)((unsigned long)sp & -16LL);
  
  memset(ctx->regs, 0, sizeof(ctx->regs));
  void** ret_addr = (void**)(sp);
  *ret_addr = (void*)fn;
  //the top pointer of the stack
  ctx->regs[RSP] = sp;
  //the return address
  ctx->regs[RETADDR] = (char*)fn;
  //parameters
  ctx->regs[RDI] = (char*) parm1;
  return 0;
}

//create a thread
int mthread_create(int *tid, void (*th_fn)()){
  int id = -1;
  ptask tsk = (ptask)malloc(sizeof(mtask));
  while(++id < N_TASKS && task[id]);
  if (id == N_TASKS){
    free(tsk);
    return -1;
  }
  task[id] = tsk;
  if(tid)
    *tid = id;

  //initialize TCB
  tsk->id = id;
  tsk->th_fn = th_fn;
  tsk->priority = 15;
  tsk->status = RUNNING;
  tsk->timeslice = 15;
  tsk->wakeuptime = 0;
  //initialize stack
  struct stack_mem* stackmem = alloc_stackmem(128 * 1024);
  tsk->stackmem = stackmem;
  tsk->ctx.stack_pointer = stackmem->stack_buffer;
  tsk->ctx.stack_size = stackmem->stack_size;
  ctx_make(&tsk->ctx, (ctx_fn)start, tsk);
  return 0;
}


void mthread_join(int tid) 
{
  while(task[tid]->status != EXIT) {
		schedule();
  }
  free(task[tid]);
  task[tid] = NULL;
}


static unsigned int getmstime() 
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
     perror("gettimeofday");
     exit(-1);
  }
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}



void msleep(int s)        
{
	curr_trd->wakeuptime = getmstime() + 1000*s;
	curr_trd->status = SLEEP;
	schedule();
}


static void mtimer() 			//时间片监控
{
	if(--(curr_trd->timeslice) > 0) 
	  return;
	curr_trd->timeslice = 0;
	schedule();
}
//symmetrical thread need time 
#ifdef SYM
__attribute__((constructor))
static void init()                    //the function start before main
{
  struct itimerval value;
  value.it_value.tv_sec = 0;
  value.it_value.tv_usec = 1000;
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 1000*10; // 10 ms
  if (setitimer(ITIMER_REAL, &value, NULL) < 0) {
    perror("setitimer");
  }
  signal(SIGALRM, mtimer);
}
#endif
void closealarm() 
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
      perror("sigprocmask BLOCK");
    }
}
void openalarm() 
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) {
      perror("sigprocmask BLOCK");
    }
}


//extension 1
//code for asymmetrical  thread
#ifdef ASYM

#define MAX_SIZE 128
ptask callstack[MAX_SIZE] = {&main_task, };
int stack_sp = 0;//the stack top pointer point the current stack top

struct stTreadList_t;
typedef struct stTreadListItem_t{
  ptask TaskItem;
  struct stTreadListItem_t* prev;
  struct stTreadListItem_t* next;
  struct stTreadList_t* list;
  unsigned long long TriggerTime;//triggering time = now time + delay time
}stTreadListItem_t;

typedef struct stTreadList_t{
  stTreadListItem_t* head;
  stTreadListItem_t* tail;
}stTreadList_t;

stTreadList_t *ActiveList, *BlockedList;

void init_list(){
  ActiveList = (stTreadList_t *)malloc(sizeof(stTreadList_t));
  BlockedList = (stTreadList_t *)malloc(sizeof(stTreadList_t));
}
//add item to list tail
void AddToList(stTreadList_t* list, stTreadListItem_t* list_item){
  //list is empty
  if(list->head == NULL && list->tail == NULL){
    list->head = list_item;
    list->tail = list_item;
    list_item->list = list;
  }
  else{
    list->tail->next = list_item;
    list_item->prev = list->tail;
    list_item->list = list;
    list->tail = list_item;
  }
}

void RemoveFromList(stTreadList_t* list, stTreadListItem_t* list_item){
  //only one in list
  if(list->head == list_item && list->tail == list_item){
    list->head = NULL;
    list->tail = NULL;
    list_item->list = NULL;
  }
  //item is the first in the list
  else if(list->head == list_item){
    list->head = list_item->next;
    list_item->prev = NULL;
    list_item->next = NULL;
    list_item->list = NULL;
  }
  else if(list->tail == list_item){
    list->tail = list_item->prev;
    list_item->prev = NULL;
    list_item->next = NULL;
    list_item->list = NULL;
  }
  else{
    list_item->prev->next = list_item->next;
    list_item->prev = NULL;
    list_item->next = NULL;
    list_item->list = NULL;
  }
  //free(list_item);
}

void resume(ptask task){
      ptask curr_tmp = curr_trd;
      curr_trd = task;
      callstack[++stack_sp] = task;
      closealarm();
      ctx_swap(&(curr_tmp->ctx), &(task->ctx));
      openalarm();
}

void yield(){
  stack_sp--;
  ptask new_task = callstack[stack_sp];
  ptask curr_tmp = curr_trd;
  curr_trd = new_task;
  closealarm();
  ctx_swap(&(curr_tmp->ctx), &(new_task->ctx));
  openalarm();
}

void thread_wait()
{
  stTreadListItem_t *WaitEvent = (stTreadListItem_t *)malloc(sizeof(stTreadListItem_t));
  WaitEvent->TaskItem = curr_trd;
  AddToList(BlockedList, WaitEvent);
  yield();
}

void thread_signal(){
  stTreadListItem_t *item = BlockedList->head;
  RemoveFromList(BlockedList, item);
  AddToList(ActiveList, item);
}

int list_is_empty(stTreadList_t *list){
  if(list->head == NULL && list->tail == NULL)
    return 1;
  return 0;
}

void RegRecallEvent(){
  stTreadListItem_t *item = (stTreadListItem_t *)malloc(sizeof(stTreadListItem_t));
  item->TaskItem = curr_trd;
  AddToList(ActiveList, item);
}

void event_loop(){
  while(1){
    while(!list_is_empty(ActiveList)){
      stTreadListItem_t *item = ActiveList->head;
      RemoveFromList(ActiveList, item);
      resume(item->TaskItem);
      free(item);
    }
  }
}
#endif

//extension 2
//timimg wheel code begin
typedef struct stTimeWheel_t{
  stTreadList_t *pLists;
  int ListNum;

  unsigned long long StartTime;
  long long StartIdx;
}stTimeWheel_t;

stTimeWheel_t *TimeWheel;

void AllocTimeWheel(int size){
  TimeWheel = (stTimeWheel_t *)calloc(1, sizeof(stTimeWheel_t));
  TimeWheel->ListNum = size;
  TimeWheel->pLists = (stTreadList_t *)calloc(1, sizeof(stTreadList_t)*size);
  TimeWheel->StartTime = getmstime();
  TimeWheel->StartIdx = 0;
}

//Add an item(also called event) to timming wheel, which will be triggred in futrue.
int AddTimeWheel(stTreadListItem_t *AddItem){
  unsigned long long NowTime = getmstime();
  if(TimeWheel->StartTime == 0){
    TimeWheel->StartTime = getmstime();
    TimeWheel->StartIdx = 0;
  }
  if(NowTime < TimeWheel->StartTime){
    printf("ERROR: Now is before the start time of timming whell!\n");
    return __LINE__;
  }
  if(AddItem->TriggerTime << NowTime){
    printf("ERROR: The triggering time of event is before Now!\n");
    return __LINE__;
  }
  unsigned long long diff = AddItem->TriggerTime - TimeWheel->StartTime;

  if(diff > TimeWheel->ListNum){
    printf("ERROR: Diff is bigger than TimeWheel!\n");
    return __LINE__;
  }
  AddToList(TimeWheel->pLists + (TimeWheel->StartIdx + diff) % TimeWheel->ListNum , AddItem);
  return 0;

}


//if the triigering time of a item is arrived/expired, remove it frome timming wheel and add it to 
//active list to trigger in function "event_loop".
void RemoveTimeWheel(){
  unsigned long long NowTime = getmstime();
  if(TimeWheel->StartTime == 0){
    TimeWheel->StartTime = getmstime();
    TimeWheel->StartIdx = 0;
  }
  if(NowTime < TimeWheel->StartTime){
    return;
  }
  int cnt = NowTime -  TimeWheel->StartTime + 1;
  if(cnt > TimeWheel->StartTime){
    cnt = TimeWheel->StartTime;
  }
  if(cnt < 0){
    return;
  }
  for(int i = 0; i < cnt; i++){
    int idx = (TimeWheel->StartIdx + i)%TimeWheel->ListNum;
    while(!list_is_empty(TimeWheel->pLists + idx)){
      stTreadListItem_t *tmp = (TimeWheel->pLists + idx)->head;
      AddToList(ActiveList, tmp);
      RemoveFromList(TimeWheel->pLists + idx, tmp);
    }
  }
}

//using time interrupt in function "init" to goto main thread to add new thread
