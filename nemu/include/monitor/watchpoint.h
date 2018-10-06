#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define NR_WP 32
#define WP_EXP_LEN 128
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  //char *str;
  /* maximum length of watchpoint expr is fixed here */
  char str[WP_EXP_LEN];
  uint32_t val;
} WP;

WP* new_wp();
void free_wp(int);
void setup_wp(WP* wp, char* str);
bool check_wp();
void get_wp_info();

#endif
