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
        if(val&0x1==0x0) return -1; //case of invalidity
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
        if(val&0x1==0x0) break;
        if(i==3){
            // if(val&0x1==0x0) break;
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

long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
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
                                     buf[length]='\0';
                                     printf("%s",buf);
                                     /*
                                     int i;
                                     for(i=0;i<length;i++){
                                         printf("%c",*(buf+i));
                                     }
                                     */
                                     return (long)length;
                             }
          case SYSCALL_EXPAND:
                             {
                                     /*Your code goes here*/
                                     u32 size = (u32) param1;
                                     int flags = (int) param2;
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
                                     u64 l_size = (u64) size;
                                     if(curr.next_free+l_size > curr.end + 1)
                                        return (long)NULL;
                                     current->mms[seg].next_free = curr.next_free + l_size;
                                     printf("seg = %d  next_free = %x\n",seg, current->mms[seg].next_free );
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
                                     u64 l_size = (u64) size;
                                     if(curr.next_free-l_size < curr.start)
                                        return (long)NULL;
                                     // to free data pages and update pte
                                     u64 i = curr.next_free-l_size;
                                     u64* k = osmap(current->pgd);
                                     while(i<curr.next_free){
                                         // u64 addr = i<<12;
                                         printf("%x\n", i);
                                         free_page(i,k);
                                         i+=1;
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
    /*Your code goes in here*/
    struct exec_context *current = get_current_ctx();
    u64 *val;
    //access the rsp register using inline
    asm volatile ( "mov %rsp, %0;"
                    : "=r" (val)
                    ::);
    printf("Div-by-zero detected at %x\n",*(val+1));
    do_exit();
    return 0;
}

extern int handle_page_fault(void)
{
    /*Your code goes in here*/
    struct exec_context *current = get_current_ctx();
    u64 cr2,rip,err;
    u64 *ptr;
    asm volatile ( "mov %%cr2, %0;"
                    : "=r" (cr2)
                    :
                    :);
    asm volatile ( "mov %rsp, %0;"
                    : "=r" (ptr)
                    :
                    :);
    err = *(ptr+1);
    rip = *(ptr+2);
    struct mm_segment data,rodata,stack;
    data = current->mms[MM_SEG_DATA];
    rodata = current->mms[MM_SEG_RODATA];
    stack = current->mms[MM_SEG_STACK];
    if(cr2>=data.start && cr2<data.end){
        if(cr2>=data.start && cr2<data.next_free){
            
        }
        else{
            printf("Error (in data segment) at VA = %x with error code = %x\n",rip,err );
            do_exit();
        }
    }
    // printf("page fault handler: unimplemented!\n");
    return 0;
}
