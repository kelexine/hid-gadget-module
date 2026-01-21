/*
 * hid-gadget.c - USB HID Gadget driver for Android (Modified for Dynamic
 * Discovery)
 *
 * This program provides a user-space interface to USB HID gadget
 * functionality, allowing the device to act as a USB keyboard, mouse,
 * or consumer control device.
 * Dynamically finds the first three /dev/hidg* devices.
 */

#include "../include/ducky.h"
#include "../include/hid_interface.h"
#include "../include/tui.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* If the kernel headers are not available, we define our own structures */
#ifndef HID_MAX_DESCRIPTOR_SIZE
#define HID_MAX_DESCRIPTOR_SIZE 4096
#endif

// --- Dynamic Device Paths (Global Variables) ---
char *g_keyboard_device = NULL;
char *g_mouse_device = NULL;
char *g_consumer_device = NULL;
// --- End Dynamic Device Paths ---

// --- Persistent File Descriptors ---
static int g_fd_keyboard = -1;
static int g_fd_mouse = -1;
static int g_fd_consumer = -1;

static int get_cached_fd(const char *path, int *cached_fd) {
  if (!path)
    return -1;
  if (*cached_fd >= 0)
    return *cached_fd;
  *cached_fd = open(path, O_RDWR);
  return *cached_fd;
}

void close_hid_fds() {
  if (g_fd_keyboard >= 0) {
    close(g_fd_keyboard);
    g_fd_keyboard = -1;
  }
  if (g_fd_mouse >= 0) {
    close(g_fd_mouse);
    g_fd_mouse = -1;
  }
  if (g_fd_consumer >= 0) {
    close(g_fd_consumer);
    g_fd_consumer = -1;
  }
}
// --- End Persistent File Descriptors ---

/* Keyboard report descriptor */
#define KEYBOARD_REPORT_SIZE 8

/* Mouse report descriptor */
#ifdef MOCK_HID
static int mock_write(int fd, const void *buf, size_t count) {
  const uint8_t *report = (const uint8_t *)buf;
  printf("[HID-MOCK] Writing %zu bytes: ", count);
  for (size_t i = 0; i < count; i++) {
    printf("%02X ", report[i]);
  }
  printf("\n");
  return (int)count;
}
#define write mock_write
#endif

#define MOUSE_REPORT_SIZE 4 /* Buttons(1), X(1), Y(1), Wheel(1) */
static int g_mouse_report_size = MOUSE_REPORT_SIZE;
static int g_mouse_support_hscroll = 0;

/* Consumer control report descriptor */
#define CONSUMER_REPORT_SIZE 2

/* Keyboard modifier masks */
#define MOD_CTRL_LEFT (1 << 0)
#define MOD_SHIFT_LEFT (1 << 1)
#define MOD_ALT_LEFT (1 << 2)
#define MOD_GUI_LEFT (1 << 3)
#define MOD_CTRL_RIGHT (1 << 4)
#define MOD_SHIFT_RIGHT (1 << 5)
#define MOD_ALT_RIGHT (1 << 6)
#define MOD_GUI_RIGHT (1 << 7)

/* Mouse button masks */
#define MOUSE_BTN_LEFT (1 << 0)
#define MOUSE_BTN_RIGHT (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

/* Keyboard usage table - mapping ASCII characters to HID usage codes */
static const uint8_t usage_table_us[128] = {
    0,  0,  0,  0,
    0,  0,  0,  0, /* 0-7 */
    42, 43, 40, 0,
    0,  0,  0,  0, /* 8-15 (Backspace, Tab, Enter) */
    0,  0,  0,  0,
    0,  0,  0,  0, /* 16-23 */
    0,  0,  0,  41,
    0,  0,  0,  0, /* 24-31 (Escape) */
    44, 30, 52, 32,
    33, 34, 35, 52, /* 32-39 (Space, !, ", #, $, %, &, ') */
    38, 39, 37, 46,
    54, 45, 55, 56, /* 40-47 ((, ), *, +, ,, -, ., /) */
    39, 30, 31, 32,
    33, 34, 35, 36, /* 48-55 (0-7) */
    37, 38, 51, 51,
    54, 46, 55, 56, /* 56-63 (8, 9, :, ;, <, =, >, ?) */
    31, 4,  5,  6,
    7,  8,  9,  10, /* 64-71 (@(Shift+2),A-G) */
    11, 12, 13, 14,
    15, 16, 17, 18, /* 72-79 (H-O) */
    19, 20, 21, 22,
    23, 24, 25, 26, /* 80-87 (P-W) */
    27, 28, 29, 47,
    49, 48, 33, 38, /* 88-95 (X-Z,[,\|],^ (Shift+6)) - Adjusted */
    53, 4,  5,  6,
    7,  8,  9,  10, /* 96-103 (`,a-g) */
    11, 12, 13, 14,
    15, 16, 17, 18, /* 104-111 (h-o) */
    19, 20, 21, 22,
    23, 24, 25, 26, /* 112-119 (p-w) */
    27, 28, 29, 47,
    49, 48, 53, 0 /* 120-127 (x-z,{,|,},~) - Adjusted */
};

/* Shift needed for these characters (US layout assumed) */
static const char *shift_chars_us =
    "!@#$%^&*()_+{}|:\"<>?~ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const uint8_t *current_usage_table = NULL;
const char *current_shift_chars = NULL;

/* Function key mapping */
struct fn_key {
  const char *name;
  uint8_t usage;
};

static const struct fn_key fn_keys[] = {
    {"F1", 58},     {"F2", 59},        {"F3", 60},          {"F4", 61},
    {"F5", 62},     {"F6", 63},        {"F7", 64},          {"F8", 65},
    {"F9", 66},     {"F10", 67},       {"F11", 68},         {"F12", 69},
    {"INSERT", 73}, {"HOME", 74},      {"PAGEUP", 75},      {"DELETE", 76},
    {"END", 77},    {"PAGEDOWN", 78},  {"RIGHT", 79},       {"LEFT", 80},
    {"DOWN", 81},   {"UP", 82},        {"NUMLOCK", 83},     {"ESC", 41},
    {"TAB", 43},    {"CAPSLOCK", 57},  {"PRINTSCREEN", 70}, {"SCROLLLOCK", 71},
    {"PAUSE", 72},  {"BACKSPACE", 42}, {"RETURN", 40},      {"ENTER", 40},
    {"SPACE", 44},  {NULL, 0}};

/* Consumer control key mapping */
struct consumer_key {
  const char *name;
  uint16_t usage;
};

static const struct consumer_key consumer_keys[] = {
    {"PLAY", 0x00B0},        {"PAUSE", 0x00B1},       {"RECORD", 0x00B2},
    {"FORWARD", 0x00B3},     {"REWIND", 0x00B4},      {"NEXT", 0x00B5},
    {"PREVIOUS", 0x00B6},    {"STOP", 0x00B7},        {"EJECT", 0x00B8},
    {"MUTE", 0x00E2},        {"VOL+", 0x00E9},        {"VOL-", 0x00EA},
    {"BRIGHTNESS+", 0x006F}, {"BRIGHTNESS-", 0x0070}, {NULL, 0}};

// Structure to hold device info for sorting
int set_hid_locale(const char *name) {
  if (strcasecmp(name, "US") == 0) {
    current_usage_table = usage_table_us;
    current_shift_chars = shift_chars_us;
    return 0;
  }
  fprintf(stderr,
          "[HID-HW] Locale '%s' not supported yet. Falling back to US.\n",
          name);
  return -1;
}

int send_raw_hid_report(const uint8_t *report, size_t size) {
  if (!g_keyboard_device)
    return -1;
  int fd = get_cached_fd(g_keyboard_device, &g_fd_keyboard);
  if (fd < 0)
    return -1;
  int n = write(fd, report, size);
  return (n == (int)size) ? 0 : -1;
}
typedef struct {
  char name[NAME_MAX]; // From <dirent.h>
  int number;
} hidg_device_info;

// Comparison function for qsort
int compare_hidg_devices(const void *a, const void *b) {
  const hidg_device_info *dev_a = (const hidg_device_info *)a;
  const hidg_device_info *dev_b = (const hidg_device_info *)b;
  return dev_a->number - dev_b->number;
}

// Function to find and assign the first three hidg devices
int find_hidg_devices() {
  DIR *dir;
  struct dirent *entry;
  hidg_device_info devices[32]; // Assume max 32 hidg devices
  int count = 0;
  const char *dev_dir = "/dev";
  const char *prefix = "hidg";
  size_t prefix_len = strlen(prefix);

  dir = opendir(dev_dir);
  if (!dir) {
    perror("Error opening /dev directory");
    return -1;
  }

  while ((entry = readdir(dir)) != NULL && count < 32) {
    // Check if filename starts with "hidg" and is followed by digits
    if (strncmp(entry->d_name, prefix, prefix_len) == 0 &&
        isdigit(entry->d_name[prefix_len])) {
      // Check if it's a character device (optional but good practice)
      char full_path[PATH_MAX];
      struct stat st;
      snprintf(full_path, sizeof(full_path), "%s/%s", dev_dir, entry->d_name);
      if (stat(full_path, &st) == 0 && S_ISCHR(st.st_mode)) {
        // Extract the number
        if (sscanf(entry->d_name + prefix_len, "%d", &devices[count].number) ==
            1) {
          snprintf(devices[count].name, NAME_MAX, "%s", entry->d_name);
          // strncpy(devices[count].name, entry->d_name, NAME_MAX - 1);
          // devices[count].name[NAME_MAX - 1] = '\0'; // Ensure null termination
          count++;
        }
      }
    }
  }
  closedir(dir);

  if (count < 3) {
    // Not fatal: caller may only need a subset (e.g., keyboard or mouse)
    // Return the number found for the caller to decide.
  }

  // Sort the devices by number
  qsort(devices, count, sizeof(hidg_device_info), compare_hidg_devices);

  // Construct full paths and assign to global variables
  char path_buffer[PATH_MAX];

  // Only fill device paths that haven't been set by environment overrides
  if (count >= 1 && g_keyboard_device == NULL) {
    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir,
             devices[0].name);
    g_keyboard_device = strdup(path_buffer);
  }
  if (count >= 2 && g_mouse_device == NULL) {
    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir,
             devices[1].name);
    g_mouse_device = strdup(path_buffer);
  }
  if (count >= 3 && g_consumer_device == NULL) {
    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir,
             devices[2].name);
    g_consumer_device = strdup(path_buffer);
  }

  // Check if strdup succeeded
  if ((count >= 1 && !g_keyboard_device) || (count >= 2 && !g_mouse_device) ||
      (count >= 3 && !g_consumer_device)) {
    perror("Error allocating memory for device paths");
    // Free any allocated memory before returning
    free(g_keyboard_device);
    g_keyboard_device = NULL;
    free(g_mouse_device);
    g_mouse_device = NULL;
    free(g_consumer_device);
    g_consumer_device = NULL;
    return -1;
  }

  if (count == 0) {
#ifdef MOCK_HID
    printf("[HID-MOCK] Mocking device paths for testing.\n");
    g_keyboard_device = strdup("/dev/null");
    g_mouse_device = strdup("/dev/null");
    g_consumer_device = strdup("/dev/null");
    return 3;
#else
    return 0;
#endif
  }

  return count;
}

// Function to free allocated device path strings
void cleanup_device_paths() {
  free(g_keyboard_device);
  free(g_mouse_device);
  free(g_consumer_device);
  g_keyboard_device = NULL;
  g_mouse_device = NULL;
  g_consumer_device = NULL;
}

/* Load device overrides from environment variables if provided */
void load_env_devices() {
  const char *kb = getenv("HID_KEYBOARD_DEV");
  const char *ms = getenv("HID_MOUSE_DEV");
  const char *cc = getenv("HID_CONSUMER_DEV");
  struct stat st;
  if (kb && stat(kb, &st) == 0 && S_ISCHR(st.st_mode)) {
    g_keyboard_device = strdup(kb);
  }
  if (ms && stat(ms, &st) == 0 && S_ISCHR(st.st_mode)) {
    g_mouse_device = strdup(ms);
  }
  if (cc && stat(cc, &st) == 0 && S_ISCHR(st.st_mode)) {
    g_consumer_device = strdup(cc);
  }
}

/* Native Auto-Recovery for Android/Magisk */
void attempt_hid_recovery() {
  static int recovery_attempted = 0;
  if (recovery_attempted)
    return;
  recovery_attempted = 1;

  if (access("/system/bin/hid-setup", F_OK) == 0) {
    fprintf(
        stderr,
        "\x1b[1;33m[!] HID devices missing. Attempting auto-fix...\x1b[0m\n");
    // We attempt to run the setup script which handles UDC binding
    int ret = system("setprop sys.usb.config hid && /system/bin/hid-setup");
    if (ret == 0) {
      // Small delay for udev/devd to settle nodes
      usleep(250000);
      find_hidg_devices();
      if (g_keyboard_device || g_mouse_device || g_consumer_device) {
        fprintf(stderr, "\x1b[1;32m[+] Auto-fix successful. HID devices "
                        "restored.\x1b[0m\n");
      }
    }
  }
}

void print_usage(const char *prog_name) {
  if (!g_keyboard_device && !g_mouse_device && !g_consumer_device) {
    find_hidg_devices();
  }

  fprintf(stderr, "\n\x1b[1;"
                  "36m┌────────────────────────────────────────────────────────"
                  "─┐\x1b[0m\n");
  fprintf(stderr, "\x1b[1;36m│ \x1b[1;37m📱 HID GADGET CONTROLLER "
                  "\x1b[1;33mv1.38.2\x1b[1;36m                 │\x1b[0m\n");
  fprintf(stderr, "\x1b[1;"
                  "36m└────────────────────────────────────────────────────────"
                  "─┘\x1b[0m\n");

  fprintf(stderr,
          "\n\x1b[1;37mUSAGE:\x1b[0m %s \x1b[1;32m<command>\x1b[0m [options]\n",
          prog_name);

  fprintf(stderr, "\n\x1b[1;34m[ ⌨️  KEYBOARD ]\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;32mkeyboard\x1b[0m "
                  "[\x1b[1;35m--hold\x1b[0m|\x1b[1;35m--release\x1b[0m] "
                  "[\x1b[1;33mmodifiers\x1b[0m] \x1b[1;37m<sequence>\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;30mDescription:\x1b[0m Sends text or raw key "
                  "combos to the target.\n");
  fprintf(stderr, "  \x1b[1;30mModifiers:\x1b[0m   CTRL, ALT, SHIFT, GUI "
                  "(Prefix 'R' for Right-side keys)\n");
  fprintf(stderr, "  \x1b[1;30mSpecials:\x1b[0m    F1-F12, ESC, TAB, ENTER, "
                  "SPACE, UP, DOWN, LEFT, RIGHT\n");
  fprintf(stderr, "  \x1b[1;30mExample:\x1b[0m     %s keyboard CTRL-ALT-DEL\n",
          prog_name);

  fprintf(stderr, "\n\x1b[1;35m[ 🖱️  MOUSE ]\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;32mmouse move\x1b[0m \x1b[1;37m<X> <Y>\x1b[0m      "
                  "- Relative motion (-127 to 127)\n");
  fprintf(stderr, "  \x1b[1;32mmouse click\x1b[0m \x1b[1;37m[btn]\x1b[0m     - "
                  "left (default), right, middle\n");
  fprintf(stderr, "  \x1b[1;32mmouse scroll\x1b[0m \x1b[1;37m<V> [H]\x1b[0m  - "
                  "Scroll vertical/horizontal\n");
  fprintf(stderr, "  \x1b[1;32mmouse down/up\x1b[0m         - Latch/Release "
                  "specific buttons\n");

  fprintf(stderr, "\n\x1b[1;31m[ 🎵 MEDIA ]\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;32mconsumer\x1b[0m \x1b[1;37m<action>\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;30mActions:\x1b[0m     PLAY, PAUSE, MUTE, VOL+, "
                  "VOL-, BRIGHTNESS+, BRIGHTNESS-\n");

  fprintf(stderr, "\n\x1b[1;33m[ 🦆 DUCKYSCRIPT 3.0 ]\x1b[0m\n");
  fprintf(
      stderr,
      "  \x1b[1;32mducky\x1b[0m \x1b[1;37m<path>\x1b[0m [\x1b[1;35m--os\x1b[0m "
      "\x1b[1;33mOS\x1b[0m]       - Load & Execute spec-compliant scripts.\n");
  fprintf(stderr, "  \x1b[1;30mOS Profiles:\x1b[0m WINDOWS (default), MACOS, "
                  "LINUX, ANDROID\n");
  fprintf(stderr, "  \x1b[1;30mInteractive:\x1b[0m Use '-' as path to read "
                  "from stdin (Ctrl+D to finish).\n");

  fprintf(stderr, "\n\x1b[1;32m[ 🖥️  INTERACTIVE TUI ]\x1b[0m\n");
  fprintf(stderr, "  \x1b[1;32mtui\x1b[0m                       - Launch full "
                  "terminal graphical remote\n");

  fprintf(stderr, "\n\x1b[1;30mFor advanced automation and variable docs, "
                  "visit the official README.\x1b[0m\n\n");

  exit(EXIT_FAILURE);
}

/* Function to parse modifier keys.
   If remainder is not NULL, and the string contains something that is not a
   modifier, it will point to the first non-modifier part in mod_str. */
uint8_t parse_modifiers(const char *mod_str, const char **remainder) {
  uint8_t modifiers = 0;
  if (remainder)
    *remainder = mod_str;

  char *mod_copy = strdup(mod_str);
  if (!mod_copy)
    return 0;

  char *token = strtok(mod_copy, "-");

  while (token != NULL) {
    uint8_t m = 0;
    if (strcasecmp(token, "CTRL") == 0 || strcasecmp(token, "CONTROL") == 0)
      m = MOD_CTRL_LEFT;
    else if (strcasecmp(token, "SHIFT") == 0)
      m = MOD_SHIFT_LEFT;
    else if (strcasecmp(token, "ALT") == 0)
      m = MOD_ALT_LEFT;
    else if (strcasecmp(token, "GUI") == 0 || strcasecmp(token, "WIN") == 0 ||
             strcasecmp(token, "META") == 0 || strcasecmp(token, "SUPER") == 0)
      m = MOD_GUI_LEFT;
    else if (strcasecmp(token, "RCTRL") == 0 ||
             strcasecmp(token, "RCONTROL") == 0)
      m = MOD_CTRL_RIGHT;
    else if (strcasecmp(token, "RSHIFT") == 0)
      m = MOD_SHIFT_RIGHT;
    else if (strcasecmp(token, "RALT") == 0)
      m = MOD_ALT_RIGHT;
    else if (strcasecmp(token, "RGUI") == 0 || strcasecmp(token, "RWIN") == 0 ||
             strcasecmp(token, "RMETA") == 0 ||
             strcasecmp(token, "RSUPER") == 0)
      m = MOD_GUI_RIGHT;

    if (m == 0) {
      /* This token is not a modifier. */
      if (remainder)
        *remainder = mod_str + (token - mod_copy);
      break;
    }

    modifiers |= m;

    /* Move remainder to after this token and hyphen if present */
    if (remainder) {
      *remainder = mod_str + (token - mod_copy) + strlen(token);
      if (**remainder == '-')
        (*remainder)++;
    }

    token = strtok(NULL, "-");
  }

  free(mod_copy);
  return modifiers;
}

/* Get function key usage code by name */
uint8_t get_fn_key_usage(const char *key_name) {
  int i;
  for (i = 0; fn_keys[i].name != NULL; i++) {
    if (strcasecmp(key_name, fn_keys[i].name) == 0) {
      return fn_keys[i].usage;
    }
  }
  return 0;
}

// Get consumer key usage code by name */
uint16_t get_consumer_key_usage(const char *key_name) {
  int i;
  for (i = 0; consumer_keys[i].name != NULL; i++) {
    if (strcasecmp(key_name, consumer_keys[i].name) == 0)
      return consumer_keys[i].usage;
  }
  return 0;
}

/* Internal function to send a raw keyboard report */
int send_keyboard_report(uint8_t modifiers, uint8_t key1, uint8_t key2,
                         uint8_t key3, uint8_t key4, uint8_t key5,
                         uint8_t key6) {
  if (!g_keyboard_device)
    return -1;

  int fd = get_cached_fd(g_keyboard_device, &g_fd_keyboard);
  if (fd < 0)
    return -1;

  uint8_t report[8] = {modifiers, 0, key1, key2, key3, key4, key5, key6};
  int ret = write(fd, report, 8);
  return (ret == 8) ? 0 : -1;
}

/* Send a single consumer report (press then release) */
int send_consumer_key(const char *action) {
  if (!g_consumer_device)
    return -1;
  uint16_t usage = get_consumer_key_usage(action);
  if (usage == 0)
    return -1;

  int fd = get_cached_fd(g_consumer_device, &g_fd_consumer);
  if (fd < 0)
    return -1;

  uint8_t report[2] = {usage & 0xFF, (usage >> 8) & 0xFF};
  write(fd, report, 2);
  usleep(50000); // 50ms tap
  memset(report, 0, 2);
  write(fd, report, 2);
  return 0;
}

/* Reusable function to send a key sequence with optional modifiers */
int send_key_sequence(const char *modifiers_str, const char *sequence) {
  if (!g_keyboard_device)
    return -1;

  // Check if the entire sequence is a consumer key first
  if (sequence && get_consumer_key_usage(sequence) != 0) {
    return send_consumer_key(sequence);
  }

  int fd = get_cached_fd(g_keyboard_device, &g_fd_keyboard);
  if (fd < 0)
    return -1;

  uint8_t modifiers = 0;
  if (modifiers_str) {
    modifiers = parse_modifiers(modifiers_str, NULL);
  }

  uint8_t report[8] = {0};
  report[0] = modifiers;

  if (sequence == NULL || strlen(sequence) == 0) {
    /* Just send modifiers */
    write(fd, report, 8);
    return 0;
  }

  /* Check for function key */
  uint8_t fn_usage = get_fn_key_usage(sequence);
  if (fn_usage != 0) {
    report[2] = fn_usage;
    write(fd, report, 8);
    /* Release if it's a one-shot */
    report[2] = 0;
    write(fd, report, 8);
  } else {
    /* Regular text */
    for (size_t i = 0; i < strlen(sequence); i++) {
      char c = sequence[i];
      uint8_t usage = 0;
      uint8_t current_mods = modifiers;

      if ((unsigned char)c < 128)
        usage = (current_usage_table ? current_usage_table
                                     : usage_table_us)[(unsigned char)c];

      if (usage != 0) {
        if (strchr(current_shift_chars ? current_shift_chars : shift_chars_us,
                   c))
          current_mods |= MOD_SHIFT_LEFT;

        report[0] = current_mods;
        report[2] = usage;
        write(fd, report, 8);

        /* Release */
        report[0] = modifiers;
        report[2] = 0;
        write(fd, report, 8);
      }
    }
  }

  /* Final release if modifiers were used and it's not a holding operation */
  if (modifiers != 0) {
    report[0] = 0;
    report[2] = 0;
    write(fd, report, 8);
  }

  return 0;
}

/* Process a keyboard sequence */
int process_keyboard(int argc, char *argv[]) {
  int fd;
  uint8_t report[KEYBOARD_REPORT_SIZE] = {0};
  int opt;
  int hold_keys = 0;
  int release_keys = 0;
  uint8_t modifiers = 0;
  int i, seq_start = 0;
  // Default delay per key (ms), can be overridden by env HID_KEY_DELAY_MS or
  // --delay wrapper
  int key_delay_ms = 10;
  {
    const char *env_delay = getenv("HID_KEY_DELAY_MS");
    if (env_delay) {
      int v = atoi(env_delay);
      if (v >= 0 && v <= 5000)
        key_delay_ms = v;
    }
  }

  // --- Check if device path is valid ---
  if (!g_keyboard_device) {
    fprintf(stderr, "Error: Keyboard device path not set.\n");
    return EXIT_FAILURE;
  }
  // ---

  static struct option long_options[] = {{"hold", no_argument, 0, 'h'},
                                         {"release", no_argument, 0, 'r'},
                                         {0, 0, 0, 0}};

  // Reset getopt for parsing within a function
  optind =
      1; // Important: Reset optind for getopt_long when called multiple times

  /* Parse command line options specific to keyboard */
  // Note: getopt_long modifies argc/argv ordering, parse options first
  while ((opt = getopt_long(argc, argv, "hr", long_options, NULL)) != -1) {
    switch (opt) {
    case 'h':
      hold_keys = 1;
      break;
    case 'r':
      release_keys = 1;
      break;
    // Handle '?' or ':' for unknown options or missing arguments if needed
    case '?':
    default:
      fprintf(stderr, "Invalid option in keyboard command.\n");
      // Consider printing keyboard-specific usage here
      // print_usage(argv[0]); // Might need adjustment if argv[0] isn't
      // program name
      return EXIT_FAILURE;
    }
  }

  /* Find where the sequence/modifiers start AFTER getopt processing */
  seq_start = optind; // optind is index of the first non-option argument

  /* Check for modifiers and sequence in positional arguments */
  const char *sequence = NULL;
  if (seq_start < argc) {
    const char *rem = NULL;
    uint8_t temp_mods = parse_modifiers(argv[seq_start], &rem);

    if (temp_mods != 0) {
      modifiers = temp_mods;
      if (rem && *rem != '\0') {
        /* String was "MOD-MOD-rest". Rest is the sequence. */
        sequence = rem;
      } else {
        /* String was purely modifiers. Consume this argument. */
        seq_start++;
        if (seq_start < argc) {
          sequence = argv[seq_start];
        }
      }
    } else {
      /* No modifiers found at start of first argument. */
      sequence = argv[seq_start];
    }
  }

  /* Open the HID keyboard device */
  fd = get_cached_fd(g_keyboard_device, &g_fd_keyboard);
  if (fd < 0) {
    fprintf(stderr, "Error opening HID keyboard device (%s): %s\n",
            g_keyboard_device, strerror(errno));
    return EXIT_FAILURE;
  }

  /* Set explicit modifiers in the report */
  report[0] = modifiers;

  /* Process key sequence or release */
  if (release_keys) {
    /* Release all keys and modifiers */
    memset(report, 0, KEYBOARD_REPORT_SIZE);
    if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
      fprintf(stderr, "Error writing keyboard release report: %s\n",
              strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (sequence != NULL) {
    /* We already have the sequence from the modifier check or next arg */
    int seq_len = strlen(sequence);

    /* Check if it's a special function key */
    uint8_t fn_usage = get_fn_key_usage(sequence);
    if (fn_usage != 0) {
      /* Function key */
      report[2] = fn_usage;

      /* Send key press */
      if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
        fprintf(stderr, "Error writing keyboard report: %s\n", strerror(errno));
        return EXIT_FAILURE;
      }

      /* If not holding, send key release */
      if (!hold_keys) {
        /* Clear key presses but keep explicit modifiers */
        memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
        // report[0] = modifiers; // Already set
        if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
          fprintf(stderr, "Error writing keyboard release report: %s\n",
                  strerror(errno));
          return EXIT_FAILURE;
        }
      }
    } else {
      /* Regular keys */
      for (i = 0; i < seq_len; i++) {
        char c = sequence[i];
        uint8_t usage = 0;
        uint8_t current_modifiers = modifiers; // Start with explicit modifiers

        /* Get usage code */
        if ((unsigned char)c < 128) {
          usage = (current_usage_table ? current_usage_table
                                       : usage_table_us)[(unsigned char)c];
        }

        if (usage != 0) {
          /* Add shift modifier if needed */
          if (strchr(current_shift_chars ? current_shift_chars : shift_chars_us,
                     c)) {
            current_modifiers |= MOD_SHIFT_LEFT;
          }

          /* Set modifiers and key in report */
          report[0] = current_modifiers;
          report[2] = usage;

          /* Send key press */
          if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
            fprintf(stderr, "Error writing keyboard report for '%c': %s\n", c,
                    strerror(errno));
            return EXIT_FAILURE;
          }

          /* Send key release if not holding */
          if (!hold_keys) {
            /* Clear key press */
            report[2] = 0;
            /* Restore original explicit modifiers for the release report */
            report[0] = modifiers;

            if (write(fd, report, KEYBOARD_REPORT_SIZE) !=
                KEYBOARD_REPORT_SIZE) {
              fprintf(stderr,
                      "Error writing keyboard release report for '%c': %s\n", c,
                      strerror(errno));
              return EXIT_FAILURE;
            }

            /* Small delay between keypresses */
            if (key_delay_ms > 0) {
              usleep((useconds_t)key_delay_ms * 1000);
            }
          }
        } else {
          fprintf(stderr,
                  "Warning: Character '%c' (ASCII %d) not mapped to HID usage "
                  "code.\n",
                  c, (unsigned char)c);
        }
      }
      // If holding keys, the last key remains pressed with its modifiers.
      // If not holding, ensure a final release with only explicit modifiers
      // is sent.
      if (!hold_keys && seq_len > 0) {
        memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
        report[0] = modifiers; // Restore original explicit modifiers
        if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
          fprintf(stderr, "Error writing final keyboard release report: %s\n",
                  strerror(errno));
          return EXIT_FAILURE;
        }
      }
      /* Ensure FULL release including modifiers if not holding */
      if (!hold_keys && modifiers != 0) {
        memset(report, 0, KEYBOARD_REPORT_SIZE);
        write(fd, report, KEYBOARD_REPORT_SIZE);
      }
    }
  } else if (modifiers != 0 && !release_keys) {
    // Only modifiers were given (and not --release)
    // Send a report with just modifiers pressed, no keys.
    // User must explicitly call --release later to clear modifiers.
    if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
      fprintf(stderr, "Error writing modifier-only report: %s\n",
              strerror(errno));
      return EXIT_FAILURE;
    }
  } else {
    // No sequence, no --release, no modifiers specified
    fprintf(stderr, "Error: Keyboard command requires a sequence, --release, "
                    "or modifiers.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Mouse Helper Functions */
int send_mouse_move(int8_t x, int8_t y) {
  if (!g_mouse_device)
    return -1;
  int fd = get_cached_fd(g_mouse_device, &g_fd_mouse);
  if (fd < 0)
    return -1;

  uint8_t report[8] = {0};
  report[1] = x;
  report[2] = y;

  if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
    return -1;
  }

  // Minimal zero report to ensure state is clean? Not strictly required for
  // relative moves but good for consistency.
  memset(report, 0, g_mouse_report_size);
  // write(fd, report, g_mouse_report_size);

  return 0;
}

int send_mouse_press(uint8_t button) {
  if (!g_mouse_device)
    return -1;
  int fd = get_cached_fd(g_mouse_device, &g_fd_mouse);
  if (fd < 0)
    return -1;

  uint8_t report[8] = {0};
  report[0] = button;

  if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
    return -1;
  }
  return 0;
}

int send_mouse_release() {
  if (!g_mouse_device)
    return -1;
  int fd = get_cached_fd(g_mouse_device, &g_fd_mouse);
  if (fd < 0)
    return -1;

  uint8_t report[8] = {0};
  if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
    return -1;
  }
  return 0;
}

int send_mouse_click(uint8_t button) {
  if (send_mouse_press(button) != 0)
    return -1;
  usleep(30000); // 30ms
  return send_mouse_release();
}
int process_mouse(int argc, char *argv[]) {
  int fd;
  // Mouse report: [Buttons] [X delta] [Y delta] [VScroll delta] [HScroll
  // delta (optional)] Use a buffer large enough for both 4- and 5-byte
  // reports
  uint8_t report[8] = {0};
  // Some descriptors might use 5 bytes to include HScroll separately.
  // Adapt MOUSE_REPORT_SIZE and indexing if necessary.

  // --- Check if device path is valid ---
  if (!g_mouse_device) {
    fprintf(stderr, "Error: Mouse device path not set.\n");
    return EXIT_FAILURE;
  }
  // ---

  if (argc <
      2) { // Need at least action (argv[0] is "mouse", argv[1] is action)
    fprintf(stderr,
            "Error: Mouse action required (e.g., move, click, scroll)\n");
    // print mouse usage?
    return EXIT_FAILURE;
  }

  /* Open the HID mouse device */
  fd = get_cached_fd(g_mouse_device, &g_fd_mouse);
  if (fd < 0) {
    fprintf(stderr, "Error opening HID mouse device (%s): %s\n", g_mouse_device,
            strerror(errno));
    return EXIT_FAILURE;
  }

  const char *action = argv[1]; // Action is now argv[1]

  if (strcmp(action, "move") == 0) {
    if (argc < 4) { // Need action + X + Y (argv[1], argv[2], argv[3])
      fprintf(stderr, "Error: move requires X Y parameters\n");
      return EXIT_FAILURE;
    }

    long x_val_long = strtol(argv[2], NULL, 10);
    long y_val_long = strtol(argv[3], NULL, 10);

    // Clamp values to int8_t range (-127 to 127)
    // Note: HID uses signed 8-bit relative values.
    int8_t x = (x_val_long > 127)
                   ? 127
                   : ((x_val_long < -127) ? -127 : (int8_t)x_val_long);
    int8_t y = (y_val_long > 127)
                   ? 127
                   : ((y_val_long < -127) ? -127 : (int8_t)y_val_long);

    // close(fd); // Helper opens its own, but we are in process_mouse which used to open manually.
    // Actually send_mouse_move calls get_cached_fd too.
    // If we are calling helper functions like send_mouse_move, we don't need to open FD here necessarily,
    // BUT process_mouse does write() directly in "up" and "scroll" cases.
    // Wait, "move" calls send_mouse_move. "click" calls send_mouse_click. "down" calls send_mouse_press.
    // "up" does write() manually. "scroll" does write() manually.
    // So for "move", "click", "down", we just call the helper.
    // For "up", "scroll", we use `fd` obtained above.
    // Since `get_cached_fd` returns the SAME fd, it works.

    if (send_mouse_move(x, y) != 0)
      return EXIT_FAILURE;

  } else if (strcmp(action, "click") == 0) {
    uint8_t button = MOUSE_BTN_LEFT; /* Default to left button */

    if (argc > 2) { // Check argv[2] for button type
      if (strcasecmp(argv[2], "right") == 0) {
        button = MOUSE_BTN_RIGHT;
      } else if (strcasecmp(argv[2], "middle") == 0) {
        button = MOUSE_BTN_MIDDLE;
      } else if (strcasecmp(argv[2], "left") != 0) {
        fprintf(stderr,
                "Warning: Unknown button '%s' for click, defaulting to left.\n",
                argv[2]);
      }
    }
    if (send_mouse_click(button) != 0)
      return EXIT_FAILURE;

  } else if (strcmp(action, "doubleclick") == 0) {
    uint8_t button = MOUSE_BTN_LEFT; /* Double-click is typically left button */

    // Argument check (should not have extra args)
    if (argc > 2) {
      fprintf(stderr,
              "Warning: doubleclick does not take additional arguments.\n");
    }
    if (send_mouse_click(button) != 0)
      return EXIT_FAILURE;
    usleep(100000);
    if (send_mouse_click(button) != 0)
      return EXIT_FAILURE;

  } else if (strcmp(action, "down") == 0) {
    uint8_t button = MOUSE_BTN_LEFT; /* Default to left button */

    if (argc > 2) { // Check argv[2] for button type
      if (strcasecmp(argv[2], "right") == 0) {
        button = MOUSE_BTN_RIGHT;
      } else if (strcasecmp(argv[2], "middle") == 0) {
        button = MOUSE_BTN_MIDDLE;
      } else if (strcasecmp(argv[2], "left") != 0) {
        fprintf(stderr,
                "Warning: Unknown button '%s' for down, defaulting to left.\n",
                argv[2]);
      }
    }
    if (send_mouse_press(button) != 0)
      return EXIT_FAILURE;

  } else if (strcmp(action, "up") == 0) {
    // Argument check (should not have extra args)
    if (argc > 2) {
      fprintf(stderr, "Warning: up does not take additional arguments.\n");
    }
    if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
      fprintf(stderr, "Error writing mouse button release report: %s\n",
              strerror(errno));
      return EXIT_FAILURE;
    }
  } else if (strcmp(action, "scroll") == 0) {
    if (argc < 3) { // Need action + V [H] (argv[1], argv[2], [argv[3]])
      fprintf(stderr, "Error: scroll requires V [H] parameters (Vertical, "
                      "optional Horizontal)\n");
      return EXIT_FAILURE;
    }

    long v_val_long = strtol(argv[2], NULL, 10);
    long h_val_long = (argc > 3) ? strtol(argv[3], NULL, 10) : 0; // Optional H

    // Clamp values to int8_t range (-127 to 127)
    int8_t vertical = (v_val_long > 127)
                          ? 127
                          : ((v_val_long < -127) ? -127 : (int8_t)v_val_long);
    int8_t horizontal = (h_val_long > 127)
                            ? 127
                            : ((h_val_long < -127) ? -127 : (int8_t)h_val_long);

    // Vertical scroll always at byte 3; optional horizontal at byte 4 when
    // enabled
    report[3] = vertical;
    if (g_mouse_support_hscroll && g_mouse_report_size >= 5) {
      report[4] = horizontal;
    } else if (horizontal != 0) {
      fprintf(stderr, "Warning: Horizontal scroll requested but not enabled "
                      "(set HID_MOUSE_HSCROLL=1).\n");
    }

    if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
      fprintf(stderr, "Error writing mouse scroll report: %s\n",
              strerror(errno));
      return EXIT_FAILURE;
    }
    // Send a zero report immediately after scroll to stop it
    memset(report, 0, g_mouse_report_size);
    usleep(10000); // Small delay before zero report
    if (write(fd, report, g_mouse_report_size) != g_mouse_report_size) {
      fprintf(stderr, "Warning: Error writing zero scroll report: %s\n",
              strerror(errno));
      // Non-fatal, scroll likely still occurred.
    }

  } else {
    fprintf(stderr, "Error: Unknown mouse action '%s'\n", action);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Process consumer control commands */
int process_consumer(int argc, char *argv[]) {
  int fd;
  uint8_t report[CONSUMER_REPORT_SIZE] = {0};

  // --- Check if device path is valid ---
  if (!g_consumer_device) {
    fprintf(stderr, "Error: Consumer device path not set.\n");
    return EXIT_FAILURE;
  }
  // ---

  if (argc < 2) { // Need action (argv[0] is "consumer", argv[1] is action)
    fprintf(
        stderr,
        "Error: Consumer control action required (e.g., PLAY, MUTE, VOL+)\n");
    // print consumer usage?
    return EXIT_FAILURE;
  }

  /* Open the HID consumer device */
  fd = get_cached_fd(g_consumer_device, &g_fd_consumer);
  if (fd < 0) {
    fprintf(stderr, "Error opening HID consumer device (%s): %s\n",
            g_consumer_device, strerror(errno));
    return EXIT_FAILURE;
  }

  const char *action = argv[1]; // Action is now argv[1]
  uint16_t usage = get_consumer_key_usage(action);

  if (usage == 0) {
    fprintf(stderr, "Error: Unknown consumer control action '%s'\n", action);
    return EXIT_FAILURE;
  }

  /* Set usage code (little endian) */
  report[0] = usage & 0xFF;
  report[1] = (usage >> 8) & 0xFF;

  /* Send key press */
  if (write(fd, report, CONSUMER_REPORT_SIZE) != CONSUMER_REPORT_SIZE) {
    fprintf(stderr, "Error writing consumer report: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  /* Short delay (necessary for consumer controls) */
  usleep(50000); // 50ms

  /* Send key release */
  memset(report, 0, CONSUMER_REPORT_SIZE);
  if (write(fd, report, CONSUMER_REPORT_SIZE) != CONSUMER_REPORT_SIZE) {
    fprintf(stderr, "Error writing consumer release report: %s\n",
            strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  // Allow environment overrides first
  load_env_devices();
  // Configure optional mouse capabilities from env (HID_MOUSE_HSCROLL /
  // HID_MOUSE_REPORT_SIZE)
  {
    const char *sz = getenv("HID_MOUSE_REPORT_SIZE");
    const char *hs = getenv("HID_MOUSE_HSCROLL");
    if (sz) {
      int v = atoi(sz);
      if (v == 5 || v == 4) {
        g_mouse_report_size = v;
      }
    }
    if (hs && (strcmp(hs, "1") == 0 || strcasecmp(hs, "true") == 0 ||
               strcasecmp(hs, "yes") == 0)) {
      g_mouse_support_hscroll = 1;
      if (g_mouse_report_size < 5)
        g_mouse_report_size = 5;
    }
  }
  // Attempt to discover devices; may return fewer than 3 and that's OK.
  find_hidg_devices();

  // Register cleanup function to free memory on exit
  atexit(cleanup_device_paths);
  atexit(close_hid_fds);

  if (argc < 2) {
    print_usage(argv[0]); // Will exit
  }

  const char *command = argv[1];

  // Shift arguments for sub-functions
  // The sub-function will receive its command name as argv[0]
  // and the subsequent arguments starting from argv[1]
  int result = EXIT_FAILURE; // Default result
  if (strcmp(command, "keyboard") == 0) {
    if (!g_keyboard_device)
      attempt_hid_recovery();
    if (!g_keyboard_device) {
      fprintf(stderr, "Error: No keyboard device available. Set "
                      "HID_KEYBOARD_DEV or run setup.\n");
      return EXIT_FAILURE;
    }
    result = process_keyboard(argc - 1, &argv[1]);
  } else if (strcmp(command, "mouse") == 0) {
    if (!g_mouse_device)
      attempt_hid_recovery();
    if (!g_mouse_device) {
      fprintf(stderr, "Error: No mouse device available. Set HID_MOUSE_DEV or "
                      "run setup.\n");
      return EXIT_FAILURE;
    }
    result = process_mouse(argc - 1, &argv[1]);
  } else if (strcmp(command, "consumer") == 0) {
    if (!g_consumer_device)
      attempt_hid_recovery();
    if (!g_consumer_device) {
      fprintf(stderr, "Error: No consumer device available. Set "
                      "HID_CONSUMER_DEV or run setup.\n");
      return EXIT_FAILURE;
    }
    result = process_consumer(argc - 1, &argv[1]);
  } else if (strcmp(command, "tui") == 0) {
    if (!g_keyboard_device)
      attempt_hid_recovery();
    if (!g_keyboard_device) {
      fprintf(stderr, "Error: No keyboard device available for TUI.\n");
      return EXIT_FAILURE;
    }
    result = run_tui();
  } else if (strcmp(command, "ducky") == 0) {
    if (!g_keyboard_device)
      attempt_hid_recovery();
    if (!g_keyboard_device) {
      // Ducky needs keyboard usually
      fprintf(stderr,
              "Warning: No keyboard device found. Ducky scripts might fail.\n");
    }
    ducky_load_profile();

    const char *script = "-";
    int script_idx = -1;
    for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "--os") == 0 || strcmp(argv[i], "-p") == 0) {
        if (i + 1 < argc) {
          ducky_set_var("_OS", argv[i + 1]);
          i++;
        }
      } else if (script_idx < 0) {
        script_idx = i;
      }
    }
    if (script_idx >= 2)
      script = argv[script_idx];
    result = ducky_execute_script(script);
  } else {
    fprintf(stderr, "Error: Unknown command '%s'\n", command);
    print_usage(argv[0]); // Will exit
  }

  // cleanup_device_paths(); // Called automatically by atexit
  return result;
}

/* --- DuckyScript Support Impl --- */
static uint8_t g_held_keys[6] = {0};
static uint8_t g_held_mods = 0;

void hid_sleep(int ms) { usleep(ms * 1000); }

static void send_held_state() {
  send_keyboard_report(g_held_mods, g_held_keys[0], g_held_keys[1],
                       g_held_keys[2], g_held_keys[3], g_held_keys[4],
                       g_held_keys[5]);
}

uint8_t get_key_code(const char *name) {
  // Basic lookup - minimal implementation using usage_table reverse or helper?
  // For now, rely on utility provided or brute force usage_table?
  // Actually send_key_sequence does lookup internaly.
  // We need a helper to get code from name.
  // Let's iterate usage table? No, it's ASCII to code.
  // We need string ("ENTER") to code.
  // fn_keys has this!
  for (int i = 0; fn_keys[i].name != NULL; i++) {
    if (strcasecmp(name, fn_keys[i].name) == 0)
      return fn_keys[i].usage;
  }
  // Check ASCII
  if (strlen(name) == 1) {
    unsigned char c = (unsigned char)name[0];
    if (c < 128)
      return (current_usage_table ? current_usage_table : usage_table_us)[c];
  }
  return 0;
}

int hold_key(const char *key_name) {
  if (strcasecmp(key_name, "CTRL") == 0)
    g_held_mods |= (MOD_CTRL_LEFT);
  else if (strcasecmp(key_name, "SHIFT") == 0)
    g_held_mods |= (MOD_SHIFT_LEFT);
  else if (strcasecmp(key_name, "ALT") == 0)
    g_held_mods |= (MOD_ALT_LEFT);
  else if (strcasecmp(key_name, "GUI") == 0 ||
           strcasecmp(key_name, "WINDOWS") == 0)
    g_held_mods |= (MOD_GUI_LEFT);
  else {
    uint8_t code = get_key_code(key_name);
    if (code) {
      for (int i = 0; i < 6; i++) {
        if (g_held_keys[i] == 0) {
          g_held_keys[i] = code;
          break;
        }
      }
    }
  }
  send_held_state();
  return 0;
}

int release_key(const char *key_name) {
  if (strcasecmp(key_name, "CTRL") == 0)
    g_held_mods &= ~(MOD_CTRL_LEFT);
  else if (strcasecmp(key_name, "SHIFT") == 0)
    g_held_mods &= ~(MOD_SHIFT_LEFT);
  else if (strcasecmp(key_name, "ALT") == 0)
    g_held_mods &= ~(MOD_ALT_LEFT);
  else if (strcasecmp(key_name, "GUI") == 0)
    g_held_mods &= ~(MOD_GUI_LEFT);
  else {
    uint8_t code = get_key_code(key_name);
    if (code) {
      for (int i = 0; i < 6; i++) {
        if (g_held_keys[i] == code) {
          g_held_keys[i] = 0;
        }
      }
      // Pack
      // (Optional: move zeros to end)
    }
  }
  send_held_state();
  return 0;
}

int release_all_keys(void) {
  g_held_mods = 0;
  memset(g_held_keys, 0, 6);
  send_held_state();
  return 0;
}

/* --- Mouse Support Impl --- */
int send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                      int8_t hwheel) {
  if (!g_mouse_device)
    return -1;
  int fd = get_cached_fd(g_mouse_device, &g_fd_mouse);
  if (fd < 0)
    return -1;

  char report[8];
  memset(report, 0, 8);
  report[0] = buttons;
  report[1] = x;
  report[2] = y;
  report[3] = wheel;
  if (g_mouse_report_size > 4) {
    report[4] = hwheel;
  }

  int ret = write(fd, report, g_mouse_report_size);
  return (ret == g_mouse_report_size) ? 0 : -1;
}
