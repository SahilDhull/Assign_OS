#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>
#include<init.h>
#include<types.h>
static u64 numticks;


void handle_timer_tick()
{
    u64* rsp;
    asm volatile (
        "push %rax; push %rbx; push %rcx; push %rdx; push %rsi; push %rdi; push %r8;"
        "push %r9; push %r10; push %r11; push %r12; push %r13; push %r14; push %r15;"
    );
    asm volatile( "mov %%rsp, %0" : "=r"(rsp) );
    // asm volatile("cli;"
    //               :::"memory");
    printf("Got a tick. #ticks = %u\n", numticks++);   //XXX Do not modify this line

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
    struct exec_context *next;
    int ready = 0;
    for(int i=1;i<16;i++){
        int ind;
        if(((i+curr->pid)%16)==0) continue;
        if(i+curr->pid>16) ind = (i+curr->pid)%16;
        else ind = i+curr->pid;
        next = &(list[ind]);
        if(next->state == READY){
            curr->state = READY;
            ready = 1;
            break;
        }
    }
    if(ready==1){
        curr->regs.r15 = *rsp;
        curr->regs.r14 = *(rsp+1);
        curr->regs.r13 = *(rsp+2);
        curr->regs.r12 = *(rsp+3);
        curr->regs.r11 = *(rsp+4);
        curr->regs.r10 = *(rsp+5);
        curr->regs.r9 = *(rsp+6);
        curr->regs.r8 = *(rsp+7);
        curr->regs.rdi = *(rsp+8);
        curr->regs.rsi = *(rsp+9);
        curr->regs.rdx = *(rsp+10);
        curr->regs.rcx = *(rsp+11);
        curr->regs.rbx = *(rsp+12);
        curr->regs.rax = *(rsp+13);

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
        printf("scheduling: old pid = %d new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove
 
        set_tss_stack_ptr(next);
        set_current_ctx(next);
        ack_irq();

        *ptr = next->regs.rbp;
        *(ptr+1) = next->regs.entry_rip;
        *(ptr+2) = next->regs.entry_cs;
        *(ptr+3) = next->regs.entry_rflags;
        *(ptr+4) = next->regs.entry_rsp;
        *(ptr+5) = next->regs.entry_ss;
        next->state = RUNNING; 

        asm volatile ("mov %0,%%r15;" ::"r"(next->regs.r15));
        asm volatile ("mov %0,%%r14;" ::"r"(next->regs.r14));
        asm volatile ("mov %0,%%r13;" ::"r"(next->regs.r13));
        asm volatile ("mov %0,%%r12;" ::"r"(next->regs.r12));
        asm volatile ("mov %0,%%r11;" ::"r"(next->regs.r11));
        asm volatile ("mov %0,%%r10;" ::"r"(next->regs.r10));
        asm volatile ("mov %0,%%r9 ;" ::"r"(next->regs.r9 ));
        asm volatile ("mov %0,%%r8 ;" ::"r"(next->regs.r8 ));
        asm volatile ("mov %0,%%rdi;" ::"r"(next->regs.rdi));
        asm volatile ("mov %0,%%rsi;" ::"r"(next->regs.rsi));
        asm volatile ("mov %0,%%rdx;" ::"r"(next->regs.rdx));
        asm volatile ("mov %0,%%rcx;" ::"r"(next->regs.rcx));
        asm volatile ("mov %0,%%rbx;" ::"r"(next->regs.rbx));
        asm volatile ("mov %0,%%rax;" ::"r"(next->regs.rax));

        asm volatile("mov %%rbp, %%rsp;"
        "pop %%rbp;"
        "iretq;"
        :::"memory");
    }
    if(curr->ticks_to_alarm == 0){
        curr->ticks_to_alarm = curr->alarm_config_time;
        invoke_sync_signal(SIGALRM,ptr+4 , ptr+1);
    }
    curr->ticks_to_alarm--;
    ack_irq(); 
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
    
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    struct exec_context *curr = get_current_ctx();
    curr->state = UNUSED;
    struct exec_context *list = get_ctx_list();
    int ready = 0, wait = 0;
    struct exec_context * next;
    for(int i=1;i<16;i++){
        int ind ;
        if(((i+curr->pid)%16)==0) continue;
        ind = (i+curr->pid)%16;
        next = &(list[ind]);
        if(next->state == READY){
            ready = 1;
            
            printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove
            next->state = RUNNING;
            os_pfn_free(OS_PT_REG,curr->os_stack_pfn);
            set_tss_stack_ptr(next);
            set_current_ctx(next);
            asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
            *ptr = next->regs.rbp;
            *(ptr+1) = next->regs.entry_rip;
            *(ptr+2) = next->regs.entry_cs;
            *(ptr+3) = next->regs.entry_rflags;
            *(ptr+4) = next->regs.entry_rsp;
            *(ptr+5) = next->regs.entry_ss;

            asm volatile ("mov %0,%%r15;" ::"r"(next->regs.r15));
            asm volatile ("mov %0,%%r14;" ::"r"(next->regs.r14));
            asm volatile ("mov %0,%%r13;" ::"r"(next->regs.r13));
            asm volatile ("mov %0,%%r12;" ::"r"(next->regs.r12));
            asm volatile ("mov %0,%%r11;" ::"r"(next->regs.r11));
            asm volatile ("mov %0,%%r10;" ::"r"(next->regs.r10));
            asm volatile ("mov %0,%%r9 ;" ::"r"(next->regs.r9 ));
            asm volatile ("mov %0,%%r8 ;" ::"r"(next->regs.r8 ));
            asm volatile ("mov %0,%%rdi;" ::"r"(next->regs.rdi));
            asm volatile ("mov %0,%%rsi;" ::"r"(next->regs.rsi));
            asm volatile ("mov %0,%%rdx;" ::"r"(next->regs.rdx));
            asm volatile ("mov %0,%%rcx;" ::"r"(next->regs.rcx));
            asm volatile ("mov %0,%%rbx;" ::"r"(next->regs.rbx));
            asm volatile ("mov %0,%%rax;" ::"r"(next->regs.rax));

            asm volatile("mov %%rbp, %%rsp;"
            "pop %%rbp;"
            "iretq;"
            :::"memory");
            // */
            break;
        }
        if(next->state == WAITING) wait=1;
    }
    if(ready==0 && wait==1){
        next = &(list[0]);
        printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove
        next->state = RUNNING;
        os_pfn_free(OS_PT_REG,curr->os_stack_pfn);
        set_tss_stack_ptr(next);
        set_current_ctx(next);
        *ptr = next->regs.rbp;
        *(ptr+1) = next->regs.entry_rip;
        *(ptr+2) = next->regs.entry_cs;
        *(ptr+3) = next->regs.entry_rflags;
        *(ptr+4) = next->regs.entry_rsp;
        *(ptr+5) = next->regs.entry_ss;

        
        asm volatile ("mov %0,%%r15;" ::"r"(next->regs.r15));
        asm volatile ("mov %0,%%r14;" ::"r"(next->regs.r14));
        asm volatile ("mov %0,%%r13;" ::"r"(next->regs.r13));
        asm volatile ("mov %0,%%r12;" ::"r"(next->regs.r12));
        asm volatile ("mov %0,%%r11;" ::"r"(next->regs.r11));
        asm volatile ("mov %0,%%r10;" ::"r"(next->regs.r10));
        asm volatile ("mov %0,%%r9 ;" ::"r"(next->regs.r9 ));
        asm volatile ("mov %0,%%r8 ;" ::"r"(next->regs.r8 ));
        asm volatile ("mov %0,%%rdi;" ::"r"(next->regs.rdi));
        asm volatile ("mov %0,%%rsi;" ::"r"(next->regs.rsi));
        asm volatile ("mov %0,%%rdx;" ::"r"(next->regs.rdx));
        asm volatile ("mov %0,%%rcx;" ::"r"(next->regs.rcx));
        asm volatile ("mov %0,%%rbx;" ::"r"(next->regs.rbx));
        asm volatile ("mov %0,%%rax;" ::"r"(next->regs.rax));

        asm volatile("mov %%rbp, %%rsp;"
        "pop %%rbp;"
        "iretq;"
        :::"memory");
    }
    else if(ready==0 && wait==0){
        do_cleanup();
    }
}


long do_sleep(u32 ticks)
{
    struct exec_context *curr = get_current_ctx();
    curr->ticks_to_sleep = ticks;
    curr->state = WAITING;

    u64* addr = (u64*)((((u64)curr->os_stack_pfn+1)<<12)-8);
    curr->regs.entry_rflags = *(addr-2);
    curr->regs.entry_rsp = *(addr-1);
    curr->regs.entry_cs = 0x23;
    curr->regs.entry_ss = 0x2b;
    curr->regs.rbp = *(addr-10);
    curr->regs.entry_rip = *(addr-4);

    struct exec_context *list = get_ctx_list();
    struct exec_context *next;
    int ready = 0;
    for(int i=1;i<16;i++){
        int ind;
        if(((i+curr->pid)%16)==0) continue;
        if(i+curr->pid>16) ind = (i+curr->pid)%16;
        else ind = i+curr->pid;
        next = &(list[ind]);
        if(next->state == READY){
            ready = 1;
            break;
        }
    }


    if(ready == 0) next = get_ctx_by_pid(0);
    
    printf("scheduling: old pid = %d  new pid = %d\n", curr->pid, next->pid); //XXX: Don't remove
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    *ptr = next->regs.rbp;
    *(ptr+1) = next->regs.entry_rip;
    *(ptr+2) = next->regs.entry_cs;
    *(ptr+3) = next->regs.entry_rflags;
    *(ptr+4) = next->regs.entry_rsp;
    *(ptr+5) = next->regs.entry_ss;
    set_current_ctx(next);
    set_tss_stack_ptr(next);
    next->state=RUNNING;
    asm volatile ("mov %0,%%r15;" ::"r"(next->regs.r15));
    asm volatile ("mov %0,%%r14;" ::"r"(next->regs.r14));
    asm volatile ("mov %0,%%r13;" ::"r"(next->regs.r13));
    asm volatile ("mov %0,%%r12;" ::"r"(next->regs.r12));
    asm volatile ("mov %0,%%r11;" ::"r"(next->regs.r11));
    asm volatile ("mov %0,%%r10;" ::"r"(next->regs.r10));
    asm volatile ("mov %0,%%r9 ;" ::"r"(next->regs.r9 ));
    asm volatile ("mov %0,%%r8 ;" ::"r"(next->regs.r8 ));
    asm volatile ("mov %0,%%rdi;" ::"r"(next->regs.rdi));
    asm volatile ("mov %0,%%rsi;" ::"r"(next->regs.rsi));
    asm volatile ("mov %0,%%rdx;" ::"r"(next->regs.rdx));
    asm volatile ("mov %0,%%rcx;" ::"r"(next->regs.rcx));
    asm volatile ("mov %0,%%rbx;" ::"r"(next->regs.rbx));
    asm volatile ("mov %0,%%rax;" ::"r"(next->regs.rax));
    
    asm volatile("mov %%rbp, %%rsp;"
    "pop %%rbp;"
    "iretq;"
    :::"memory");
}


long do_clone(void *th_func, void *user_stack)
{
    u64* ptr;
    asm volatile ( "mov %%rbp, %0;" : "=r" (ptr));
    struct exec_context *new = get_new_ctx();
    struct exec_context *curr = get_current_ctx();
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

    new->state = READY;
}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip){
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
    }
}
/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
    struct exec_context *curr = get_current_ctx();
    (curr->sighandlers)[signo] = (void*)handler;
}
/*system call handler for alarm*/
long do_alarm(u32 ticks)
{
    struct exec_context *curr = get_current_ctx();
    curr->alarm_config_time = ticks;
    curr->ticks_to_alarm = ticks;
}
