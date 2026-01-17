#define TB_IMPL
#include "tui.h"
#include "termbox2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// External declarations for HID functions (from hid-gadget.c)
extern int send_keyboard_report(uint8_t modifiers, uint8_t key1, uint8_t key2,
                                uint8_t key3, uint8_t key4, uint8_t key5,
                                uint8_t key6);
extern int send_key_sequence(const char *modifiers_str, const char *sequence);
extern uint8_t parse_modifiers(const char *mod_str, const char **remainder);
// More may be needed if tui.c calls them directly.
// For now, we'll try to keep tui.c self-contained in calling back through
// existing logic if possible, or we'll expose what we need in a header.

// Modifier bits (re-defining or including from a shared header would be better)
#define MOD_CTRL_LEFT (1 << 0)
#define MOD_SHIFT_LEFT (1 << 1)
#define MOD_ALT_LEFT (1 << 2)
#define MOD_GUI_LEFT (1 << 3)

typedef struct {
  char *label;
  char *cmd;
  float width;
} Key;

static Key layout[][20] = {{{"ESC", "ESC", 1},
                            {"F1", "F1", 1},
                            {"F2", "F2", 1},
                            {"F3", "F3", 1},
                            {"F4", "F4", 1},
                            {"F5", "F5", 1},
                            {"F6", "F6", 1},
                            {"F7", "F7", 1},
                            {"F8", "F8", 1},
                            {"F9", "F9", 1},
                            {"F10", "F10", 1},
                            {"F11", "F11", 1},
                            {"F12", "F12", 1},
                            {"HOME", "HOME", 1},
                            {"END", "END", 1},
                            {"DEL", "DELETE", 1},
                            {NULL, NULL, 0}},
                           {{"`", "`", 1},
                            {"1", "1", 1},
                            {"2", "2", 1},
                            {"3", "3", 1},
                            {"4", "4", 1},
                            {"5", "5", 1},
                            {"6", "6", 1},
                            {"7", "7", 1},
                            {"8", "8", 1},
                            {"9", "9", 1},
                            {"0", "0", 1},
                            {"-", "-", 1},
                            {"=", "=", 1},
                            {"BKSP", "BACKSPACE", 2},
                            {NULL, NULL, 0}},
                           {{"TAB", "TAB", 1.5},
                            {"Q", "q", 1},
                            {"W", "w", 1},
                            {"E", "e", 1},
                            {"R", "r", 1},
                            {"T", "t", 1},
                            {"Y", "y", 1},
                            {"U", "u", 1},
                            {"I", "i", 1},
                            {"O", "o", 1},
                            {"P", "p", 1},
                            {"[", "[", 1},
                            {"]", "]", 1},
                            {"\\", "\\", 1},
                            {NULL, NULL, 0}},
                           {{"CAPS", "CAPSLOCK", 1.75},
                            {"A", "a", 1},
                            {"S", "s", 1},
                            {"D", "d", 1},
                            {"F", "f", 1},
                            {"G", "g", 1},
                            {"H", "h", 1},
                            {"J", "j", 1},
                            {"K", "k", 1},
                            {"L", "l", 1},
                            {";", ";", 1},
                            {"'", "'", 1},
                            {"ENTER", "ENTER", 2.25},
                            {NULL, NULL, 0}},
                           {{"SHIFT", "SHIFT", 2.25},
                            {"Z", "z", 1},
                            {"X", "x", 1},
                            {"C", "c", 1},
                            {"V", "v", 1},
                            {"B", "b", 1},
                            {"N", "n", 1},
                            {"M", "m", 1},
                            {",", ",", 1},
                            {".", ".", 1},
                            {"/", "/", 1},
                            {"SHIFT", "SHIFT", 2.75},
                            {NULL, NULL, 0}},
                           {{"CTRL", "CTRL", 1.5},
                            {"WIN", "WIN", 1.5},
                            {"ALT", "ALT", 1.5},
                            {"SPACE", "SPACE", 5.0},
                            {"ALT", "ALT", 1.5},
                            {"CTRL", "CTRL", 1.5},
                            {"\u2190", "LEFT", 1},
                            {"\u2192", "RIGHT", 1},
                            {"\u2191", "UP", 1},
                            {"\u2193", "DOWN", 1},
                            {NULL, NULL, 0}},
                           {{NULL, NULL, 0}}};

static uint8_t active_mods = 0;
static int win_press_count = 0;

typedef struct {
  char label[16];
  char cmd[32];
  int x, y, w, h;
} RenderKey;

static RenderKey render_keys[128];
static int render_key_count = 0;

void draw_text(int x, int y, uintattr_t fg, uintattr_t bg, const char *str) {
  while (*str) {
    uint32_t uni;
    str += tb_utf8_char_to_unicode(&uni, str);
    tb_set_cell(x++, y, uni, fg, bg);
  }
}

void render_keyboard() {
  tb_clear();
  int term_w = tb_width();
  int term_h = tb_height();

  draw_text(2, 0, TB_WHITE | TB_BOLD, TB_DEFAULT,
            "HID C-TUI v1.24.0 | CTRL+ALT+Q to quit");

  int scale = (term_w - 4) / 16;
  if (scale < 2)
    scale = 2;

  render_key_count = 0;
  // Calculate vertical start to align to bottom
  int total_rows = 0;
  while (layout[total_rows][0].label != NULL)
    total_rows++;
  int ky = term_h - (total_rows * 3) - 1;
  if (ky < 2)
    ky = 2;

  for (int r = 0; layout[r][0].label != NULL; r++) {
    int kx = 2;
    for (int k = 0; layout[r][k].label != NULL; k++) {
      Key *key = &layout[r][k];
      int kw = (int)(key->width * scale);
      int kh = 2;

      RenderKey *rk = &render_keys[render_key_count++];
      strncpy(rk->label, key->label, sizeof(rk->label));
      strncpy(rk->cmd, key->cmd, sizeof(rk->cmd));
      rk->x = kx;
      rk->y = ky;
      rk->w = kw;
      rk->h = kh;

      uintattr_t bg = TB_WHITE;
      uintattr_t fg = TB_BLACK;

      // Check if it's an active modifier
      if (strcmp(rk->cmd, "CTRL") == 0 && (active_mods & MOD_CTRL_LEFT))
        bg = TB_YELLOW;
      if (strcmp(rk->cmd, "SHIFT") == 0 && (active_mods & MOD_SHIFT_LEFT))
        bg = TB_YELLOW;
      if (strcmp(rk->cmd, "ALT") == 0 && (active_mods & MOD_ALT_LEFT))
        bg = TB_YELLOW;
      if (strcmp(rk->cmd, "WIN") == 0 && (active_mods & MOD_GUI_LEFT))
        bg = TB_YELLOW;

      for (int iy = 0; iy < kh; iy++) {
        for (int ix = 0; ix < kw; ix++) {
          tb_set_cell(kx + ix, ky + iy, ' ', fg, bg);
        }
      }

      int lx = kx + (kw - strlen(rk->label)) / 2;
      int ly = ky + kh / 2;
      draw_text(lx, ly, fg | TB_BOLD, bg, rk->label);

      kx += kw + 1;
    }
    ky += 3;
  }
  tb_present();
}

void handle_input(const char *cmd) {
  if (strcmp(cmd, "CTRL") == 0) {
    active_mods ^= MOD_CTRL_LEFT;
    return;
  }
  if (strcmp(cmd, "SHIFT") == 0) {
    active_mods ^= MOD_SHIFT_LEFT;
    return;
  }
  if (strcmp(cmd, "ALT") == 0) {
    active_mods ^= MOD_ALT_LEFT;
    return;
  }
  if (strcmp(cmd, "WIN") == 0) {
    if (win_press_count == 1) {
      active_mods &= ~MOD_GUI_LEFT;
      send_key_sequence("WIN", NULL);
      win_press_count = 0;
    } else {
      active_mods |= MOD_GUI_LEFT;
      win_press_count = 1;
    }
    return;
  }

  // Regular key
  char mods_str[32] = {0};
  if (active_mods & MOD_CTRL_LEFT)
    strcat(mods_str, "CTRL-");
  if (active_mods & MOD_SHIFT_LEFT)
    strcat(mods_str, "SHIFT-");
  if (active_mods & MOD_ALT_LEFT)
    strcat(mods_str, "ALT-");
  if (active_mods & MOD_GUI_LEFT)
    strcat(mods_str, "WIN-");

  if (strlen(mods_str) > 0)
    mods_str[strlen(mods_str) - 1] = '\0'; // Remove trailing dash

  send_key_sequence(mods_str[0] ? mods_str : NULL, cmd);

  // Auto-release WIN if it was a single press modifier
  if (win_press_count == 1) {
    active_mods &= ~MOD_GUI_LEFT;
    win_press_count = 0;
  }
}

int run_tui(void) {
  int err = tb_init();
  if (err) {
    fprintf(stderr, "termbox init failed: %d\n", err);
    return EXIT_FAILURE;
  }

  tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

  while (1) {
    render_keyboard();

    struct tb_event ev;
    int res = tb_poll_event(&ev);
    if (res < 0)
      break;

    if (ev.type == TB_EVENT_KEY) {
      if ((ev.mod & TB_MOD_CTRL) && (ev.mod & TB_MOD_ALT) &&
          (ev.ch == 'q' || ev.ch == 'Q'))
        break;

      if (ev.key == TB_KEY_ESC) {
        handle_input("ESC");
        continue;
      }

      // Physical keyboard mapping
      if (ev.key == TB_KEY_ENTER)
        handle_input("ENTER");
      else if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2)
        handle_input("BACKSPACE");
      else if (ev.key == TB_KEY_TAB)
        handle_input("TAB");
      else if (ev.key == TB_KEY_SPACE)
        handle_input("SPACE");
      else if (ev.key == TB_KEY_ARROW_UP)
        handle_input("UP");
      else if (ev.key == TB_KEY_ARROW_DOWN)
        handle_input("DOWN");
      else if (ev.key == TB_KEY_ARROW_LEFT)
        handle_input("LEFT");
      else if (ev.key == TB_KEY_ARROW_RIGHT)
        handle_input("RIGHT");
      else if (ev.key == TB_KEY_DELETE)
        handle_input("DELETE");
      else if (ev.ch) {
        char buf[8];
        int len = tb_utf8_unicode_to_char(buf, ev.ch);
        buf[len] = '\0';
        handle_input(buf);
      }
    } else if (ev.type == TB_EVENT_MOUSE) {
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        for (int i = 0; i < render_key_count; i++) {
          RenderKey *rk = &render_keys[i];
          if (ev.x >= rk->x && ev.x < rk->x + rk->w && ev.y >= rk->y &&
              ev.y < rk->y + rk->h) {
            handle_input(rk->cmd);
            break;
          }
        }
      }
    } else if (ev.type == TB_EVENT_RESIZE) {
      // Screen handles resize in the next loop
    }
  }

  tb_shutdown();
  return EXIT_SUCCESS;
}
