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

// Modifier bits
#define MOD_CTRL_LEFT (1 << 0)
#define MOD_SHIFT_LEFT (1 << 1)
#define MOD_ALT_LEFT (1 << 2)
#define MOD_GUI_LEFT (1 << 3)

typedef struct {
  const char *label;
  const char *cmd;
  float width; // Width in units (u)
} Key;

// Realistic Staggered Layout (Mechanical Keyboard Style)
static Key layout[6][20] = {
    // Row 0: Function Keys (0 offset)
    {{"ESC", "ESC", 1},
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

    // Row 1: Numbers (0 offset)
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

    // Row 2: QWERTY (1.5u Tab)
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
     {"\\", "\\", 1.5},
     {NULL, NULL, 0}},

    // Row 3: ASDF (1.75u Caps, 2.25u Enter)
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

    // Row 4: ZXCV (2.25u ShiftL, 2.75u ShiftR)
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
     {"\u2191", "UP", 1},
     {NULL, NULL, 0}},

    // Row 5: Modifiers (1.25u/1.5u, 6.25u Space, T-Arrows)
    {{"CTRL", "CTRL", 1.5},
     {"WIN", "WIN", 1.25},
     {"ALT", "ALT", 1.25},
     {"SPACE", "SPACE", 6.25},
     {"ALT", "ALT", 1.25},
     {"CTRL", "CTRL", 1.5},
     {"\u2190", "LEFT", 1},
     {"\u2193", "DOWN", 1},
     {"\u2192", "RIGHT", 1},
     {NULL, NULL, 0}}};

// Row Offsets in units (to create staggering)
static float row_offsets[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

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

// Draw a box with Unicode characters
void draw_box(int x, int y, int w, int h, uintattr_t fg, uintattr_t bg) {
  for (int iy = 0; iy < h; iy++) {
    for (int ix = 0; ix < w; ix++) {
      uint32_t ch = ' ';
      if (iy == 0 && ix == 0)
        ch = 0x250C; // ┌
      else if (iy == 0 && ix == w - 1)
        ch = 0x2510; // ┐
      else if (iy == h - 1 && ix == 0)
        ch = 0x2514; // └
      else if (iy == h - 1 && ix == w - 1)
        ch = 0x2518; // ┘
      else if (iy == 0 || iy == h - 1)
        ch = 0x2500; // ─
      else if (ix == 0 || ix == w - 1)
        ch = 0x2502; // │
      tb_set_cell(x + ix, y + iy, ch, fg, bg);
    }
  }
}

void render_keyboard() {
  tb_clear();
  int term_w = tb_width();
  int term_h = tb_height();

  // Header Title
  draw_text(2, 0, TB_WHITE | TB_BOLD, TB_DEFAULT,
            "HID INDUSTRIAL v1.26.0 | Tap EXIT or Ctrl+C to quit");

  // --- EXIT BUTTON ROW ---
  int btn_w = 14;
  int btn_x = (term_w - btn_w) / 2;
  int btn_y = 1; // Row 1
  draw_box(btn_x, btn_y, btn_w, 3, TB_WHITE | TB_BOLD, TB_RED);
  draw_text(btn_x + 3, btn_y + 1, TB_WHITE | TB_BOLD, TB_RED, "[ EXIT ]");
  // -----------------------

  // Determine scaling. 1u = scale pixels.
  // Max units in a row is approx 15-16.
  int scale = (term_w - 6) / 16;
  if (scale < 3)
    scale = 3;

  render_key_count = 0;
  int total_rows = 6;
  // Shift down by 4 rows to make room for Exit Button
  int ky_start = term_h - (total_rows * 3) - 1;
  if (ky_start < 5)
    ky_start = 5;

  for (int r = 0; r < total_rows; r++) {
    int ky = ky_start + (r * 3);
    int kx = 2 + (int)(row_offsets[r] * scale);

    for (int k = 0; layout[r][k].label != NULL; k++) {
      Key *key = &layout[r][k];
      int kw = (int)(key->width * scale);
      if (kw < 2)
        kw = 2;
      int kh = 3;

      RenderKey *rk = &render_keys[render_key_count++];
      strncpy(rk->label, key->label, sizeof(rk->label));
      strncpy(rk->cmd, key->cmd, sizeof(rk->cmd));
      rk->x = kx;
      rk->y = ky;
      rk->w = kw;
      rk->h = kh;

      uintattr_t bg = TB_DEFAULT;
      uintattr_t fg = TB_WHITE;

      if (strcmp(rk->cmd, "CTRL") == 0 && (active_mods & MOD_CTRL_LEFT)) {
        bg = TB_YELLOW;
        fg = TB_BLACK;
      }
      if (strcmp(rk->cmd, "SHIFT") == 0 && (active_mods & MOD_SHIFT_LEFT)) {
        bg = TB_YELLOW;
        fg = TB_BLACK;
      }
      if (strcmp(rk->cmd, "ALT") == 0 && (active_mods & MOD_ALT_LEFT)) {
        bg = TB_YELLOW;
        fg = TB_BLACK;
      }
      if (strcmp(rk->cmd, "WIN") == 0 && (active_mods & MOD_GUI_LEFT)) {
        bg = TB_YELLOW;
        fg = TB_BLACK;
      }

      // Special coloring for big keys
      if (key->width > 1.25 && bg == TB_DEFAULT) {
        bg = TB_BLUE;
        fg = TB_WHITE;
      }

      draw_box(kx, ky, kw, kh, fg, bg);

      int label_len = strlen(rk->label);
      int lx = kx + (kw - label_len) / 2;
      int ly = ky + 1;
      draw_text(lx, ly, fg | TB_BOLD, bg, rk->label);

      kx += kw + 0; // No spacing between boxes since borders touch
    }
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
    mods_str[strlen(mods_str) - 1] = '\0';

  send_key_sequence(mods_str[0] ? mods_str : NULL, cmd);

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

  tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE | TB_INPUT_ALT);

  while (1) {
    render_keyboard();

    struct tb_event ev;
    int res = tb_poll_event(&ev);
    if (res < 0)
      break;

    if (ev.type == TB_EVENT_KEY) {
      // Exit sequences
      if (ev.key == TB_KEY_CTRL_C)
        break;

      if (ev.key == TB_KEY_ESC) {
        handle_input("ESC");
        continue;
      }
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
        // CHECK EXIT BUTTON CLICK
        int term_w = tb_width();
        int btn_w = 14;
        int btn_x = (term_w - btn_w) / 2;
        int btn_y = 1;
        // Button height is 3 (y=1 to y=3)
        if (ev.x >= btn_x && ev.x < btn_x + btn_w && ev.y >= btn_y &&
            ev.y < btn_y + 3) {
          break; // EXIT
        }

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
      // Handled in next loop
    }
  }

  tb_shutdown();
  return EXIT_SUCCESS;
}
