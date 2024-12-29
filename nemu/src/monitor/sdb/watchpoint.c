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

#include <string.h>
#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char *expr;
  word_t value;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

static WP* new_wp() {
  if (free_ == NULL) {
    fprintf(stderr, "no more watchpoints\n");
    return NULL;
  }
  WP *wp = free_;
  free_ = free_->next;
  wp->next = head;
  head = wp;
  return wp;
}

static void free_wp(WP *wp) {
  WP* tmp = head;
  if (tmp == wp) {
    head = NULL;
  } else {
    while (tmp != NULL && tmp->next != wp) tmp = tmp->next;
    if (tmp == NULL) {
      fprintf(stderr, "watchpoint %d not in use\n", wp->NO);
      return;
    }
    tmp->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void add_wp(char *expr, word_t value) {
  WP *wp = new_wp();
  if (wp == NULL) return;
  wp->expr = strdup(expr);
  wp->value = value;
  printf("watchpoint added %d: %s\n", wp->NO, expr);
}

void remove_wp(int NO) {
  if (NO >= NR_WP) {
    fprintf(stderr, "watchpoint %d not found\n", NO);
    return;
  }
  WP *wp = &wp_pool[NO];
  free_wp(wp);
  printf("watchpoint removed %d: %s\n", NO, wp->expr);
}

void watchpoint_display() {
  WP *wp = head;
  if (wp == NULL) {
    printf("no watchpoints\n");
    return;
  }
  printf("%-8s%-8s\n", "NO", "EXPR");
  while (wp != NULL) {
    printf("%-8d%-8s\n", wp->NO, wp->expr);
    wp = wp->next;
  }
}

void difftest_wp() {
  WP *wp = head;
  while (wp != NULL) {
    bool _;
    word_t result = expr(wp->expr, &_);
    if (result != wp->value) {
      printf("watchpoint %d: %s triggered: \nold = 0x%08x\nnew = 0x%08x\n", wp->NO, wp->expr, wp->value, result);
      wp->value = result;
      nemu_state.state = NEMU_STOP;
      return;
    }
    wp = wp->next;
  }
}