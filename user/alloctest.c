#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"

void
test0() {
  enum { NCHILD = 50, NFD = 10};
  int i, j;
  int fd;

  printf("filetest: start\n");
  
  if(NCHILD*NFD < NFILE) {
    printf("test setup is wrong\n");
    exit(1);
  }

  for (i = 0; i < NCHILD; i++) {
    int pid = fork();
    if(pid < 0){
      printf("fork failed");
      exit(1);
    }
    if(pid == 0){
      for(j = 0; j < NFD; j++) {
        if ((fd = open("README", O_RDONLY)) < 0) {
          // the open() failed; exit with -1
          exit(1);
        }
      }
      sleep(10);
      exit(0);  // no errors; exit with 0.
    }
  }

  int all_ok = 1;
  for(int i = 0; i < NCHILD; i++){
    int xstatus;
    wait(&xstatus);
    if(xstatus != 0) {
      if(all_ok == 1)
        printf("filetest: FAILED\n");
      all_ok = 0;
    }
  }

  if(all_ok)
    printf("filetest: OK\n");
}

// Allocate all free memory and count how it is
void test1()
{
  void *a;
  int tot = 0;
  char buf[1];
  int fds[2];
  
  printf("memtest: start\n");  
  if(pipe(fds) != 0){
    printf("pipe() failed\n");
    exit(1);
  }
  int pid = fork();
  if(pid < 0){
    printf("fork failed");
    exit(1);
  }
  if(pid == 0){
      close(fds[0]);
      while(1) {
        a = sbrk(PGSIZE);
        if (a == (char*)0xffffffffffffffffL)
          exit(0);
        *(int *)(a+4) = 1;
        if (write(fds[1], "x", 1) != 1) {
          printf("write failed");
          exit(1);
        }
      }
      exit(0);
  }
  close(fds[1]);
  while(1) {
      if (read(fds[0], buf, 1) != 1) {
        break;
      } else {
        tot += 1;
      }
  }
  //int n = (PHYSTOP-KERNBASE)/PGSIZE;
  //printf("allocated %d out of %d pages\n", tot, n);
  if(tot < 31950) {
    printf("expected to allocate at least 31950, only got %d\n", tot);
    printf("memtest: FAILED\n");  
  } else {
    printf("memtest: OK\n");  
  }
}

// allocate all mem, free it, and allocate again
void
test2()
{
  void *m1, *m2;
  int pid, xstatus;

  printf("memtest2: start\n");
  if((pid = fork()) == 0){
    m1 = 0;
    while((m2 = malloc(10001)) != 0){
      *(char**)m2 = m1;
      m1 = m2;
    }
    while(m1){
      m2 = *(char**)m1;
      free(m1);
      m1 = m2;
    }
    m1 = malloc(1024*20);
    if(m1 == 0){
      printf("couldn't allocate mem?!!\n");
      exit(1);
    }
    free(m1);
    exit(0);
  } else {
    wait(&xstatus);
    if(xstatus != 0)
      printf("memtest2: FAILED\n");
    else
      printf("memtest2: OK\n");
  }
}

void
test3()
{
  enum { TOOMUCH=1024*1024*1024};
  int i, pid, pid2, xstatus;
  char *c, *a, *b;

  printf("sbrktest: start\n");
  if((pid = fork()) == 0){
    // does sbrk() return the expected failure value?
    a = sbrk(TOOMUCH);
    if(a != (char*)0xffffffffffffffffL){
      printf("sbrk(<toomuch>) returned %p\n", a);
      exit(1);
    }

    // can one sbrk() less than a page?
    a = sbrk(0);
    for(i = 0; i < 5000; i++){
      b = sbrk(1);
      if(b != a){
        printf("sbrk test failed %d %x %x\n", i, a, b);
        exit(1);
      }
      *b = 1;
      a = b + 1;
    }
    pid2 = fork();
    if(pid2 < 0){
      printf("sbrk test fork failed\n");
      exit(1);
    }
    c = sbrk(1);
    c = sbrk(1);
    if(c != a + 1){
      printf("sbrk test failed post-fork\n");
      exit(1);
    }
  } else {
    wait(&xstatus);
    if(xstatus != 0)
      printf("sbrktest: FAILED\n");
    else
      printf("sbrktest: OK\n");
  }
}

int
main(int argc, char *argv[])
{
  test0();
  test1();
  test2();
  test3();
  exit(0);
}
