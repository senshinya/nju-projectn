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
  Elf_Ehdr ehdr;
  fs_read(fd, &ehdr, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  Elf_Phdr phdrs[ehdr.e_phnum];
  fs_lseek(fd, ehdr.e_phoff, 0);
  fs_read(fd, (void *)phdrs, sizeof(Elf_Phdr) * ehdr.e_phnum);
  for (int i = 0; i < ehdr.e_phnum; i ++) {
    if (phdrs[i].p_type != PT_LOAD) {
      continue;
    }
    Elf_Phdr phdr = phdrs[i];
    uint8_t buf[phdr.p_filesz];
    fs_lseek(fd, phdr.p_offset, 0);
    fs_read(fd, (void *)buf, phdr.p_filesz);
    memcpy((void *)phdr.p_vaddr, buf, phdr.p_filesz);
    if (phdr.p_filesz < phdr.p_memsz) {
      memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz-phdr.p_filesz);
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

