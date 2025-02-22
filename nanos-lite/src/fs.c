#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
extern size_t serial_write(const void *buf, size_t offset, size_t len);
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, serial_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < sizeof(file_table)/sizeof(Finfo); i ++) {
    if (strcmp(file_table[i].name, pathname) != 0) continue;
    file_table[i].open_offset = 0;
    return i;
  }
  panic("file %s not found!", pathname);
  return -1;
}

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0) {
    return -1;
  }
  size_t nread = 0;
  if (file_table[fd].read == NULL) {
    nread = ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  } else {
    nread = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  }
  if (fd != FD_STDIN) {
    file_table[fd].open_offset = file_table[fd].open_offset + nread > file_table[fd].size ? file_table[fd].size : file_table[fd].open_offset + nread;
  }
  return nread;
}

extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0) {
    return -1;
  }
  size_t nwrite = 0;
  if (file_table[fd].write == NULL) {
    nwrite = ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  } else {
    nwrite = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  }
  if (fd != FD_STDOUT && fd != FD_STDERR) {
    file_table[fd].open_offset = file_table[fd].open_offset + nwrite > file_table[fd].size ? file_table[fd].size : file_table[fd].open_offset + nwrite;
  }
  return nwrite;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0) {
    return -1;
  }
  switch (whence) {
    case SEEK_SET: file_table[fd].open_offset = offset; break;
    case SEEK_CUR: file_table[fd].open_offset += offset; break;
    case SEEK_END: file_table[fd].open_offset = file_table[fd].size + offset; break;
    default: assert(0);
  }
  if (file_table[fd].open_offset < 0 || file_table[fd].open_offset > file_table[fd].size) {
    Log("fs_lseek: offset out of range");
    return -1;
  }
  return file_table[fd].open_offset >= 0 ? file_table[fd].open_offset : -1;
}

int fs_close(int fd) {
  return 0;
}