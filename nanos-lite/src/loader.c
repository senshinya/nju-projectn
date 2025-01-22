#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif


static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  if (fd == -1) {
      printf("Failed to open file %s\n", filename);
      assert(0);
  }
  Elf_Ehdr ehdr;
  if (fs_read(fd, &ehdr, sizeof(Elf_Ehdr)) == 0) {
      printf("Failed to read ELF header\n");
      assert(0);
  }
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
      printf("Not an ELF file\n");
      assert(0);
  }
  Elf_Phdr phdr[ehdr.e_phnum];
  fs_lseek(fd, ehdr.e_phoff, SEEK_SET);
  if (fs_read(fd, phdr, ehdr.e_phnum * sizeof(Elf_Phdr)) == 0) {
      printf("Failed to read program headers\n");
      assert(0);
  }
  for (int i = 0; i < ehdr.e_phnum; i++) {
      if (phdr[i].p_type == PT_LOAD) {
          fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
          if (fs_read(fd, (void *)phdr[i].p_vaddr, phdr[i].p_filesz) == 0) {
              printf("Failed to read segment %d\n", i);
              assert(0);
          }
          if (phdr[i].p_filesz < phdr[i].p_memsz) {
              memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
          }
      }
  }
  fs_close(fd);
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

