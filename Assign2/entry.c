/*System Call handler*/
#include<init.h>
#include<lib.h>
#include<context.h>
#include<memory.h>

u64 l4=0xFF8000000000,l3=0x7FC0000000,l2=0x3FE00000,l1=0x1FF000;
u64 f=0xFFFFFFFFFF000;

int check(u64 addr,u64* k4){
    u64 pos[4]={(addr&l4)>>39,(addr&l3)>>30,(addr&l2)>>21,(addr&l1)>>12};
    u32 address,i;
    u64 *k[4];
    k[0]=k4;
    u64 val;
    for(i=0;i<4;i++){
        val=*(k[i]+pos[i]);
        if((val&0x1)==0x0) return -1; //case of invalidity
        if(i==3) break;
        val = (val&f)>>12;
        address=(u32)val;
        k[i+1]=osmap(address);
    }
    return 0;
}

void free_page(u64 addr,u64* k4){
    u64 pos[4]={(addr&l4)>>39,(addr&l3)>>30,(addr&l2)>>21,(addr&l1)>>12};
    u32 address,i;
    u64 *k[4];
    k[0]=k4;
    u64 val;
    for(i=0;i<4;i++){
        val=*(k[i]+pos[i]);
        // printf("val = %x\n", val);
        if((val&0x1)==0x0) break;
        if(i==3){
            // if(val&0x1==0x0) break;
            // printf("wrong\n" );
            val = (val&f)>>12;
            address=(u32)val;
            os_pfn_free(USER_REG,address);
            *(k[3]+pos[3]) = 0x0;
            break;
        }
        val = (val&f)>>12;
        address=(u32)val;
        k[i+1]=osmap(address);
    }
}

unsigned long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
{
    struct exec_context *current = get_current_ctx();
    printf("[GemOS] System call invoked. syscall no  = %d\n", syscall);
    switch(syscall)
    {
          case SYSCALL_EXIT:
                              printf("[GemOS] exit code = %d\n", (int) param1);
                              do_exit();
                              break;
          case SYSCALL_GETPID:
                              printf("[GemOS] getpid called for process %s, with pid = %d\n", current->name, current->id);
                              return current->id;
          case SYSCALL_WRITE:
                             {
                                     /*Your code goes here*/
                                     char *buf = (char*) param1;
                                     int length = (int) param2;
                                     if(length<0 || length >1024) return -1;
                                     //to check the availability of buf address by accessing init process page table
                                     u64* k = osmap(current->pgd);
                                     u64 addr = (u64) buf;
                                     if(check(addr,k)==-1||check(addr+length-1,k)==-1)
                                        return -1;
                                     // buf[length]='\0';
                                     // printf("%d %s",length,buf);

                                     int i;
                                     for(i=0;i<length;i++){
                                         printf("%c",*(buf+i));
                                     }
                                     // printf("check\n" );
                                     return (u64)length;
                             }
          case SYSCALL_EXPAND:
                             {
                                     /*Your code goes here*/
                                     u32 size = (u32) param1;
                                     int flags = (int) param2;
                                     // printf("%x, MAP_RD = %d, MAP_WR = %d\n", param2, MAP_RD,MAP_WR);
                                     if(size>512) return (long)NULL;
                                     struct mm_segment curr;
                                     u64 ret_val,seg;
                                     if(flags==MAP_RD){
                                         seg = (u64) MM_SEG_RODATA;
                                     }
                                     else if(flags==MAP_WR){
                                         seg = (u64) MM_SEG_DATA;
                                     }
                                     else return (long)NULL;
                                     curr=current->mms[seg];
                                     ret_val = curr.next_free;
                                     u64 l_size = (u64) size<<12;
                                     if(curr.next_free+l_size > curr.end + 1)
                                        return (long)NULL;
                                     current->mms[seg].next_free = curr.next_free + l_size;
                                     // printf("seg = %d  next_free = %x\n",seg, current->mms[seg].next_free );
                                     return (long)ret_val;
                             }
          case SYSCALL_SHRINK:
                             {
                                     /*Your code goes here*/
                                     u32 size = (u32) param1;
                                     int flags = (int) param2;
                                     if(size>512) return (long)NULL;
                                     struct mm_segment curr;
                                     u64 seg;
                                     if(flags==MAP_RD){
                                         seg = MM_SEG_RODATA;
                                     }
                                     else if(flags==MAP_WR){
                                         seg = MM_SEG_DATA;
                                     }
                                     else return (long)NULL;
                                     curr=current->mms[seg];
                                     u64 l_size = (u64) size<<12;
                                     if(curr.next_free-l_size < curr.start)
                                        return (long)NULL;
                                     u64 i = curr.next_free-l_size;
                                     // printf("i = %x\n", i);
                                     u64* k = osmap(current->pgd);
                                     while(i<curr.next_free){
                                         // printf("%x\n", i);
                                         free_page(i,k);
                                         i=i+(0x1<<12);
                                     }
                                     current->mms[seg].next_free = curr.next_free - l_size;
                                     return (long)current->mms[seg].next_free;
                             }

          default:
                              return -1;

    }
    return 0;   /*GCC shut up!*/
}


extern int handle_div_by_zero(void)
{
    printf("Divide by Zero\n");
    /*Your code goes in here*/
    struct exec_context *current = get_current_ctx();
    u64 *val;
    //access the rsp register using inline
    asm volatile ( "mov %%rbp, %0;"
                    : "=r" (val)
                    ::);
    printf("Div-by-zero detected at %x\n",*(val+1));
    do_exit();
    return 0;
}

void allocate(u64 addr,u64* k4,u32 per){
    u64 *k[4];
    k[0]=k4;
    u64 pos[4]={(addr&l4)>>39,(addr&l3)>>30,(addr&l2)>>21,(addr&l1)>>12};
    u32 address,i;
    u64 val;
    for(i=0;i<3;i++){
        val=*(k[i]+pos[i]);
        if(val!=0x0){
            val = (val&f)>>12;
            address=(u32)val;
            k[i+1]=osmap(address);
        }
        else{
            address=os_pfn_alloc(OS_PT_REG);
            k[i+1]=osmap(address);
            for(u32 j=0x0;j<0x1000;j+=0x4){
                *(k[i+1]+j)=0x0;
            }
            *(k[i]+pos[i]) = ((address<<12) & f) | per | 0x4;
        }
    }
    address=os_pfn_alloc(USER_REG);
    u32* x=osmap(address);
    for(i=0x0;i<0x1000;i+=0x4){
        *(x+i)=0x0;
    }
    *(k[3]+pos[3]) = ((address<<12) & f) | per | 0x4;
}

extern int handle_page_fault(void)
{
    asm volatile (
        "push %rax;"
        "push %rbx;"
        "push %rcx;"
        "push %rdx;"
        "push %rsi;"
        "push %rdi;"
        "push %r8;"
        "push %r9;"
        "push %r10;"
        "push %r11;"
        "push %r12;"
        "push %r13;"
        "push %r14;"
        "push %r15;"
    );
    /*Your code goes in here*/
    struct exec_context *current = get_current_ctx();
    u64* k = osmap(current->pgd);
    u64 cr2,rip,err;
    u64 *ptr;
    asm volatile ( "mov %%cr2, %0;"
                    : "=r" (cr2)
                    :
                    :);
    asm volatile ( "mov %%rbp, %0;"
                    : "=r" (ptr)
                    :
                    :);
    err = *(ptr+1);
    rip = *(ptr+2);
    struct mm_segment data,rodata,stack;
    data = current->mms[MM_SEG_DATA];
    rodata = current->mms[MM_SEG_RODATA];
    stack = current->mms[MM_SEG_STACK];
    // printf("cr2 = %x, rip = %x, error = %x\n",cr2 ,rip,err);
    if(cr2>=data.start && cr2<data.end){
        if(cr2<data.next_free){
            allocate(cr2,k,0x3);
        }
        else{
            printf("Error (in data segment) at VA (cr2) = %x with RIP = %x, error code = %x\n",cr2,rip,err );
            do_exit();
        }
    }
    else if(cr2>=rodata.start && cr2<rodata.end){
        u64 w = (err&0x2)>>1;
        if(w==0x1){
            printf("Access error (in RO data segment) at VA (cr2) = %x with RIP = %x, error code = %x\n",cr2,rip,err );
            do_exit();
        }
        else if(cr2<rodata.next_free){
            allocate(cr2,k,0x1);
        }
        else{
            printf("Error (in RO data segment) at VA (cr2) = %x with RIP = %x, error code = %x\n",cr2,rip,err );
            do_exit();
        }
    }
    else if(cr2>=stack.start && cr2<stack.end){
        allocate(cr2,k,0x3);
    }
    else{
        printf("Error at VA (cr2) = %x with RIP = %x, error code = %x\n",cr2,rip,err );
        do_exit();
    }
    //point rsp to rip
    asm volatile (
        "pop %r15;"
        "pop %r14;"
        "pop %r13;"
        "pop %r12;"
        "pop %r11;"
        "pop %r10;"
        "pop %r9;"
        "pop %r8;"
        "pop %rdi;"
        "pop %rsi;"
        "pop %rdx;"
        "pop %rcx;"
        "pop %rbx;"
        "pop %rax;"
    );
    asm volatile (
        "mov %rbp, %rsp;"
        "pop %rbp;"
        "add $8, %rsp;"
        "iretq;"
    );
    return 0;
}
