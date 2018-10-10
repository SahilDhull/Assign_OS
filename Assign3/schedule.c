#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>
#include<init.h>
#include<types.h>
static u64 numticks;

static void save_current_context()
{
  /*Your code goes in here*/ 
} 

static void schedule_context(struct exec_context *next)
{
  /*Your code goes in here. get_current_ctx() still returns the old context*/
 struct exec_context *current = get_current_ctx();
 printf("schedluing: old pid = %d  new pid  = %d\n", current->pid, next->pid); /*XXX: Don't remove*/
/*These two lines must be executed*/
 set_tss_stack_ptr(next);
 set_current_ctx(next);
/*Your code for scheduling context*/
 return;
}

static struct exec_context *pick_next_context(struct exec_context *list)
{
  /*Your code goes in here*/
  
   return NULL;
}
static void schedule()
{
/* 
 struct exec_context *next;
 struct exec_context *current = get_current_ctx(); 
 struct exec_context *list = get_ctx_list();
 next = pick_next_context(list);
 schedule_context(next);
*/     
}

static void do_sleep_and_alarm_account()
{
 /*All processes in sleep() must decrement their sleep count*/

}

/*The five functions above are just a template. You may change the signatures as you wish*/
void handle_timer_tick()
{
 /*
   This is the timer interrupt handler. 
   You should account timer ticks for alarm and sleep
   and invoke schedule
 */
  u64 rsp;
  asm volatile (
        "push %rax; push %rbx; push %rcx; push %rdx; push %rsi; push %rdi; push %r8;"
        "push %r9; push %r10; push %r11; push %r12; push %r13; push %r14; push %r15;"
    );
  asm volatile( "mov %%rsp, %0" : "=r"(rsp) );
  printf("Got a tick. #ticks = %u\n", numticks++);   /*XXX Do not modify this line*/

  struct exec_context *curr = get_current_ctx();
  struct exec_context *sleeping = get_ctx_by_pid(1);

  // printf("ticks to sleep = %x\n",sleeping->ticks_to_sleep );
  // printf("current pid = %x\n", curr->pid);
  // if(sleeping->ticks_to_sleep>1)
    sleeping->ticks_to_sleep--;
  if(sleeping->ticks_to_sleep==0){
    sleeping->ticks_to_sleep--;
    // printf("--------------------\n");
    curr->state=READY;
    sleeping->state=RUNNING;
    //pushing current registers to context of current process
    asm volatile ("mov %%r15,%0" : "=r" (curr->regs.r15));
    asm volatile ("mov %%r14,%0" : "=r" (curr->regs.r14));
    asm volatile ("mov %%r13,%0" : "=r" (curr->regs.r13));
    asm volatile ("mov %%r12,%0" : "=r" (curr->regs.r12));
    asm volatile ("mov %%r11,%0" : "=r" (curr->regs.r11));
    asm volatile ("mov %%r10,%0" : "=r" (curr->regs.r10));
    asm volatile ("mov %%r9,%0"  : "=r"  (curr->regs.r9));
    asm volatile ("mov %%r8,%0"  : "=r"  (curr->regs.r8));
    // asm volatile ("mov %%rbp,%0" : (curr->regs.rbp));
    asm volatile ("mov %%rdi,%0" : "=r" (curr->regs.rdi));
    asm volatile ("mov %%rsi,%0" : "=r" (curr->regs.rsi));
    asm volatile ("mov %%rdx,%0" : "=r" (curr->regs.rdx));
    asm volatile ("mov %%rcx,%0" : "=r" (curr->regs.rcx));
    asm volatile ("mov %%rbx,%0" : "=r" (curr->regs.rbx));
    asm volatile ("mov %%rax,%0" : "=r" (curr->regs.rax));
    //current registers pushed
    // printf("--------------------\n");
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    curr->regs.rbp = *ptr;
    curr->regs.entry_rip = *(ptr+1);
    curr->regs.entry_cs = *(ptr+2);
    curr->regs.entry_rflags = *(ptr+3);
    curr->regs.entry_rsp = *(ptr+4);
    curr->regs.entry_ss = *(ptr+5);

    set_tss_stack_ptr(sleeping);
    set_current_ctx(sleeping);
    ack_irq();
    // printf("rbp = %x\n", ptr);
    asm volatile ("mov %0,%%r15" : "=r" (sleeping->regs.r15));
    asm volatile ("mov %0,%%r14" : "=r" (sleeping->regs.r14));
    asm volatile ("mov %0,%%r13" : "=r" (sleeping->regs.r13));
    asm volatile ("mov %0,%%r12" : "=r" (sleeping->regs.r12));
    asm volatile ("mov %0,%%r11" : "=r" (sleeping->regs.r11));
    asm volatile ("mov %0,%%r10" : "=r" (sleeping->regs.r10));
    asm volatile ("mov %0,%%r9"  : "=r"  (sleeping->regs.r9));
    asm volatile ("mov %0,%%r8"  : "=r"  (sleeping->regs.r8));
    // asm volatile ("mov %0,%%rbp" : (sleeping->regs.rbp));
    asm volatile ("mov %0,%%rdi" : "=r" (sleeping->regs.rdi));
    asm volatile ("mov %0,%%rsi" : "=r" (sleeping->regs.rsi));
    asm volatile ("mov %0,%%rdx" : "=r" (sleeping->regs.rdx));
    asm volatile ("mov %0,%%rcx" : "=r" (sleeping->regs.rcx));
    asm volatile ("mov %0,%%rbx" : "=r" (sleeping->regs.rbx));
    asm volatile ("mov %0,%%rax" : "=r" (sleeping->regs.rax));
    // printf("--------------------\n");
    // asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));

    *ptr = sleeping->regs.rbp;
    *(ptr+1) = sleeping->regs.entry_rip;
    *(ptr+2) = sleeping->regs.entry_cs;
    *(ptr+3) = sleeping->regs.entry_rflags;
    *(ptr+4) = sleeping->regs.entry_rsp;
    *(ptr+5) = sleeping->regs.entry_ss;

    // printf("--------------------\n");

    asm volatile("mov %%rbp, %%rsp;"
               "pop %%rbp;"
               "iretq;"
               :::"memory");
  }
  // printf("coming here\n");

  // sleeping->ticks_to_sleep--;
  // u64* sp;
  // asm volatile("mov %%rbp, %0" : "=r"(sp) :: "memory");
  // printf("%x\n",*(sp+1) );
  // printf("custom handler %x\n", *());
  // printf("ticks to alarm = %x\n", curr->ticks_to_alarm);
  if(curr->ticks_to_alarm == 0){
    // printf("Coming here ------------------------\n");
    curr->ticks_to_alarm = curr->alarm_config_time;
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;"
                    : "=r" (ptr)
                    :
                    :);
    invoke_sync_signal(SIGALRM,ptr+4 , ptr+1);
  }
  curr->ticks_to_alarm--;
  // printf("ticks to alarm = %x\n", curr->ticks_to_alarm);
  // printf(" ticks to alarm = %x\n", curr->ticks_to_alarm);
  
  ack_irq();  /*acknowledge the interrupt, before calling iretq */
  asm volatile( "mov %0,%%rsp" : : "r"(rsp) );
  asm volatile (
        "pop %r15; pop %r14; pop %r13; pop %r12; pop %r11; pop %r10; pop %r9;"
        "pop %r8; pop %rdi; pop %rsi; pop %rdx; pop %rcx; pop %rbx; pop %rax;"
    );
  asm volatile("mov %%rbp, %%rsp;"
               "pop %%rbp;"
               "iretq;"
               :::"memory");
}

void do_exit()
{
  /*You may need to invoke the scheduler from here if there are
    other processes except swapper in the system. Make sure you make 
    the status of the current process to UNUSED before scheduling 
    the next process. If the only process alive in system is swapper, 
    invoke do_cleanup() to shutdown gem5 (by crashing it, huh!)
    */
    do_cleanup();  /*Call this conditionally, see comments above*/
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
  struct exec_context *curr = get_current_ctx();
  struct exec_context *swapper = get_ctx_by_pid(0);
  curr->ticks_to_sleep = ticks;
  curr->state = WAITING;
  swapper->state=RUNNING;
  // asm volatile("mov %%cs, %0" : "=r" ((curr->regs).entry_cs));
  // asm volatile("mov %%ss, %0" : "=r" ((curr->regs).entry_cs));
  // u64* ptr1;
  // asm volatile("mov %%rbp, %0" : "=r"(ptr1) :: "memory");
  u64* addr = (u64*)((((u64)curr->os_stack_pfn+1)<<12)-8);
  curr->regs.entry_rflags = *(addr-2);
  curr->regs.entry_rsp = *(addr-1);
  curr->regs.entry_cs = 0x23;
  curr->regs.entry_ss = 0x2b;
  curr->regs.rbp = *(addr-10);
  curr->regs.entry_rip = *(addr-4);
  // printf("rip in sleep = %x\n",*(addr-4));
  
  set_current_ctx(swapper);
  set_tss_stack_ptr(swapper);
  asm volatile ("mov %0,%%r15" : "=r" (swapper->regs.r15));
  asm volatile ("mov %0,%%r14" : "=r" (swapper->regs.r14));
  asm volatile ("mov %0,%%r13" : "=r" (swapper->regs.r13));
  asm volatile ("mov %0,%%r12" : "=r" (swapper->regs.r12));
  asm volatile ("mov %0,%%r11" : "=r" (swapper->regs.r11));
  asm volatile ("mov %0,%%r10" : "=r" (swapper->regs.r10));
  asm volatile ("mov %0,%%r9" :  "=r" (swapper->regs.r9));
  asm volatile ("mov %0,%%r8" :  "=r" (swapper->regs.r8));
  // asm volatile ("mov %0,%%rbp" : (swapper->regs.rbp));
  asm volatile ("mov %0,%%rdi" : "=r" (swapper->regs.rdi));
  asm volatile ("mov %0,%%rsi" : "=r" (swapper->regs.rsi));
  asm volatile ("mov %0,%%rdx" : "=r" (swapper->regs.rdx));
  asm volatile ("mov %0,%%rcx" : "=r" (swapper->regs.rcx));
  asm volatile ("mov %0,%%rbx" : "=r" (swapper->regs.rbx));
  asm volatile ("mov %0,%%rax" : "=r" (swapper->regs.rax));
  u64* ptr;
  asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
  *(ptr+1) = swapper->regs.entry_rip;
  *(ptr+2) = swapper->regs.entry_cs;
  *(ptr+3) = swapper->regs.entry_rflags;
  *(ptr+4) = swapper->regs.entry_rsp;
  *(ptr+5) = swapper->regs.entry_ss;
  asm volatile("mov %%rbp, %%rsp;"
               "pop %%rbp;"
               "iretq;"
               :::"memory");
}

/*
  system call handler for clone, create thread like 
  execution contexts
*/
long do_clone(void *th_func, void *user_stack)
{
   
}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip){
   /*If signal handler is registered, manipulate user stack and RIP to execute signal handler*/
   /*ustackp and urip are pointers to user RSP and user RIP in the exception/interrupt stack*/
   printf("Called signal with ustackp=%x urip=%x\n", *ustackp, *urip);
   /*Default behavior is exit( ) if sighandler is not registered for SIGFPE or SIGSEGV.
    Ignore for SIGALRM*/
  struct exec_context *curr = get_current_ctx();
  if(curr->sighandlers[signo] == NULL){
    if(signo != SIGALRM){
      do_exit();
    }
  }
  else{
    u64* c = (u64*)*ustackp;
    *(c-1) = *urip;
    *(ustackp) = (u64)(c-1);
    *(urip) = (u64)((curr->sighandlers)[signo]);
    // printf("%x   %x\n", *ustackp,*urip);
    // printf("custom handler %x\n", curr->sighandlers[signo]);
  }
}
/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
  // printf("Handler Address : ----------------%x \n", handler);
  struct exec_context *curr = get_current_ctx();
  (curr->sighandlers)[signo] = (void*)handler;
}
// printf("coming here\n");
/*system call handler for alarm*/
long do_alarm(u32 ticks)
{
  // printf("coming here\n");
  struct exec_context *curr = get_current_ctx();
  curr->alarm_config_time = ticks;
  curr->ticks_to_alarm = ticks;
  // printf(" ticks to alarm in the handler = = %x\n", curr->ticks_to_alarm);
}
