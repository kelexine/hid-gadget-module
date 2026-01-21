#include "../include/ducky.h"
#include "../include/hid_interface.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LEN 1024
#define MAX_LINES 2048
#define MAX_VARS 128
#define MAX_VAR_NAME 64
#define MAX_VAR_VAL 256
#define MAX_FUNCTIONS 32
#define MAX_FUNC_PARAMS 8
#define MAX_LABELS 128

typedef struct {
  char name[MAX_VAR_NAME];
  char value[MAX_VAR_VAL];
} Variable;

typedef struct {
  char name[MAX_VAR_NAME];
  int start_line;
  int param_count;
  char params[MAX_FUNC_PARAMS][MAX_VAR_NAME];
} Function;

typedef struct {
  char name[MAX_VAR_NAME];
  int line;
} Label;

typedef struct {
  char **lines;
  int count;
} Script;

static Variable g_vars[MAX_VARS];
static int g_var_count = 0;
static Function g_functions[MAX_FUNCTIONS];
static int g_func_count = 0;
static Label g_labels[MAX_LABELS];
static int g_label_count = 0;
static int g_default_delay = 0;
static int g_default_delay_fuzz = 0;
static int g_default_char_delay = 0;
static int g_default_char_fuzz = 0;
static int g_in_function = 0;

#define LED_NUMLOCK 0x01
#define LED_CAPSLOCK 0x02
#define LED_SCROLLLOCK 0x04

static int g_hid_led_fd = -1;
static int g_last_led_report = 0;

static int get_led_state() {
  if (g_hid_led_fd < 0) {
    const char *dev = getenv("HID_KEYBOARD_DEV");
    if (!dev)
      dev = "/dev/hidg0";
    g_hid_led_fd = open(dev, O_RDONLY | O_NONBLOCK);
  }
  if (g_hid_led_fd < 0)
    return g_last_led_report;

  unsigned char report = 0;
  while (read(g_hid_led_fd, &report, 1) > 0) {
    g_last_led_report = report;
  }
  return g_last_led_report;
}

static const char *get_system_var(const char *name);
void ducky_set_var(const char *name, const char *val);
static Variable *find_var(const char *name);

static void rtrim(char *str);
static void free_script(Script *s);
static int load_script(const char *filename, Script *s);

static void free_script(Script *s) {
  if (!s)
    return;
  for (int i = 0; i < s->count; i++)
    free(s->lines[i]);
  free(s->lines);
  s->count = 0;
}

static void rtrim(char *str) {
  size_t n = strlen(str);
  while (n > 0 && isspace((unsigned char)str[n - 1]))
    n--;
  str[n] = '\0';
}

static char *lskip(const char *s) {
  while (*s && isspace((unsigned char)*s))
    s++;
  return (char *)s;
}

static Variable *find_var(const char *name) {
  for (int i = 0; i < g_var_count; i++) {
    if (strcmp(g_vars[i].name, name) == 0)
      return &g_vars[i];
  }
  return NULL;
}

void ducky_set_var(const char *name, const char *val) {
  Variable *v = find_var(name);
  if (!v) {
    if (g_var_count >= MAX_VARS)
      return;
    v = &g_vars[g_var_count++];
    snprintf(v->name, MAX_VAR_NAME, "%s", name);
  }
  snprintf(v->value, MAX_VAR_VAL, "%s", val);
}

static const char *get_system_var(const char *name) {
  // 1. Check for manual overrides in script-defined variables (for Profiles)
  Variable *v = find_var(name);
  if (v)
    return v->value;

  // 2. Check environment variables
  const char *env = getenv(name);
  if (env)
    return env;

  // 3. Handle Special Hardcoded/Dynamic Logic
  if (strcmp(name, "_OS") == 0) {
    const char *tos = getenv("TARGET_OS");
    return tos ? tos : "WINDOWS";
  }
  if (strcmp(name, "WINDOWS") == 0)
    return "WINDOWS";
  if (strcmp(name, "LINUX") == 0)
    return "LINUX";
  if (strcmp(name, "MACOS") == 0)
    return "MACOS";

  if (strcmp(name, "_CAPSLOCK_ON") == 0)
    return (get_led_state() & LED_CAPSLOCK) ? "TRUE" : "FALSE";
  if (strcmp(name, "_NUMLOCK_ON") == 0)
    return (get_led_state() & LED_NUMLOCK) ? "TRUE" : "FALSE";
  if (strcmp(name, "_SCROLLOCK_ON") == 0)
    return (get_led_state() & LED_SCROLLLOCK) ? "TRUE" : "FALSE";

  if (strcmp(name, "_RANDOM_INT") == 0) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%d", rand() % 10000);
    return buf;
  }
  if (strcmp(name, "_RANDOM_LOWERCASE_LETTER") == 0) {
    static char buf[2];
    buf[0] = 'a' + (rand() % 26);
    buf[1] = '\0';
    return buf;
  }
  if (strcmp(name, "_RANDOM_UPPERCASE_LETTER") == 0) {
    static char buf[2];
    buf[0] = 'A' + (rand() % 26);
    buf[1] = '\0';
    return buf;
  }
  if (strcmp(name, "_RANDOM_HEX") == 0) {
    static char buf[2];
    const char *hex = "0123456789ABCDEF";
    buf[0] = hex[rand() % 16];
    buf[1] = '\0';
    return buf;
  }
  if (strcmp(name, "_RANDOM_CHAR") == 0) {
    static char buf[2];
    buf[0] = 33 + (rand() % 94); // ASCII range 33-126
    buf[1] = '\0';
    return buf;
  }
  if (strcmp(name, "_TIMESTAMP") == 0) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)time(NULL));
    return buf;
  }

  // 4. OS metadata lookup
  const char *os = get_system_var("_OS");
  struct os_meta {
    const char *os;
    const char *major;
    const char *build;
  } meta[] = {{"WINDOWS", "10", "19041"}, {"WINDOWS_11", "11", "22000"},
              {"MACOS", "14", "0"},       {"LINUX", "6", "0"},
              {"ANDROID", "13", "0"},     {NULL, NULL, NULL}};

  for (int i = 0; meta[i].os; i++) {
    if (strcmp(os, meta[i].os) == 0) {
      if (strcmp(name, "_OS_VERSION_MAJOR") == 0)
        return meta[i].major;
      if (strcmp(name, "_BUILD_NUMBER") == 0)
        return meta[i].build;
    }
  }

  // Generic fallback
  if (strcmp(name, "_OS_VERSION_MAJOR") == 0)
    return "0";
  if (strcmp(name, "_BUILD_NUMBER") == 0)
    return "0";

  return NULL;
}

static int load_script(const char *filename, Script *s) {
  FILE *fp = stdin;
  if (strcmp(filename, "-") != 0) {
    fp = fopen(filename, "r");
    if (!fp) {
      perror("Error opening script");
      return -1;
    }
  }
  s->lines = malloc(sizeof(char *) * MAX_LINES);
  s->count = 0;
  char buf[MAX_LINE_LEN];
  while (fgets(buf, sizeof(buf), fp) && s->count < MAX_LINES) {
    rtrim(buf);
    s->lines[s->count++] = strdup(buf);
  }
  if (fp != stdin)
    fclose(fp);

  g_func_count = 0;
  g_label_count = 0;
  for (int i = 0; i < s->count; i++) {
    char *line = lskip(s->lines[i]);
    if (line[0] == ':') {
      if (g_label_count < MAX_LABELS) {
        snprintf(g_labels[g_label_count].name, MAX_VAR_NAME, "%s",
                 lskip(line + 1));
        rtrim(g_labels[g_label_count].name);
        g_labels[g_label_count].line = i;
        g_label_count++;
      }
    } else if (strncmp(line, "FUNCTION ", 9) == 0) {
      if (g_func_count < MAX_FUNCTIONS) {
        Function *nf = &g_functions[g_func_count++];
        const char *p = lskip(line + 9);
        int j = 0;
        while (*p && *p != '(' && !isspace(*p) && j < MAX_VAR_NAME - 1)
          nf->name[j++] = *p++;
        nf->name[j] = '\0';
        nf->start_line = i + 1;
        nf->param_count = 0;
        if (*p == '(') {
          p++;
          while (*p && *p != ')') {
            p = lskip(p);
            if (*p == '$')
              p++;
            j = 0;
            while (*p && *p != ',' && *p != ')' && !isspace(*p) &&
                   j < MAX_VAR_NAME - 1)
              nf->params[nf->param_count][j++] = *p++;
            nf->params[nf->param_count][j] = '\0';
            if (j > 0)
              nf->param_count++;
            if (*p == ',')
              p++;
          }
        }
      }
    }
  }
  return 0;
}

static char *substitute_vars(const char *line) {
  char *res = strdup(line);
  int changed = 1;
  while (changed) {
    changed = 0;
    char *p = strchr(res, '$');
    if (!p)
      break;

    char name[MAX_VAR_NAME];
    char *start = p;
    p++;
    int i = 0;
    while (*p && (isalnum(*p) || *p == '_') && i < MAX_VAR_NAME - 1)
      name[i++] = *p++;
    name[i] = '\0';

    const char *val = get_system_var(name);
    if (!val) {
      Variable *v = find_var(name);
      if (v)
        val = v->value;
    }

    if (val) {
      int prefix = start - res;
      int vlen = strlen(val);
      int slen = strlen(p);
      char *nres = malloc(prefix + vlen + slen + 1);
      memcpy(nres, res, prefix);
      memcpy(nres + prefix, val, vlen);
      strcpy(nres + prefix + vlen, p);
      free(res);
      res = nres;
      changed = 1;
    } else {
      *start = '#';
      changed = 1;
    }
  }
  for (char *c = res; *c; c++)
    if (*c == '#')
      *c = '$';
  return res;
}

static int eval_condition(const char *cond) {
  char *sub = substitute_vars(cond);
  char *work = sub;

  // Handle && and || (Simple left-to-right, no complex nesting support yet)
  char *and_pos = strstr(work, " && ");
  char *or_pos = strstr(work, " || ");

  if (and_pos) {
    *and_pos = '\0';
    int res = eval_condition(work) && eval_condition(and_pos + 4);
    free(sub);
    return res;
  }
  if (or_pos) {
    *or_pos = '\0';
    int res = eval_condition(work) || eval_condition(or_pos + 4);
    free(sub);
    return res;
  }

  while (*work && (*work == ' ' || *work == '('))
    work++;
  char *end = work + strlen(work) - 1;
  while (end > work && (*end == ' ' || *end == ')')) {
    *end = '\0';
    end--;
  }

  char lstr[MAX_VAR_VAL], rstr[MAX_VAR_VAL], op[4];
  int res = 0;
  if (sscanf(work, "%255s %3s %255s", lstr, op, rstr) >= 3) {
    const char *lv = lstr;
    const char *rv = rstr;
    // (OS logic removed here as get_system_var handles it now)

    if (strcmp(op, "==") == 0)
      res = (strcmp(lv, rv) == 0);
    else if (strcmp(op, "!=") == 0)
      res = (strcmp(lv, rv) != 0);
    else {
      int li = atoi(lv), ri = atoi(rv);
      if (strcmp(op, ">") == 0)
        res = (li > ri);
      else if (strcmp(op, "<") == 0)
        res = (li < ri);
      else if (strcmp(op, ">=") == 0)
        res = (li >= ri);
      else if (strcmp(op, "<=") == 0)
        res = (li <= ri);
    }
  } else if (sscanf(work, "%255s", lstr) == 1) {
    if (strcmp(lstr, "TRUE") == 0)
      res = 1;
    else if (strcmp(lstr, "FALSE") == 0)
      res = 0;
    else
      res = (atoi(lstr) != 0);
  }
  free(sub);
  return res;
}

static int exec_line(Script *s, int pc);

static int exec_line(Script *s, int pc) {
  if (pc < 0 || pc >= s->count)
    return s->count;
  char *line = lskip(s->lines[pc]);
  if (*line == '\0' || strncmp(line, "REM", 3) == 0 || line[0] == ':') {
    if (strncmp(line, "REM_BLOCK", 9) == 0) {
      for (int i = pc + 1; i < s->count; i++) {
        if (strncmp(lskip(s->lines[i]), "END_REM_BLOCK", 13) == 0)
          return i + 1;
      }
      return s->count; // Unterminated block
    }
    return pc + 1;
  }

  // GOTO
  if (strncmp(line, "GOTO ", 5) == 0) {
    char label[MAX_VAR_NAME];
    snprintf(label, MAX_VAR_NAME, "%s", lskip(line + 5));
    rtrim(label);
    for (int i = 0; i < g_label_count; i++)
      if (strcmp(g_labels[i].name, label) == 0)
        return g_labels[i].line;
    return pc + 1;
  }

  // No early substitution. Print raw line for debugging.
  fprintf(stderr, "[Ducky] %d: %s\n", pc + 1, line);

  if (strncmp(line, "STRING ", 7) == 0 || strncmp(line, "STRINGLN ", 9) == 0) {
    char *sub = substitute_vars(line);
    int ln = (sub[6] == 'L');
    const char *t = sub + (ln ? 9 : 7);
    for (const char *c = t; *c; c++) {
      char b[2] = {*c, 0};
      send_key_sequence(NULL, b);
      if (g_default_char_delay > 0)
        hid_sleep(g_default_char_delay + (rand() % (g_default_char_fuzz + 1)));
    }
    if (ln)
      send_key_sequence(NULL, "ENTER");
    free(sub);
  } else if (strncmp(line, "DELAY ", 6) == 0) {
    char *sub = substitute_vars(line);
    hid_sleep(atoi(sub + 6));
    free(sub);
  } else if (strncmp(line, "IF ", 3) == 0) {
    char *sub = substitute_vars(line);
    char *c = strdup(sub + 3);
    char *t = strstr(c, " THEN");
    if (t)
      *t = '\0';
    int res = eval_condition(c);
    free(c);
    if (!res) {
      int nest = 1;
      for (int i = pc + 1; i < s->count; i++) {
        char *l = lskip(s->lines[i]);
        if (strncmp(l, "IF ", 3) == 0)
          nest++;
        if (strncmp(l, "ENDIF", 5) == 0 || strncmp(l, "END_IF", 6) == 0)
          nest--;
        if (nest == 1 && strncmp(l, "ELSE", 4) == 0) {
          free(sub);
          return i + 1;
        }
        if (nest == 0) {
          free(sub);
          return i + 1;
        }
      }
    }
  } else if (strncmp(line, "ELSE", 4) == 0) {
    int nest = 1;
    for (int i = pc + 1; i < s->count; i++) {
      char *l = lskip(s->lines[i]);
      if (strncmp(l, "IF ", 3) == 0)
        nest++;
      if (strncmp(l, "ENDIF", 5) == 0 || strncmp(l, "END_IF", 6) == 0)
        nest--;
      if (nest == 0) {
        return i + 1;
      }
    }
  } else if (strncmp(line, "FOR ", 4) == 0) {
    char *sub = substitute_vars(line);
    char var[MAX_VAR_NAME];
    int st, en;
    if (sscanf(sub, "FOR $%63s = %d TO %d", var, &st, &en) >= 3 ||
        sscanf(sub, "FOR %63s = %d TO %d", var, &st, &en) >= 3) {
      for (int v = st; v <= en; v++) {
        char b[16];
        snprintf(b, 16, "%d", v);
        ducky_set_var(var, b);
        int lpc = pc + 1;
        while (lpc < s->count && strncmp(lskip(s->lines[lpc]), "NEXT", 4) != 0)
          lpc = exec_line(s, lpc);
      }
      free(sub);
      for (int j = pc + 1; j < s->count; j++)
        if (strncmp(lskip(s->lines[j]), "NEXT", 4) == 0) {
          return j + 1;
        }
    } else {
      free(sub);
    }
  } else if (strncmp(line, "ECHO ", 5) == 0) {
    char *sub = substitute_vars(line);
    printf("%s\n", sub + 5);
    free(sub);
  } else if (strncmp(line, "VAR ", 4) == 0 || line[0] == '$') {
    char name[MAX_VAR_NAME];
    const char *p = (line[0] == '$') ? line : lskip(line + 4);
    if (*p == '$')
      p++;
    int i = 0;
    while (*p && (isalnum(*p) || *p == '_') && i < MAX_VAR_NAME - 1)
      name[i++] = *p++;
    name[i] = '\0';
    p = lskip(p);
    if (*p == '=') {
      char *expr_raw = lskip((char *)p + 1);
      char *expr = substitute_vars(expr_raw);
      int lv, rv;
      char op;
      if (sscanf(expr, "%d %c %d", &lv, &op, &rv) == 3) {
        int r = (op == '+')   ? lv + rv
                : (op == '-') ? lv - rv
                : (op == '*') ? lv * rv
                : (op == '/') ? (rv ? lv / rv : 0)
                              : lv;
        char b[16];
        snprintf(b, 16, "%d", r);
        ducky_set_var(name, b);
      } else {
        ducky_set_var(name, expr);
      }
      free(expr);
    }
  } else if (strncmp(line, "HOLD ", 5) == 0) {
    char *sub = substitute_vars(line);
    hold_key(sub + 5);
    free(sub);
  } else if (strncmp(line, "RELEASE ", 8) == 0) {
    char *sub = substitute_vars(line);
    release_key(sub + 8);
    free(sub);
  } else if (strncmp(line, "LOCALE ", 7) == 0) {
    char *sub = substitute_vars(line);
    set_hid_locale(sub + 7);
    free(sub);
  } else if (strncmp(line, "KEYCODE ", 8) == 0) {
    char *sub = substitute_vars(line);
    uint8_t report[8] = {0};
    char *p = sub + 8;
    int i = 0;
    while (*p && i < 8) {
      report[i++] = (uint8_t)strtol(p, &p, 0);
      while (*p && (*p == ' ' || *p == ','))
        p++;
    }
    if (i > 0)
      send_raw_hid_report(report, 8);
    free(sub);
  } else if (strncmp(line, "EXTENSION ", 10) == 0) {
    fprintf(stderr,
            "[Ducky-HW] Extension command '%s' is not supported on this "
            "platform.\n",
            line + 10);
  } else if (strncmp(line, "WAIT_FOR_CAPS_ON", 16) == 0) {
    while (!(get_led_state() & LED_CAPSLOCK))
      hid_sleep(10);
  } else if (strncmp(line, "WAIT_FOR_CAPS_OFF", 17) == 0) {
    while (get_led_state() & LED_CAPSLOCK)
      hid_sleep(10);
  } else if (strncmp(line, "WAIT_FOR_NUM_ON", 15) == 0) {
    while (!(get_led_state() & LED_NUMLOCK))
      hid_sleep(10);
  } else if (strncmp(line, "WAIT_FOR_NUM_OFF", 16) == 0) {
    while (get_led_state() & LED_NUMLOCK)
      hid_sleep(10);
  } else if (strncmp(line, "WAIT_FOR_SCROLL_ON", 18) == 0) {
    while (!(get_led_state() & LED_SCROLLLOCK))
      hid_sleep(10);
  } else if (strncmp(line, "WAIT_FOR_SCROLL_OFF", 19) == 0) {
    while (get_led_state() & LED_SCROLLLOCK)
      hid_sleep(10);
  } else if (strncmp(line, "ATTACKMODE ", 11) == 0) {
    char *sub = substitute_vars(line);
    fprintf(stderr, "[Ducky-HW] ATTACKMODE: %s\n", sub + 11);
    free(sub);
  } else if (strncmp(line, "LED ", 4) == 0) {
    char *sub = substitute_vars(line);
    fprintf(stderr, "[Ducky-HW] LED Color: %s\n", sub + 4);
    free(sub);
  } else if (strncmp(line, "WAIT_FOR_BUTTON_PRESS", 21) == 0) {
    fprintf(stderr, "[Ducky-HW] WAIT_FOR_BUTTON_PRESS (Skipping...)\n");
  } else if (strncmp(line, "DEFAULTDELAY ", 13) == 0) {
    char *sub = substitute_vars(line);
    g_default_delay = atoi(sub + 13);
    free(sub);
  } else if (strncmp(line, "FUNCTION ", 9) == 0 ||
             strncmp(line, "END_FUNCTION", 12) == 0 ||
             strncmp(line, "RETURN", 6) == 0 ||
             strncmp(line, "ENDIF", 5) == 0 ||
             strncmp(line, "END_IF", 6) == 0) { /* Handled elsewhere */
  } else {
    char *sub = substitute_vars(line);
    // Try function call or modifiers
    Function *f = NULL;
    char f_name[MAX_VAR_NAME];
    int ni = 0;
    while (sub[ni] && (isalnum(sub[ni]) || sub[ni] == '_') &&
           ni < MAX_VAR_NAME - 1)
      f_name[ni] = sub[ni], ni++;
    f_name[ni] = '\0';
    for (int i = 0; i < g_func_count; i++)
      if (strcmp(g_functions[i].name, f_name) == 0) {
        f = &g_functions[i];
        break;
      }
    if (f) {
      int saved = g_in_function;
      g_in_function = 1;
      int f_pc = f->start_line;
      while (f_pc < s->count) {
        char *fl = lskip(s->lines[f_pc]);
        if (strncmp(fl, "END_FUNCTION", 12) == 0 ||
            strncmp(fl, "RETURN", 6) == 0)
          break;
        f_pc = exec_line(s, f_pc);
      }
      g_in_function = saved;
    } else {
      char *tokens[10];
      int count = 0;
      char *p = lskip(sub);
      while (*p && count < 10) {
        char *st = p;
        while (*p && !isspace(*p))
          p++;
        tokens[count++] = strndup(st, p - st);
        p = lskip(p);
      }
      if (count > 0) {
        static const char *mods_map[] = {"CTRL",    "CONTROL", "SHIFT",
                                         "ALT",     "OPTION",  "GUI",
                                         "WINDOWS", "COMMAND", NULL};
        char mods_str[256] = "";
        int m_cnt = 0;
        int k_idx = -1;
        for (int i = 0; i < count; i++) {
          int found = 0;
          for (int j = 0; mods_map[j]; j++)
            if (strcasecmp(tokens[i], mods_map[j]) == 0) {
              if (m_cnt++)
                strcat(mods_str, " ");
              if (strcasecmp(tokens[i], "CONTROL") == 0)
                strcat(mods_str, "CTRL");
              else if (strcasecmp(tokens[i], "OPTION") == 0)
                strcat(mods_str, "ALT");
              else if (strcasecmp(tokens[i], "WINDOWS") == 0 ||
                       strcasecmp(tokens[i], "COMMAND") == 0)
                strcat(mods_str, "GUI");
              else
                strcat(mods_str, tokens[i]);
              found = 1;
              break;
            }
          if (!found) {
            k_idx = i;
            break;
          }
        }
        if (k_idx >= 0)
          send_key_sequence(m_cnt > 0 ? mods_str : NULL, tokens[k_idx]);
        else if (m_cnt > 0)
          send_key_sequence(mods_str, NULL);
        else
          send_key_sequence(NULL, sub); // Fallback to single key
        for (int i = 0; i < count; i++)
          free(tokens[i]);
      }
    }
    free(sub);
  }
  if (g_default_delay > 0)
    hid_sleep(g_default_delay + (rand() % (g_default_delay_fuzz + 1)));
  return pc + 1;
}

void ducky_init() {
  static int initialized = 0;
  if (!initialized) {
    srand(time(NULL));
    // Setup system constants
    ducky_set_var("WINDOWS", "WINDOWS");
    ducky_set_var("LINUX", "LINUX");
    ducky_set_var("MACOS", "MACOS");
    initialized = 1;
  }
}

void ducky_load_profile() {
  ducky_init();
  Script vars_s;
  if (access("ducky_vars.ducky", F_OK) == 0) {
    if (load_script("ducky_vars.ducky", &vars_s) == 0) {
      for (int i = 0; i < vars_s.count; i++)
        exec_line(&vars_s, i);
      free_script(&vars_s);
    }
  }
}

int ducky_execute_script(const char *filename) {
  ducky_init();

  Script s;
  if (load_script(filename, &s) != 0)
    return -1;
  int pc = 0;
  while (pc < s.count)
    pc = exec_line(&s, pc);
  if (g_hid_led_fd >= 0) {
    close(g_hid_led_fd);
    g_hid_led_fd = -1;
  }
  free_script(&s);
  return 0;
}
