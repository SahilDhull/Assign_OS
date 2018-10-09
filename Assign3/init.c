#include<init.h>
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

static void func(){
  write("System Call Handler Implemented\n",5);
  // exit(0); 
  //return 0;
}

static int main()
{
  // int a =1;  
  // signal(SIGFPE,&func);
  // write((char*)&func,8);
  // int x = 1/0;
  signal(SIGALRM,&func);
  alarm(4);
  // for(int i=0;i<70000;i++);
  while(1);
  // write("reached ending-----------\n",20);
  return 0;
}
