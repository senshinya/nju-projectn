/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  uint64_t n = 1;
  char *arg = strtok(NULL, " ");
  if (arg != NULL) {
    /* specify the number of instructions */
    n = strtol(arg, NULL, 10);
    if (n == 0) n = 1;
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info r|w\n");
  } else if (strcmp(arg, "r") == 0) {
    // print register statement
    isa_reg_display();
  } else if (strcmp(arg, "w") == 0) {
    // TODO implement watchpoint information
  } else {
    printf("Usage: info r|w\n");
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  int n = strtol(arg, NULL, 10);
  arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  bool success = true;
  word_t result = expr(arg, &success);
  int *mem = (int *)guest_to_host(result);
  for (int i = 0; i < n; i ++) {
    printf("0x%08x: %08x\n", result+i*4, mem[i]);
  }
  return 0;
}

static int cmd_p(char *args) {
  bool success = true;
  word_t result = expr(args, &success);
  if (!success) {
    printf("Invalid expression\n");
    return 0;
  }
  printf("0x%08x\n", result);
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step through N(default 1) instruction(s)", cmd_si },
  { "info", "Print register state(info r) or watchpoint information(info w)", cmd_info },
  { "x", "x N EXPR: Print N 4-byte values after EXPR", cmd_x },
  { "p", "p EXPR: Calculate and print the value of EXPR", cmd_p },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr() {
  FILE *fp = fopen("/home/shinya/ics2024/nemu/tools/gen-expr/build/input", "r");
  if (fp == NULL)
    perror("test_expr error");
  
  word_t correct;
  char exp[1024];
  while (fscanf(fp, "%u", &correct) != EOF) {
    if(fgets(exp, 1024, fp) == NULL) break;
    // remove the '\n'
    size_t len = strlen(exp);
    if (len > 0 && exp[len - 1] == '\n') {
        exp[len - 1] = '\0';
    }
    bool success = true;
    word_t res = expr(exp, &success);
    if (res != correct || !success) {
      puts(exp);
      printf("expected: %u, got: %u\n", correct, res);
      assert(0);
    }
  }

  fclose(fp);

  Log("pass expr test!");
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Test decimal expression*/
  test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
