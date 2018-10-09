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
  write("adsfasd\n",12);
  write((char*)handler,8);
  _syscall2(SYSCALL_SIGNAL,signo,(long)handler);
}

void func(){
  write("okay\n",5);
  exit(0);
  //return 0;
}

static int main()
{
  int a =1;  
  signal(SIGFPE,&func);
  // write((char*)&func,8);
  // signal(SIGSEGV,&func);
  int p[1];
  p[2]=1;
  // signal(SIGALRM,&func);
  // int x = 1/0;

  // write("ok\n",3);
  
  // for(i=0; i<4096; ++i){
  //     j = buff[i];
  // }
  // i=0x100034;
  // j = i / (i-0x100034);
  // exit(-5);
  return 0;
}
