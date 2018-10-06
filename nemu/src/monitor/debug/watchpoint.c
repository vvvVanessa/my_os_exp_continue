#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
    WP *wp = NULL;
    if (free_ != NULL) {
        wp = free_;
        free_ = free_ -> next;
    }
    Assert(wp != NULL, "wp pool is empty.");
    wp -> next = head;
    head = wp;
    return wp;
}

void free_wp(int no) {
    WP* wp = wp_pool + no;
    WP* pre = NULL;
    WP* cur = head;
    for(; cur != wp && cur != NULL; pre = cur, cur = cur -> next) {}
    Assert(cur == NULL, "wp No.%d not found\n", no);
    if (pre == NULL) { /* if head has been deleted */
        head = head -> next;
    } else {
        pre -> next = cur -> next;
    }

    memset(wp -> str, 0, sizeof(wp -> str));
    wp -> next = free_;
    free_ = wp;
}

void setup_wp(WP* wp, char* str) {
    Assert(strlen(str) <= WP_EXP_LEN, "Expression length out of range.");
    strcpy(wp -> str, str);
    bool success = true;
    uint32_t val = expr(wp -> str, &success);
    if (success) {
        wp -> val = val;
    } else {
        free_wp(wp -> NO);
    }
}

bool check_wp() {
    WP* cur = head;
    while(cur != NULL) {
        bool success = true;
        uint32_t val = expr(cur -> str, &success);
        if (success) {
            if (val != cur -> val) {
                printf("watch point %d changed.\nexpr: %s\nval:%d --> %d\n", cur -> NO, cur -> str, cur -> val, val);
                cur -> val = val;
                return false;
            }
        }
        cur = cur -> next;
    }
    return true;
}

void get_wp_info() {
    WP* cur = head;
    printf("%-5s%-32s%-32s\n", "Num", "Expression", "value");
    while(cur != NULL) {
        printf("%-5d%-32s%-32d\n", cur -> NO, cur -> str, cur -> val);
    }
}

