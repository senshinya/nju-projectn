#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

static void reverse(char *s, int len) {
  char *end = s + len - 1;
  char tmp;
  while (s < end) {
    tmp = *s;
    *s = *end;
    *end = tmp;
    s ++; end --;
  }
}

static int int2str(int n, char *s, int base) {
  int pos = n < 0 ? -n : n;
  int i = 0;
  do {
    int c = pos % base;
    if (c >= 10) s[i++] = 'a'+c-10;
    else s[i++] = '0'+c;
    pos /= base;
  } while (pos != 0);
  if (n < 0) s[i++] = '-';
  reverse(s, i);
  return i;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *p = out;
  const char *f = fmt;
  while (*f != '\0') {
    if (*f != '%') {
      *p = *f;
      p ++;
      f ++;
      continue;
    }
    f ++; // skip %
    switch (*f) {
      case 'd':
        int value = va_arg(ap, int);
        p += int2str(value, p, 10);
        break;
      case 's':
        const char *str = va_arg(ap, const char *);
        strcpy(p, str);
        p += strlen(str);
        break;
      case '%':
        *p = '%';
        p ++;
        break;
    }
    f ++;
  }
  *p = '\0';
  return p - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int res = vsprintf(out, fmt, ap);
  va_end(ap);
  return res;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
