#include <common.h>
#include <elf.h>

struct func_info {
    paddr_t entry;
    uint32_t size;
    char func_name[32];
    struct func_info *next;
};

struct func_info *func_list = NULL;

static void parse_func_table(Elf32_Ehdr *ehdr) {
    // section header table
    Elf32_Shdr *shdr = (Elf32_Shdr *)((uint8_t *)ehdr + ehdr->e_shoff);
    // find symbol table & string table
    Elf32_Shdr *symbol_table_section = NULL, *string_table_section = NULL;
    for (int i = 0; i < ehdr->e_shnum; i ++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            symbol_table_section = &shdr[i];
            string_table_section = &shdr[symbol_table_section->sh_link];
        }
    }
    Assert(symbol_table_section, "Can not find symbol table");
    Assert(string_table_section, "Can not find string table");

    Elf32_Sym *symbol_table = (Elf32_Sym *)((uint8_t *)ehdr + symbol_table_section->sh_offset);
    char *string_table = (char *)ehdr + string_table_section->sh_offset;

    for (int i = 0; i < symbol_table_section->sh_size / symbol_table_section->sh_entsize; i ++) {
        Elf32_Sym *symbol = &symbol_table[i];
        if (ELF32_ST_TYPE(symbol->st_info) != STT_FUNC) {
            continue;
        }
        struct func_info *func = malloc(sizeof(struct func_info));
        func->entry = symbol->st_value;
        func->size = symbol->st_size;
        strcpy(func->func_name, string_table + symbol->st_name);
        func->next = func_list;
        func_list = func;
    }
}

void load_elf(char *elf_file) {
    if (elf_file == NULL) {
        return;
    }

    FILE *fp = fopen(elf_file, "r");
    Assert(fp, "Can not open '%s'", elf_file);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    Log("The elf file is %s, size = %ld", elf_file, size);

    uint8_t *buf = malloc(size);
    Assert(buf, "malloc elf buffer error");

    int ret = fread(buf, 1, size, fp);
    assert(ret == size);
    fclose(fp);

    Elf32_Ehdr *elf = (Elf32_Ehdr *)buf;
    parse_func_table(elf);

    free(buf);
}

static char *get_func_name(paddr_t entry, bool range) {
    struct func_info *func = func_list;
    while (func) {
        if (!range && func->entry == entry) {
            return func->func_name;
        }
        if (range && func->entry <= entry && func->entry + func->size > entry) {
            return func->func_name;
        }
        func = func->next;
    }
    return "???";
}

int call_depth = 0;
void ftrace_call(paddr_t pc, paddr_t target) {
    if (func_list == NULL) return;

    call_depth ++;
    if (call_depth <= 2) return;    // ignore _trm_init and main

    char *func_name = get_func_name(target, false);
    log_write("[ftrace]" FMT_PADDR ": %*scall [%s@" FMT_PADDR "]\n",
		pc,
		(call_depth-3)*2, "",
		func_name,
		target
	);
}

void ftrace_ret(paddr_t pc) {
    if (func_list == NULL) return;

    if (call_depth <= 2) return;

    char *func_name = get_func_name(pc, true);
    log_write("[ftrace]" FMT_PADDR ": %*sret [%s]\n",
		pc,
		(call_depth-3)*2, "",
		func_name
	);

    call_depth --;
}