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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static char *buf_start = buf;
static char *buf_end = buf+sizeof(buf);

static void gen_char(char c) {
  int n_writes = snprintf(buf_start, buf_end-buf_start, "%c", c);
  if (buf_start < buf_end) {
    if (n_writes > 0) {
      buf_start += n_writes;
    }
  }
}

static void gen_space() {
  int size = rand() % 4;
  if (buf_start < buf_end) {
    int n_writes = snprintf(buf_start, buf_end-buf_start, "%*s", size, "");
    if (n_writes > 0) {
      buf_start += n_writes;
    }
  }
}

static void gen_num() {
  int num = rand() % INT8_MAX;
  if (buf_start < buf_end) {
    int n_writes = snprintf(buf_start, buf_end-buf_start, "%d", num);
    if (n_writes > 0) {
      buf_start += n_writes;
    }
  }
}

static void gen_rand_op() {
  int op = rand() % 4;
  switch (op) {
    case 0:
      gen_char('+');
      break;
    case 1:
      gen_char('-');
      break;
    case 2:
      gen_char('*');
      break;
    case 3:
      gen_char('/');
      break;
  }
}

static void gen_rand_expr(int* length) {
  gen_space();
  switch (rand() % 3) {
    case 0:
      gen_num(); (*length) ++;
      break;
    case 1:
      gen_char('('); (*length) ++;
      gen_rand_expr(length);
      gen_char(')'); (*length) ++;
      break;
    case 2:
      gen_rand_expr(length);
      gen_rand_op(); (*length) ++;
      gen_rand_expr(length);
      break;
  }
  gen_space();
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf_start = buf;
    int length = 0;
    gen_rand_expr(&length);
    if (length > 32) continue;

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // add '-Wall -Werror' to avoid 'divide-0' situation
    int ret = system("gcc -Wall -Werror /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
