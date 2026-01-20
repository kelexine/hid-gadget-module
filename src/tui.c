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

// External declarations for Mouse functions (from hid-gadget.c)
extern int send_mouse_move(int8_t x, int8_t y);
extern int send_mouse_click(uint8_t button);
extern int send_mouse_press(uint8_t button);
extern int send_mouse_release(void);

// Constants
#define MOUSE_BTN_LEFT 1
#define MOUSE_BTN_RIGHT 2
#define MOUSE_BTN_MIDDLE 4

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

// State for Analog Stick
static int sensitivity = 5;
static int last_mx = -1;
static int last_my = -1;
static int mouse_btn_state = 0;        // Temporary click state (bits 1,2,4)
static uint8_t held_mouse_buttons = 0; // Latched click state

typedef struct {
  int term_w, term_h;
  // Exit Button
  int btn_x, btn_y, btn_w, btn_h;
  // Media Keys Area
  int media_y;
  // Radar Zone
  int rad_x, rad_y, rad_w, rad_h;
  int rad_cx, rad_cy; // Center
  // Controls
  int ctrl_x, ctrl_y;
  int hold_y;
  // Keyboard
  int key_start_y;
  int scale;
} Layout;

static Layout calc_layout() {
  Layout l;
  l.term_w = tb_width();
  l.term_h = tb_height();

  // Exit Button
  l.btn_w = 14;
  l.btn_h = 3;
  l.btn_x = (l.term_w - l.btn_w) / 2;
  l.btn_y = 1;

  // Media Keys area starts below Exit
  l.media_y = 5;

  // Bottom-up calculation
  int keyboard_rows = 6;
  int row_height = 3;
  int key_total_h = keyboard_rows * row_height;

  l.key_start_y = l.term_h - key_total_h - 1;

  // Controls take 2 rows of 3 lines + spacing
  l.hold_y = l.key_start_y - 4;
  l.ctrl_y = l.hold_y - 3;

  // Available space for Radar
  int top_limit = l.media_y + 7; // 2 rows of media = 6 + 1
  int bottom_limit = l.ctrl_y - 1;
  int avail_h = bottom_limit - top_limit;

  if (avail_h < 5) {
    avail_h = 5; // Minimum fallback
  }

  // Radar Zone
  l.rad_w = l.term_w - 4;
  if (l.rad_w > 80)
    l.rad_w = 80;
  l.rad_h = avail_h;

  l.rad_x = (l.term_w - l.rad_w) / 2;
  l.rad_y = top_limit;

  l.rad_cx = l.rad_x + l.rad_w / 2;
  l.rad_cy = l.rad_y + l.rad_h / 2;

  // Mouse Controls (L/M/R/Sens)
  int ctrl_w = 52;
  l.ctrl_x = (l.term_w - ctrl_w) / 2;

  l.scale = (l.term_w - 6) / 16;
  if (l.scale < 3)
    l.scale = 3;

  return l;
}

static int clamp_to_int8(int val) {
  if (val > 127)
    return 127;
  if (val < -127)
    return -127;
  return val;
}

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
  Layout l = calc_layout();

  // Header Title
  draw_text(2, 0, TB_WHITE | TB_BOLD, TB_DEFAULT,
            "HID INDUSTRIAL v1.38.2 | Tap EXIT or Ctrl+C to quit");

  // --- EXIT BUTTON ---
  draw_box(l.btn_x, l.btn_y, l.btn_w, l.btn_h, TB_WHITE | TB_BOLD, TB_RED);
  draw_text(l.btn_x + 3, l.btn_y + 1, TB_WHITE | TB_BOLD, TB_RED, "[ EXIT ]");

  // --- MEDIA KEYS ---
  struct MediaKey {
    const char *label;
    const char *cmd;
    uintattr_t fg;
  };

  struct MediaKey media1[] = {{"\u23EA", "REWIND", TB_CYAN},   // ⏪
                              {"\u23EE", "PREVIOUS", TB_CYAN}, // ⏮
                              {"\u25B6", "PLAY", TB_GREEN},    // ▶ (Play)
                              {"\u23ED", "NEXT", TB_CYAN},     // ⏭
                              {"\u23E9", "FORWARD", TB_CYAN},  // ⏩
                              {"\U0001F509", "VOL-", TB_BLUE}, // 🔉
                              {"\U0001F50A", "VOL+", TB_BLUE}, // 🔊
                              {"\U0001F507", "MUTE", TB_RED | TB_BOLD}, // 🔇
                              {NULL, NULL, 0}};

  struct MediaKey media2[] = {
      {"\u23FA", "RECORD", TB_RED},                   // ⏺
      {"\u23F8", "PAUSE", TB_YELLOW},                 // ⏸
      {"\u23F9", "STOP", TB_RED},                     // ⏹
      {"\u23CF", "EJECT", TB_MAGENTA},                // ⏏
      {"\U0001F505", "BRIGHTNESS-", TB_YELLOW},       // 🔅
      {"\u2600", "BRIGHTNESS+", TB_YELLOW | TB_BOLD}, // ☀
      {NULL, NULL, 0},
      {NULL, NULL, 0},
      {NULL, NULL, 0}}; // Padding to match loop

  int mw = 8;
  int mx = (l.term_w - 8 * mw) / 2;
  if (mx < 0)
    mx = 0;

  for (int i = 0; media1[i].label; i++) {
    draw_box(mx + i * mw, l.media_y, mw - 1, 3, media1[i].fg, TB_DEFAULT);
    // Center the icon (roughly)
    draw_text(mx + i * mw + 2, l.media_y + 1, media1[i].fg, TB_DEFAULT,
              media1[i].label);
  }
  for (int i = 0; media2[i].label; i++) {
    draw_box(mx + i * mw, l.media_y + 3, mw - 1, 3, media2[i].fg, TB_DEFAULT);
    draw_text(mx + i * mw + 2, l.media_y + 4, media2[i].fg, TB_DEFAULT,
              media2[i].label);
  }

  // --- RADAR ZONE ---
  draw_box(l.rad_x, l.rad_y, l.rad_w, l.rad_h, TB_CYAN, TB_DEFAULT);
  int cx = l.rad_cx;
  int cy = l.rad_cy;

  tb_set_cell(cx, cy, '+', TB_CYAN | TB_BOLD, TB_DEFAULT);
  for (int i = 1; i <= 4; i++) {
    if (cx - i > l.rad_x)
      tb_set_cell(cx - i, cy, '-', TB_CYAN, TB_DEFAULT);
    if (cx + i < l.rad_x + l.rad_w - 1)
      tb_set_cell(cx + i, cy, '-', TB_CYAN, TB_DEFAULT);
  }
  for (int i = 1; i <= 2; i++) {
    if (cy - i > l.rad_y)
      tb_set_cell(cx, cy - i, '|', TB_CYAN, TB_DEFAULT);
    if (cy + i < l.rad_y + l.rad_h - 1)
      tb_set_cell(cx, cy + i, '|', TB_CYAN, TB_DEFAULT);
  }

  if (last_mx != -1 && last_my != -1) {
    if (last_mx >= l.rad_x && last_mx < l.rad_x + l.rad_w &&
        last_my >= l.rad_y && last_my < l.rad_y + l.rad_h) {
      tb_set_cell(last_mx, last_my, 0x25C9, TB_WHITE | TB_BOLD, TB_RED); // ◉
    }
  } else if (l.rad_h > 4) {
    draw_text(cx - 3, cy + 2, TB_CYAN, TB_DEFAULT, "ANALOG");
  }

  // --- MOUSE CONTROLS ---
  int bx = l.ctrl_x;
  int cy_ctrl = l.ctrl_y;
  int hy = l.hold_y;

  // [ L ] / [ HL ] - Green / Magenta
  uintattr_t l_fg = TB_WHITE | TB_BOLD;
  uintattr_t l_bg = TB_GREEN;
  if (mouse_btn_state & 1) {
    l_fg = TB_GREEN | TB_BOLD;
    l_bg = TB_WHITE;
  }
  draw_box(bx, cy_ctrl, 7, 3, l_fg, l_bg);
  draw_text(bx + 2, cy_ctrl + 1, l_fg, l_bg, "L");

  uintattr_t hl_fg = TB_WHITE;
  uintattr_t hl_bg = TB_DEFAULT;
  if (held_mouse_buttons & MOUSE_BTN_LEFT) {
    hl_fg = TB_WHITE | TB_BOLD;
    hl_bg = TB_MAGENTA;
  }
  draw_box(bx, hy, 7, 3, hl_fg, hl_bg);
  draw_text(bx + 2, hy + 1, hl_fg, hl_bg, "HL");
  bx += 8;

  // [ M ] / [ HM ] - Yellow / Magenta
  uintattr_t m_fg = TB_BLACK | TB_BOLD;
  uintattr_t m_bg = TB_YELLOW;
  if (mouse_btn_state & 2) {
    m_fg = TB_YELLOW | TB_BOLD;
    m_bg = TB_BLACK;
  }
  draw_box(bx, cy_ctrl, 7, 3, m_fg, m_bg);
  draw_text(bx + 2, cy_ctrl + 1, m_fg, m_bg, "M");

  uintattr_t hm_fg = TB_WHITE;
  uintattr_t hm_bg = TB_DEFAULT;
  if (held_mouse_buttons & MOUSE_BTN_MIDDLE) {
    hm_fg = TB_WHITE | TB_BOLD;
    hm_bg = TB_MAGENTA;
  }
  draw_box(bx, hy, 7, 3, hm_fg, hm_bg);
  draw_text(bx + 2, hy + 1, hm_fg, hm_bg, "HM");
  bx += 8;

  // [ R ] / [ HR ] - Red / Magenta
  uintattr_t r_fg = TB_WHITE | TB_BOLD;
  uintattr_t r_bg = TB_RED;
  if (mouse_btn_state & 4) {
    r_fg = TB_RED | TB_BOLD;
    r_bg = TB_WHITE;
  }
  draw_box(bx, cy_ctrl, 7, 3, r_fg, r_bg);
  draw_text(bx + 2, cy_ctrl + 1, r_fg, r_bg, "R");

  uintattr_t hr_fg = TB_WHITE;
  uintattr_t hr_bg = TB_DEFAULT;
  if (held_mouse_buttons & MOUSE_BTN_RIGHT) {
    hr_fg = TB_WHITE | TB_BOLD;
    hr_bg = TB_MAGENTA;
  }
  draw_box(bx, hy, 7, 3, hr_fg, hr_bg);
  draw_text(bx + 2, hy + 1, hr_fg, hr_bg, "HR");
  bx += 10;

  // Sensitivity
  draw_text(bx, cy_ctrl + 1, TB_WHITE, TB_DEFAULT, "SENS:");
  bx += 6;
  draw_box(bx, cy_ctrl, 5, 3, TB_WHITE, TB_DEFAULT);
  draw_text(bx + 1, cy_ctrl + 1, TB_WHITE, TB_DEFAULT, "-");
  bx += 6;
  char sens_str[4];
  sprintf(sens_str, "%d", sensitivity);
  draw_text(bx, cy_ctrl + 1, TB_YELLOW | TB_BOLD, TB_DEFAULT, sens_str);
  bx += 3;
  draw_box(bx, cy_ctrl, 5, 3, TB_WHITE, TB_DEFAULT);
  draw_text(bx + 1, cy_ctrl + 1, TB_WHITE, TB_DEFAULT, "+");
  // -------------------------------

  render_key_count = 0;
  int total_rows = 6;

  for (int r = 0; r < total_rows; r++) {
    int ky = l.key_start_y + (r * 3);
    int kx = 2 + (int)(row_offsets[r] * l.scale);

    for (int k = 0; layout[r][k].label != NULL; k++) {
      Key *key = &layout[r][k];
      int kw = (int)(key->width * l.scale);
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

      if (key->width > 1.25 && bg == TB_DEFAULT) {
        bg = TB_BLUE;
        fg = TB_WHITE;
      }

      draw_box(kx, ky, kw, kh, fg, bg);

      int label_len = strlen(rk->label);
      int lx = kx + (kw - label_len) / 2;
      int ly = ky + 1;
      draw_text(lx, ly, fg | TB_BOLD, bg, rk->label);

      kx += kw + 0;
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
      Layout l = calc_layout();

      // Mouse Release Logic: Reset markers and button state
      if (ev.key == TB_KEY_MOUSE_RELEASE) {
        if (last_mx != -1 && last_my != -1) {
          int dx = last_mx - l.rad_cx;
          int dy = (last_my - l.rad_cy) * 2;
          if (dx * dx + dy * dy < 6 * 6)
            send_mouse_click(MOUSE_BTN_LEFT);
        }

        last_mx = -1;
        last_my = -1;
        mouse_btn_state = 0;
        send_mouse_press(held_mouse_buttons); // Re-assert permanent hold
        continue;
      }

      // 1. EXIT BUTTON check
      if (ev.key == TB_KEY_MOUSE_LEFT && ev.x >= l.btn_x &&
          ev.x < l.btn_x + l.btn_w && ev.y >= l.btn_y &&
          ev.y < l.btn_y + l.btn_h) {
        break; // EXIT
      }

      // Media Keys handling
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        struct {
          const char *label;
          const char *cmd;
        } m1[] = {{"RWD", "REWIND"}, {"PREV", "PREVIOUS"}, {"PLAY", "PLAY"},
                  {"NEXT", "NEXT"},  {"FWD", "FORWARD"},   {"V-", "VOL-"},
                  {"V+", "VOL+"},    {"MUTE", "MUTE"},     {NULL, NULL}};
        struct {
          const char *label;
          const char *cmd;
        } m2[] = {
            {"REC", "RECORD"}, {"PAUS", "PAUSE"},      {"STOP", "STOP"},
            {"EJ", "EJECT"},   {"BR-", "BRIGHTNESS-"}, {"BR+", "BRIGHTNESS+"},
            {NULL, NULL}};
        int mw = 8;
        int mx_m = (l.term_w - 8 * mw) / 2;
        if (mx_m < 0)
          mx_m = 0;

        if (ev.y >= l.media_y && ev.y < l.media_y + 3) {
          int idx = (ev.x - mx_m) / mw;
          if (idx >= 0 && idx < 8)
            send_key_sequence(NULL, m1[idx].cmd);
        } else if (ev.y >= l.media_y + 3 && ev.y < l.media_y + 6) {
          int idx = (ev.x - mx_m) / mw;
          if (idx >= 0 && idx < 7)
            send_key_sequence(NULL, m2[idx].cmd);
        }
      }

      // 2. RADAR ANALOG check
      if (ev.key == TB_KEY_MOUSE_LEFT && ev.x >= l.rad_x &&
          ev.x < l.rad_x + l.rad_w && ev.y >= l.rad_y &&
          ev.y < l.rad_y + l.rad_h) {

        last_mx = ev.x;
        last_my = ev.y;

        // Vector Calc
        int raw_dx = ev.x - l.rad_cx;
        int raw_dy = ev.y - l.rad_cy;

        // Apply aspect ratio correction for MAGNITUDE (speed)
        // Vertical chars are 2x pixels.
        // So "Visual Distance" = sqrt(dx^2 + (dy*2)^2)
        // But output "Delta" needs to push pixels.
        // Tapping 1 row down (dy=1) should be "fast" if 1 row is "far".

        // Simple Linear Scaling:
        // x_speed = raw_dx * sensitivity
        // y_speed = raw_dy * sensitivity * 2 (Boost vertical pixel speed to
        // match visual effort)

        // Deadzone check for movement prevention
        int vis_dy = raw_dy * 2;
        int dist_sq = raw_dx * raw_dx + vis_dy * vis_dy;

        if (dist_sq >= 4 * 4) { // Only move if outside inner deadzone
          int move_x = clamp_to_int8(raw_dx * sensitivity);
          int move_y = clamp_to_int8(raw_dy * sensitivity * 2);
          send_mouse_move(move_x, move_y);
        }
      }

      // 3. MOUSE CONTROLS check
      if (ev.key == TB_KEY_MOUSE_LEFT) {
        int bx = l.ctrl_x;
        int cyc = l.ctrl_y;
        int hy = l.hold_y;

        // [ L ] / [ HL ]
        if (ev.x >= bx && ev.x < bx + 7) {
          if (ev.y >= cyc && ev.y < cyc + 3) {
            send_mouse_press(held_mouse_buttons | MOUSE_BTN_LEFT);
            mouse_btn_state |= 1;
          }
          if (ev.y >= hy && ev.y < hy + 3) {
            held_mouse_buttons ^= MOUSE_BTN_LEFT;
            send_mouse_press(held_mouse_buttons);
          }
        }
        bx += 8;

        // [ M ] / [ HM ]
        if (ev.x >= bx && ev.x < bx + 7) {
          if (ev.y >= cyc && ev.y < cyc + 3) {
            send_mouse_press(held_mouse_buttons | MOUSE_BTN_MIDDLE);
            mouse_btn_state |= 2;
          }
          if (ev.y >= hy && ev.y < hy + 3) {
            held_mouse_buttons ^= MOUSE_BTN_MIDDLE;
            send_mouse_press(held_mouse_buttons);
          }
        }
        bx += 8;

        // [ R ] / [ HR ]
        if (ev.x >= bx && ev.x < bx + 7) {
          if (ev.y >= cyc && ev.y < cyc + 3) {
            send_mouse_press(held_mouse_buttons | MOUSE_BTN_RIGHT);
            mouse_btn_state |= 4;
          }
          if (ev.y >= hy && ev.y < hy + 3) {
            held_mouse_buttons ^= MOUSE_BTN_RIGHT;
            send_mouse_press(held_mouse_buttons);
          }
        }
        bx += 11; // Spacing

        // Sens [-]
        bx += 6;
        if (ev.x >= bx && ev.x < bx + 5 && ev.y >= cyc && ev.y < cyc + 3) {
          if (sensitivity > 1)
            sensitivity--;
        }
        bx += 9;
        // Sens [+]
        if (ev.x >= bx && ev.x < bx + 5 && ev.y >= cyc && ev.y < cyc + 3) {
          if (sensitivity < 20)
            sensitivity++;
        }
      }

      // 4. KEYBOARD KEYS check
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
      // Handled in next loop
    }
  }

  tb_shutdown();
  return EXIT_SUCCESS;
}
