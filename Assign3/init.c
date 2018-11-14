#include<init.h>
#include<memory.h>
static void exit(int);
static int main(void);


void init_start()
{
  int retval = main();
  exit(0);
}

/*Invoke system call with no additional arguments*/
static long _syscall0(int syscall_num)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke system call with one argument*/

static long _syscall1(int syscall_num, int exit_code)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}
/*Invoke system call with two arguments*/

static long _syscall2(int syscall_num, u64 arg1, u64 arg2)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}


static void exit(int code)
{
  _syscall1(SYSCALL_EXIT, code); 
}

static long getpid()
{
  return(_syscall0(SYSCALL_GETPID));
}

static long write(char *ptr, int size)
{
   return(_syscall2(SYSCALL_WRITE, (u64)ptr, size));
}

static void *expand(unsigned size, int flags) {
  return (void *)_syscall2(SYSCALL_EXPAND, size, flags);
}

static void signal(u64 signo, void* handler){
  // write("printing handler address==========\n",40);
  // write((char*)handler,8);
  _syscall2(SYSCALL_SIGNAL,signo,(long)handler);
}

static void alarm(int ticks){
  // write((char*)ticks,8);
  _syscall1(SYSCALL_ALARM,(u32)ticks);
}

static void sleep(int ticks){
  // write((char*)ticks,8);
  _syscall1(SYSCALL_SLEEP,(u32)ticks);
}

static void clone(void* func_addr,void* st_addr)
{
  // write("func address in clone call in init = ",30);
  // write((char*)func_addr,8);
  _syscall2(SYSCALL_CLONE, (long)func_addr,(long)st_addr) ;
}

static void func(){
  write("System Call Handler Implemented\n",5);
  // exit(0); 
  //return 0;
}

static void f(){
  write("-------inside pid 2--------\n",30);
  // char * ptr = expand(10,MAP_WR);
  // clone(&f1,ptr);
  // write("----exit with code 2----------\n",30);
  exit(2);
}

static void f1(){
  write("-------inside pid 3--------\n",30);
  // char * ptr = expand(10,MAP_WR);
  // clone(&f1,ptr);
  // write("----exit with code 3----------\n",30);
  exit(3);
}

static void f2(){
  write("-------inside pid 4--------\n",30);
  // char * ptr = expand(10,MAP_WR);
  // clone(&f1,ptr);
  // write("----exit with code 5----------\n",30);
  exit(4);
}


static int main()
{
  // int a =1;  
  // signal(SIGFPE,&func);
  // write((char*)&func,8);
  // int x = 1/0;
  // signal(SIGALRM,&func);
  // alarm(3);
  // write("doing sleep in next step\n",30);
  // sleep(5);
  // write("okay=================\n",30);
  char * ptr = expand(10,MAP_WR);
  // char * ptr2 = expand(10,MAP_WR);
  clone(&f,ptr);
  // write("\ncoming here\n",15);
  char * ptr1 = expand(10,MAP_WR);
  clone(&f1,ptr1);
  // clone(&f2,ptr2);
  // for(int i=0;i<70000;i++);
  // while(1);
  // write("reached ending-----------\n",20);
  exit(1);
  return 0;
}
