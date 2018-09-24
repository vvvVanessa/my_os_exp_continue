#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "cpu/reg.h"
#include "memory.h"
#include "nemu.h"

#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
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
  return -1;
}

static int cmd_si(char *args) {
    int N;
    if(args == NULL) {
        N = 1;
    } else {
        if(strlen(args) > 1) {
            printf("N must be positive and less than 10.\n");
            return 0;
        }
        N = *args - '0';
    }
    cpu_exec(N);
    return 0;
}

static int cmd_info(char *args) {
    if (!(strcmp(args, "r") == 0 || strcmp(args, "w") == 0)) {
        printf("wrong argument.\n");
    } else {

        if (strcmp(args, "r") == 0) {
            for (int i = 0; i < 8; i++) {
                printf("%-6s%u\n", reg_name(i, 4), reg_l(i));
                printf("%-6s%u\n", reg_name(i, 2), reg_w(i));
                printf("%-6s%u\n", reg_name(i, 1), reg_b(i));
            }
        } else if (strcmp(args, "w") == 0) {
        }
    }
    return 0;
}

static int cmd_p(char *args) {
    return 0;
}

static int cmd_x(char *args) {
    char *arg0 = strtok(NULL, " ");
    if (arg0 == NULL) {
        printf("wrong expression");
        return 0;
    }
    int N = atoi(arg0);
    char *arg1 = strtok(NULL, " ");
    if (arg1 == NULL) 
        return 0;
    int len = strlen(arg1);
    for (int i = 0; i < len; i++) {
        if (arg1[i] >= 'A' && arg1[i] <= 'Z')
            arg1[i] += 32;
    }

    paddr_t addr = 0;
    uint32_t base = 0;
    if (arg1[0] == '0' && arg1[1] == 'x') {
        for (int i = len - 1; i > 1; i--) {
            if (arg1[i] >= 'a' && arg1[i] <= 'f') {
                addr += (arg1[i] - 'a') << (base << 2);
                ++base;
            } else if (arg1[i] >= '0' && arg1[i] <= '9'){
                addr += (arg1[i] - '0') << (base << 2);
                ++base;
            } else {
                printf("wrong expression.\n");
                return 0;
            }
        }
        for (int i = 0; i < N; i++) {
            printf("%-10x%u\n", addr, paddr_read(addr, 4));
            ++addr;
        }
    }
    return 0;
}

static int cmd_w(char *args) {
    return 0;
}

static int cmd_d(char *args) {
    return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "suspend execution after N step, default N=1", cmd_si},
  { "info", "info r: print information of registers; info w: print information of watch-points", cmd_info },
  { "p", "calculate EXPR", cmd_p },
  { "x", "calculate EXPR", cmd_x },
  { "w", "suspend execution when the value of EXPR changes", cmd_w },
  { "d", "delete watch point N", cmd_d },

  /* TODO: Add more commands */
  /* DONE: 2018-9-24 18:23*/
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

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

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
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
