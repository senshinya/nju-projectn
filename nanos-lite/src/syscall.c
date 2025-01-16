#include <common.h>
#include "syscall.h"
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_exit: halt(c->GPR2); break;
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_write:
      int fd = c->GPR2;
      if (fd != 1 && fd != 2) {
        c->GPRx = -1;
        break;
      }
      char *buf = (char *)c->GPR3;
      int len = c->GPR4;
      for (int i = 0; i < len; i ++) putch(buf[i]);
      c->GPRx = len;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
