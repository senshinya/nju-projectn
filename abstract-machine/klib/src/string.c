#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t res = 0;
  while (s[res] != '\0') res ++;
  return res;
}

char *strcpy(char *dst, const char *src) {
  int i;
  for (i = 0; src[i] != '\0'; i++) dst[i] = src[i];
  dst[i] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  int i;
  for (i = 0; src[i] != '\0' && i < n; i++) dst[i] = src[i];
  for (; i < n; i++) dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  int i, j;
  for (i = 0; dst[i] != '\0'; i++) {}
  for (j = 0; src[j] != '\0'; j++, i++) dst[i] = src[j];
  dst[i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  int i;
  for (i = 0; s1[i] == s2[i] && s1[i] != '\0'; i++) {}
  return s1[i] - s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int i;
  for (i = 0; s1[i] == s2[i] && s1[i] != '\0' && i < n; i++) {}
  return s1[i] - s2[i];
}

void *memset(void *s, int c, size_t n) {
  unsigned char* target = (unsigned char*)s;
  for (int i = 0; i < n; ++i){
    target[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *cdst = (char*)dst;
  char *csrc = (char*)src;
  if (cdst < csrc) {
    for (int i = 0; i < n; i++) {
      cdst[i] = csrc[i];
    }
    return dst;
  }
  if (cdst > csrc) {
    for (int i = n - 1; i >= 0; --i) {
      cdst[i] = csrc[i];
    }
  }
  return dst;
}


void *memcpy(void *out, const void *in, size_t n) {
  char *cout = (char*)out;
  char *cin = (char*)in;
  for (int i = 0; i < n; i++) {
    cout[i] = cin[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  char *cs1 = (char*)s1;
  char *cs2 = (char*)s2;
  for (int i = 0; i < n; i++) {
    if (cs1[i] != cs2[i]) {
      return cs1[i] - cs2[i];
    }
  }
  return 0;
}

#endif
