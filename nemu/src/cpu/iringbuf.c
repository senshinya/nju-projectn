#include <common.h>
#include <memory/vaddr.h>

#define IRINGBUF_SIZE 9

struct iringbuf_node {
    word_t pc;
    word_t inst;
};

struct iringbuf {
    int next;
    bool full;
    struct iringbuf_node nodes[IRINGBUF_SIZE];
} iringbuf = {
    .next = 0,
    .full = false,
};

void iringbuf_push(word_t pc, word_t inst) {
    iringbuf.nodes[iringbuf.next].pc = pc;
    iringbuf.nodes[iringbuf.next].inst = inst;
    iringbuf.next = (iringbuf.next + 1) % IRINGBUF_SIZE;
    if (!iringbuf.full && iringbuf.next == 0) iringbuf.full = true;
}

void iringbuf_display() {
    if (!iringbuf.full && iringbuf.next == 0) return; // empty
#ifdef CONFIG_ITRACE
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

    char buf[128];  // line buffer
    char *p;

    // print previous and current
    int i = iringbuf.full?iringbuf.next:0;
    int end = iringbuf.next;    // not included
    do {
        p = buf;
        p += sprintf(p, "%s" FMT_WORD ": ", (i+1)%IRINGBUF_SIZE==end?" --> ":"     ", iringbuf.nodes[i].pc);
        disassemble(p, buf+sizeof(buf)-p, iringbuf.nodes[i].pc, (uint8_t *)&iringbuf.nodes[i].inst, 4);
        puts(buf);
        i = (i + 1) % IRINGBUF_SIZE;
    } while (i != end);

    // print next 8 insts
    int current = iringbuf.nodes[(IRINGBUF_SIZE+iringbuf.next-1)%IRINGBUF_SIZE].pc;
    for (i = 1; i <= 8; i ++) {
        p = buf;
        word_t inst_addr = current + i * sizeof(word_t);
        word_t inst = vaddr_ifetch(inst_addr, sizeof(word_t));
        p += sprintf(p, "     " FMT_WORD ": ", inst_addr);
        disassemble(p, buf+sizeof(buf)-p, inst_addr, (uint8_t *)&inst, 4);
        puts(buf);
    }
#endif
}