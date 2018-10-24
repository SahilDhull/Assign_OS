#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>
#include<init.h>
#include<types.h>
static u64 numticks;
/*
static void save_current_context()
{
    //Your code goes in here
}

static void schedule_context(struct exec_context *next, u64* ptr, int flag)
{
    //Your code goes in here. get_current_ctx() still returns the old context
    // u64* ptr;
    //change the value of rbp accordingly
    // asm volatile("mov %%rbp,%0" : "=r"(ptr));
    struct exec_context *curr = get_current_ctx();
    printf("scheduling: old pid = %d  new pid  = %d\n", curr->pid, next->pid); //XXX: Don't remove

  //   u64* sp;
  //   asm volatile("sub $0x18,%rsp;");
  // asm volatile("mov %%rsp,%0":"=r"(sp)::"memory");
  // printf("sp ===== %x\n",*sp );
  // printf("rbp obtained ==%s\n", *ptr);

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

    curr->regs.rbp = *ptr;
    curr->regs.entry_rip = *(ptr+1);
    curr->regs.entry_cs = *(ptr+2);
    curr->regs.entry_rflags = *(ptr+3);
    curr->regs.entry_rsp = *(ptr+4);
    curr->regs.entry_ss = *(ptr+5);

    set_tss_stack_ptr(next);
    set_current_ctx(next);
    // if(flag==1) ack_irq();
    // printf("------------------------\n");
    *ptr = next->regs.rbp;
    *(ptr+1) = next->regs.entry_rip;
    *(ptr+2) = next->regs.entry_cs;
    *(ptr+3) = next->regs.entry_rflags;
    *(ptr+4) = next->regs.entry_rsp;
    *(ptr+5) = next->regs.entry_ss;
    // asm volatile("mov %0,%%rbp": "=r" (ptr));

    next->state = RUNNING;
    asm volatile ("mov %0,%%r15" : "=r" (next->regs.r15));
    asm volatile ("mov %0,%%r14" : "=r" (next->regs.r14));
    asm volatile ("mov %0,%%r13" : "=r" (next->regs.r13));
    asm volatile ("mov %0,%%r12" : "=r" (next->regs.r12));
    asm volatile ("mov %0,%%r11" : "=r" (next->regs.r11));
    asm volatile ("mov %0,%%r10" : "=r" (next->regs.r10));
    asm volatile ("mov %0,%%r9" :  "=r" (next->regs.r9));
    asm volatile ("mov %0,%%r8" :  "=r" (next->regs.r8));
    // asm volatile ("mov %0,%%rbp" : (next->regs.rbp));
    asm volatile ("mov %0,%%rdi" : "=r" (next->regs.rdi));
    asm volatile ("mov %0,%%rsi" : "=r" (next->regs.rsi));
    asm volatile ("mov %0,%%rdx" : "=r" (next->regs.rdx));
    asm volatile ("mov %0,%%rcx" : "=r" (next->regs.rcx));
    asm volatile ("mov %0,%%rbx" : "=r" (next->regs.rbx));
    asm volatile ("mov %0,%%rax" : "=r" (next->regs.rax));

    asm volatile("mov %%rbp, %%rsp;"
    "pop %%rbp;"
    "iretq;"
    :::"memory");
    return;
}

static struct exec_context *pick_next_context(struct exec_context *list)
{
    //Your code goes in here

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
    
}

static void do_sleep_and_alarm_account()
{
    //All processes in sleep() must decrement their sleep count
}

//The five functions above are just a template. You may change the signatures as you wish
*/
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
    struct exec_context *list = get_ctx_list();

    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));

    for(int i=1; i < 16; i++){
        struct exec_context* proc = &(list[i]);
        if(proc->state==WAITING){
            if(proc->ticks_to_sleep>1) proc->ticks_to_sleep--;
            else if(proc->ticks_to_sleep==1){
                proc->ticks_to_sleep--;
                proc->state=READY;
            }
        }
    }

    int ready = 0;
    for(int i=1;i<16;i++){
        int ind;
        if(((i+curr->pid)%16)==0) continue;
        if(i+curr->pid>16) ind = (i+curr->pid)%16;
        else ind = i+curr->pid;
        struct exec_context *next = &(list[ind]);
        if(next->state == READY){
            //schedule it
            curr->state = READY;
            // schedule_context(next,  ptr,1);
            sleeping = next;
            ready = 1;
            break;
        }
    }
    // printf("current pid = %x\n", curr->pid);
  /*
    if(ready==0){
    schedule_context(list[0]);
        }


    // printf("ticks to sleep = %x\n",sleeping->ticks_to_sleep );
    // printf("current pid = %x\n", curr->pid);
    // if(sleeping->ticks_to_sleep>1)

    sleeping->ticks_to_sleep--;
    if(sleeping->ticks_to_sleep==0){
    sleeping->ticks_to_sleep--;
    // printf("--------------------\n");
    curr->state=READY;
    sleeping->state=RUNNING;*/
    //pushing current registers to context of current process
    if(ready==1){
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
        // u64* ptr;
        asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
        curr->regs.rbp = *ptr;
        curr->regs.entry_rip = *(ptr+1);
        curr->regs.entry_cs = *(ptr+2);
        curr->regs.entry_rflags = *(ptr+3);
        curr->regs.entry_rsp = *(ptr+4);
        curr->regs.entry_ss = *(ptr+5);
        printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, sleeping->pid); //XXX: Don't remove

        set_tss_stack_ptr(sleeping);
        set_current_ctx(sleeping);
        ack_irq();

        *ptr = sleeping->regs.rbp;
        *(ptr+1) = sleeping->regs.entry_rip;
        *(ptr+2) = sleeping->regs.entry_cs;
        *(ptr+3) = sleeping->regs.entry_rflags;
        *(ptr+4) = sleeping->regs.entry_rsp;
        *(ptr+5) = sleeping->regs.entry_ss;

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
        invoke_sync_signal(SIGALRM,ptr+4 , ptr+1);
    }
    curr->ticks_to_alarm--;
    // printf("ticks to alarm = %x\n", curr->ticks_to_alarm);
    // printf(" ticks to alarm = %x\n", curr->ticks_to_alarm);

    ack_irq();  //acknowledge the interrupt, before calling iretq
  
  //-------------------------------
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
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    struct exec_context *curr = get_current_ctx();
    curr->state = UNUSED;
    struct exec_context *list = get_ctx_list();
    // printf("------entering do_exit-------- with pid = %x \n",curr->pid);
    // printf("rbp == %x\n", *ptr);
    int ready = 0, wait = 0;
    for(int i=1;i<16;i++){
        int ind;
        if(((i+curr->pid)%16)==0) continue;
        if(i+curr->pid>16) ind = (i+curr->pid)%16;
        else ind = i+curr->pid;
        struct exec_context *next = &(list[ind]);
        if(next->state == READY){
            // printf("swapping with pid = %x\n",next->pid );
            //schedule it
            // schedule_context(next, ptr,0);
            printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove

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

    curr->regs.rbp = *ptr;
    curr->regs.entry_rip = *(ptr+1);
    curr->regs.entry_cs = *(ptr+2);
    curr->regs.entry_rflags = *(ptr+3);
    curr->regs.entry_rsp = *(ptr+4);
    curr->regs.entry_ss = *(ptr+5);

    set_tss_stack_ptr(next);
    set_current_ctx(next);
    // if(flag==1) ack_irq();
    // printf("------------------------\n");
    *ptr = next->regs.rbp;
    *(ptr+1) = next->regs.entry_rip;
    *(ptr+2) = next->regs.entry_cs;
    *(ptr+3) = next->regs.entry_rflags;
    *(ptr+4) = next->regs.entry_rsp;
    *(ptr+5) = next->regs.entry_ss;
    // asm volatile("mov %0,%%rbp": "=r" (ptr));

    next->state = RUNNING;
    asm volatile ("mov %0,%%r15" : "=r" (next->regs.r15));
    asm volatile ("mov %0,%%r14" : "=r" (next->regs.r14));
    asm volatile ("mov %0,%%r13" : "=r" (next->regs.r13));
    asm volatile ("mov %0,%%r12" : "=r" (next->regs.r12));
    asm volatile ("mov %0,%%r11" : "=r" (next->regs.r11));
    asm volatile ("mov %0,%%r10" : "=r" (next->regs.r10));
    asm volatile ("mov %0,%%r9" :  "=r" (next->regs.r9));
    asm volatile ("mov %0,%%r8" :  "=r" (next->regs.r8));
    // asm volatile ("mov %0,%%rbp" : (next->regs.rbp));
    asm volatile ("mov %0,%%rdi" : "=r" (next->regs.rdi));
    asm volatile ("mov %0,%%rsi" : "=r" (next->regs.rsi));
    asm volatile ("mov %0,%%rdx" : "=r" (next->regs.rdx));
    asm volatile ("mov %0,%%rcx" : "=r" (next->regs.rcx));
    asm volatile ("mov %0,%%rbx" : "=r" (next->regs.rbx));
    asm volatile ("mov %0,%%rax" : "=r" (next->regs.rax));

    asm volatile("mov %%rbp, %%rsp;"
    "pop %%rbp;"
    "iretq;"
    :::"memory");
            ready = 1;
            break;
        }
        if(next->state == WAITING) wait=1;
    }
    if(ready==0 && wait==1){
        //schedule swapper
        struct exec_context *next = &(list[0]);
        // schedule_context(next, ptr,0);
        printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove

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

    curr->regs.rbp = *ptr;
    curr->regs.entry_rip = *(ptr+1);
    curr->regs.entry_cs = *(ptr+2);
    curr->regs.entry_rflags = *(ptr+3);
    curr->regs.entry_rsp = *(ptr+4);
    curr->regs.entry_ss = *(ptr+5);

    set_tss_stack_ptr(next);
    set_current_ctx(next);
    // printf("------------------------\n");
    *ptr = next->regs.rbp;
    *(ptr+1) = next->regs.entry_rip;
    *(ptr+2) = next->regs.entry_cs;
    *(ptr+3) = next->regs.entry_rflags;
    *(ptr+4) = next->regs.entry_rsp;
    *(ptr+5) = next->regs.entry_ss;
    // asm volatile("mov %0,%%rbp": "=r" (ptr));

    next->state = RUNNING;
    asm volatile ("mov %0,%%r15" : "=r" (next->regs.r15));
    asm volatile ("mov %0,%%r14" : "=r" (next->regs.r14));
    asm volatile ("mov %0,%%r13" : "=r" (next->regs.r13));
    asm volatile ("mov %0,%%r12" : "=r" (next->regs.r12));
    asm volatile ("mov %0,%%r11" : "=r" (next->regs.r11));
    asm volatile ("mov %0,%%r10" : "=r" (next->regs.r10));
    asm volatile ("mov %0,%%r9" :  "=r" (next->regs.r9));
    asm volatile ("mov %0,%%r8" :  "=r" (next->regs.r8));
    // asm volatile ("mov %0,%%rbp" : (next->regs.rbp));
    asm volatile ("mov %0,%%rdi" : "=r" (next->regs.rdi));
    asm volatile ("mov %0,%%rsi" : "=r" (next->regs.rsi));
    asm volatile ("mov %0,%%rdx" : "=r" (next->regs.rdx));
    asm volatile ("mov %0,%%rcx" : "=r" (next->regs.rcx));
    asm volatile ("mov %0,%%rbx" : "=r" (next->regs.rbx));
    asm volatile ("mov %0,%%rax" : "=r" (next->regs.rax));

    asm volatile("mov %%rbp, %%rsp;"
    "pop %%rbp;"
    "iretq;"
    :::"memory");
    }
    else if(ready==0 && wait==0){
        printf("do_cleanup\n");
        os_pfn_free(OS_PT_REG,curr->pgd);
        do_cleanup();  //Call this conditionally, see comments above
    }
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
    struct exec_context *curr = get_current_ctx();
    curr->ticks_to_sleep = ticks;
    curr->state = WAITING;

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


    struct exec_context *swapper/* = get_ctx_by_pid(0)*/;
    struct exec_context *list = get_ctx_list();

    int ready = 0;
    for(int i=1;i<16;i++){
        int ind;
        if(((i+curr->pid)%16)==0) continue;
        if(i+curr->pid>16) ind = (i+curr->pid)%16;
        else ind = i+curr->pid;
        struct exec_context *next = &(list[ind]);
        if(next->state == READY){
            swapper = next;
            ready = 1;
            break;
        }
    }


    if(ready == 0) swapper = get_ctx_by_pid(0);
    swapper->state=RUNNING;
    printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, swapper->pid); //XXX: Don't remove

    // printf("------------------------\n");
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
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    struct exec_context *new = get_new_ctx();
    struct exec_context *curr = get_current_ctx();
    // u64 k = *th_func;
    // printf("th_func address = %x\n",th_func );
    u64* addr=(u64*)((((u64)curr->os_stack_pfn+1)<<12)-8);
    new->type = curr->type;
    new->pgd = curr->pgd;
    new->used_mem = curr->used_mem;
    new->pgd = curr->pgd;
    //os_stack_pfn is different
    new->os_rsp = curr->os_rsp;
    // new->mms = curr->mms;
    for(int i=0; i < MAX_MM_SEGS; i++){
        new->mms[i] = curr->mms[i];
    }
    //name and regs will be changed
    new->pending_signal_bitmap = curr->pending_signal_bitmap;
    // new->sighandlers = curr->sighandlers;
    for(int i=0; i < MAX_SIGNALS; i++){
        new->sighandlers[i] = curr->sighandlers[i];
    }
    new->ticks_to_sleep = curr->ticks_to_sleep;
    new->alarm_config_time = curr->alarm_config_time;
    new->ticks_to_alarm = curr->ticks_to_alarm;
    //set os_stack_pfn, name and regs
    new->os_stack_pfn = os_pfn_alloc(OS_PT_REG);
    // memcpy(new->name,curr->name,(u32)(new->pid));
    char copy[CNAME_MAX];
    int n = strlen(curr->name);
    memcpy(copy,curr->name,n);
    int pid = new->pid;
    if(pid<10){
        copy[n] = pid + '0';
        copy[n+1] = '\0';
    }
    else{
        copy[n] = '1';
        copy[n+1] = pid-10+'0';
        copy[n+2] = '\0';
    }
    memcpy(new->name,copy,strlen(copy));
    new->regs = curr->regs;
    new->regs.entry_cs = 0x23;
    new->regs.entry_ss = 0x2b;
    new->regs.entry_rflags=*(addr-2);
    new->regs.entry_rip = (u64)th_func;
    new->regs.entry_rsp = (u64)user_stack;
    new->regs.rbp = (u64)user_stack;
    //change in rflags

    new->state = READY;
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
/*system call handler for alarm*/
long do_alarm(u32 ticks)
{
    // printf("coming here\n");
    struct exec_context *curr = get_current_ctx();
    curr->alarm_config_time = ticks;
    curr->ticks_to_alarm = ticks;
    // printf(" ticks to alarm in the handler = = %x\n", curr->ticks_to_alarm);
}
