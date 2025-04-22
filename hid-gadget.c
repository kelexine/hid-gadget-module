/*
 * hid-gadget.c - USB HID Gadget driver for Android (Modified for Dynamic Discovery)
 *
 * This program provides a user-space interface to USB HID gadget
 * functionality, allowing the device to act as a USB keyboard, mouse,
 * or consumer control device.
 * Dynamically finds the first three /dev/hidg* devices.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/types.h>
#include <ctype.h>
#include <dirent.h> // For directory operations

/* If the kernel headers are not available, we define our own structures */
#ifndef HID_MAX_DESCRIPTOR_SIZE
#define HID_MAX_DESCRIPTOR_SIZE 4096
#endif

// --- Dynamic Device Paths (Global Variables) ---
char *g_keyboard_device = NULL;
char *g_mouse_device    = NULL;
char *g_consumer_device = NULL;
// --- End Dynamic Device Paths ---

/* Keyboard report descriptor */
#define KEYBOARD_REPORT_SIZE 8

/* Mouse report descriptor */
#define MOUSE_REPORT_SIZE 4 /* Buttons(1), X(1), Y(1), Wheel(1) */

/* Consumer control report descriptor */
#define CONSUMER_REPORT_SIZE 2

/* Keyboard modifier masks */
#define MOD_CTRL_LEFT    (1 << 0)
#define MOD_SHIFT_LEFT   (1 << 1)
#define MOD_ALT_LEFT     (1 << 2)
#define MOD_GUI_LEFT     (1 << 3)
#define MOD_CTRL_RIGHT   (1 << 4)
#define MOD_SHIFT_RIGHT  (1 << 5)
#define MOD_ALT_RIGHT    (1 << 6)
#define MOD_GUI_RIGHT    (1 << 7)

/* Mouse button masks */
#define MOUSE_BTN_LEFT   (1 << 0)
#define MOUSE_BTN_RIGHT  (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

/* Keyboard usage table - mapping ASCII characters to HID usage codes */
static const uint8_t usage_table[128] = {
    0, 0, 0, 0, 0, 0, 0, 0,                      /* 0-7 */
    42, 43, 40, 0, 0, 0, 0, 0,                      /* 8-15 (Backspace, Tab, Enter) */
    0, 0, 0, 0, 0, 0, 0, 0,                      /* 16-23 */
    0, 0, 0, 41, 0, 0, 0, 0,                      /* 24-31 (Escape) */
    44, 30, 52, 32, 33, 34, 35, 51,               /* 32-39 (Space, 1, ', 3, 4, 5, 6, ;) - Adjusted for US layout symbols */
    47, 48, 46, 45, 54, 56, 55, 36,               /* 40-47 ((,),*,+,,,-./, 7) - Adjusted */
    39, 30, 31, 32, 33, 34, 35, 36,               /* 48-55 (0-7) */
    37, 38, 51, 46, 54, 45, 55, 52,               /* 56-63 (8,9,:,=,<,>,?,') - Adjusted */
    31, 4, 5, 6, 7, 8, 9, 10,                     /* 64-71 (@(Shift+2),A-G) */
    11, 12, 13, 14, 15, 16, 17, 18,               /* 72-79 (H-O) */
    19, 20, 21, 22, 23, 24, 25, 26,               /* 80-87 (P-W) */
    27, 28, 29, 47, 49, 48, 33, 38,               /* 88-95 (X-Z,[,\|],^ (Shift+6)) - Adjusted */
    53, 4, 5, 6, 7, 8, 9, 10,                     /* 96-103 (`,a-g) */
    11, 12, 13, 14, 15, 16, 17, 18,               /* 104-111 (h-o) */
    19, 20, 21, 22, 23, 24, 25, 26,               /* 112-119 (p-w) */
    27, 28, 29, 47, 49, 48, 53, 0                 /* 120-127 (x-z,{,|,},~) - Adjusted */
};

/* Shift needed for these characters (US layout assumed) */
static const char *shift_chars = "!@#$%^&*()_+{}|:\"<>?~ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Function key mapping */
struct fn_key {
    const char *name;
    uint8_t usage;
};

static const struct fn_key fn_keys[] = {
    {"F1", 58}, {"F2", 59}, {"F3", 60}, {"F4", 61},
    {"F5", 62}, {"F6", 63}, {"F7", 64}, {"F8", 65},
    {"F9", 66}, {"F10", 67}, {"F11", 68}, {"F12", 69},
    {"INSERT", 73}, {"HOME", 74}, {"PAGEUP", 75},
    {"DELETE", 76}, {"END", 77}, {"PAGEDOWN", 78},
    {"RIGHT", 79}, {"LEFT", 80}, {"DOWN", 81}, {"UP", 82},
    {"NUMLOCK", 83}, {"ESC", 41}, {"TAB", 43},
    {"CAPSLOCK", 57}, {"PRINTSCREEN", 70}, {"SCROLLLOCK", 71},
    {"PAUSE", 72}, {"BACKSPACE", 42}, {"RETURN", 40}, {"ENTER", 40},
    {"SPACE", 44}, {NULL, 0}
};

/* Consumer control key mapping */
struct consumer_key {
    const char *name;
    uint16_t usage;
};

static const struct consumer_key consumer_keys[] = {
    {"PLAY", 0x00B0}, {"PAUSE", 0x00B1}, {"RECORD", 0x00B2},
    {"FORWARD", 0x00B3}, {"REWIND", 0x00B4}, {"NEXT", 0x00B5},
    {"PREVIOUS", 0x00B6}, {"STOP", 0x00B7}, {"EJECT", 0x00B8},
    {"MUTE", 0x00E2}, {"VOL+", 0x00E9}, {"VOL-", 0x00EA},
    {"BRIGHTNESS+", 0x006F}, {"BRIGHTNESS-", 0x0070},
    {NULL, 0}
};

// Structure to hold device info for sorting
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
            isdigit(entry->d_name[prefix_len]))
        {
            // Check if it's a character device (optional but good practice)
            char full_path[PATH_MAX];
            struct stat st;
            snprintf(full_path, sizeof(full_path), "%s/%s", dev_dir, entry->d_name);
            if (stat(full_path, &st) == 0 && S_ISCHR(st.st_mode)) {
                 // Extract the number
                if (sscanf(entry->d_name + prefix_len, "%d", &devices[count].number) == 1) {
                    strncpy(devices[count].name, entry->d_name, NAME_MAX - 1);
                    devices[count].name[NAME_MAX - 1] = '\0'; // Ensure null termination
                    count++;
                }
            }
        }
    }
    closedir(dir);

    if (count < 3) {
        fprintf(stderr, "Error: Found only %d /dev/hidg* device(s), need at least 3.\n", count);
        return count; // Return the number found
    }

    // Sort the devices by number
    qsort(devices, count, sizeof(hidg_device_info), compare_hidg_devices);

    // Construct full paths and assign to global variables
    char path_buffer[PATH_MAX];

    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir, devices[0].name);
    g_keyboard_device = strdup(path_buffer);

    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir, devices[1].name);
    g_mouse_device = strdup(path_buffer);

    snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dev_dir, devices[2].name);
    g_consumer_device = strdup(path_buffer);

    // Check if strdup succeeded
    if (!g_keyboard_device || !g_mouse_device || !g_consumer_device) {
        perror("Error allocating memory for device paths");
        // Free any allocated memory before returning
        free(g_keyboard_device); g_keyboard_device = NULL;
        free(g_mouse_device);    g_mouse_device = NULL;
        free(g_consumer_device); g_consumer_device = NULL;
        return -1;
    }

    return 3; // Success, found and assigned 3 devices
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


void print_usage(const char *prog_name) {
    // Ensure devices were found before printing usage
    if (!g_keyboard_device || !g_mouse_device || !g_consumer_device) {
         fprintf(stderr, "Error: HID Gadget devices not initialized. Cannot print usage.\n");
         // Attempt to find them again, or provide a generic message
         if (find_hidg_devices() < 3) {
              fprintf(stderr, "Failed to find required /dev/hidg* devices.\n");
              fprintf(stderr, "Usage: %s [keyboard|mouse|consumer] [options]\n", prog_name);
              exit(EXIT_FAILURE);
         }
    }

    fprintf(stderr, "Usage: %s [keyboard|mouse|consumer] [options]\n", prog_name);
    fprintf(stderr, "\nKeyboard mode (uses %s):\n", g_keyboard_device);
    fprintf(stderr, "  %s keyboard [--hold] [--release] [modifiers] sequence\n", prog_name);
    fprintf(stderr, "    modifiers: CTRL, ALT, SHIFT, GUI (can be combined with -)\n");
    fprintf(stderr, "               Prefix with R for right-side keys (e.g., RCTRL)\n");
    fprintf(stderr, "    special keys: F1-F12, ESC, TAB, etc. (case-insensitive)\n");
    fprintf(stderr, "    sequence: String of characters or a single special key name.\n");

    fprintf(stderr, "\nMouse mode (uses %s):\n", g_mouse_device);
    fprintf(stderr, "  %s mouse [action] [parameters]\n", prog_name);
    fprintf(stderr, "    move X Y       - Move mouse by X,Y relative units (-127 to 127)\n");
    fprintf(stderr, "    click [button] - Click button (left(default),right,middle)\n");
    fprintf(stderr, "    doubleclick    - Double click left button\n");
    fprintf(stderr, "    down [button]  - Press and hold button (left(default),right,middle)\n");
    fprintf(stderr, "    up             - Release all held buttons\n");
    fprintf(stderr, "    scroll V [H]   - Scroll V(vertical) H(horizontal, optional) units (-127 to 127)\n");

    fprintf(stderr, "\nConsumer mode (uses %s):\n", g_consumer_device);
    fprintf(stderr, "  %s consumer [action]\n", prog_name);
    fprintf(stderr, "    actions: PLAY, PAUSE, MUTE, VOL+, VOL-, etc. (case-insensitive)\n");

    exit(EXIT_FAILURE);
}

/* Function to parse modifier keys */
uint8_t parse_modifiers(const char *mod_str) {
    uint8_t modifiers = 0;
    char *mod_copy = strdup(mod_str);
    if (!mod_copy) {
        perror("Error duplicating modifier string");
        return 0; // Return 0 modifiers on allocation failure
    }
    char *token = strtok(mod_copy, "-");

    while (token != NULL) {
        if (strcasecmp(token, "CTRL") == 0 || strcasecmp(token, "CONTROL") == 0)
            modifiers |= MOD_CTRL_LEFT;
        else if (strcasecmp(token, "SHIFT") == 0)
            modifiers |= MOD_SHIFT_LEFT;
        else if (strcasecmp(token, "ALT") == 0)
            modifiers |= MOD_ALT_LEFT;
        else if (strcasecmp(token, "GUI") == 0 || strcasecmp(token, "WIN") == 0 || strcasecmp(token, "META") == 0)
            modifiers |= MOD_GUI_LEFT;
        else if (strcasecmp(token, "RCTRL") == 0 || strcasecmp(token, "RCONTROL") == 0)
            modifiers |= MOD_CTRL_RIGHT;
        else if (strcasecmp(token, "RSHIFT") == 0)
            modifiers |= MOD_SHIFT_RIGHT;
        else if (strcasecmp(token, "RALT") == 0)
            modifiers |= MOD_ALT_RIGHT;
        else if (strcasecmp(token, "RGUI") == 0 || strcasecmp(token, "RWIN") == 0 || strcasecmp(token, "RMETA") == 0)
            modifiers |= MOD_GUI_RIGHT;

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

/* Get consumer key usage code by name */
uint16_t get_consumer_key_usage(const char *key_name) {
    int i;
    for (i = 0; consumer_keys[i].name != NULL; i++) {
        if (strcasecmp(key_name, consumer_keys[i].name) == 0) {
            return consumer_keys[i].usage;
        }
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

    // --- Check if device path is valid ---
    if (!g_keyboard_device) {
        fprintf(stderr, "Error: Keyboard device path not set.\n");
        return EXIT_FAILURE;
    }
    // ---

    static struct option long_options[] = {
        {"hold", no_argument, 0, 'h'},
        {"release", no_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    // Reset getopt for parsing within a function
    optind = 1; // Important: Reset optind for getopt_long when called multiple times

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
                 // print_usage(argv[0]); // Might need adjustment if argv[0] isn't program name
                 return EXIT_FAILURE;
        }
    }

    /* Find where the sequence/modifiers start AFTER getopt processing */
    seq_start = optind; // optind is index of the first non-option argument

    /* Check if we have modifiers */
    if (seq_start < argc && strpbrk(argv[seq_start], "-")) { // Check if it looks like modifiers
         // Attempt to parse as modifiers. If it fails, it might be a sequence starting with '-'
         uint8_t temp_mods = parse_modifiers(argv[seq_start]);
         // Heuristic: if parse_modifiers returns non-zero OR the string ONLY contains modifier names and '-', assume it's modifiers.
         // This is imperfect but covers common cases. A more robust approach might be needed.
         // A simple check: does it contain any characters NOT in "CTRLALTSHIFTGUI-" (case-insensitive)?
         int only_mods = 1;
         for(int k=0; argv[seq_start][k]; k++){
             if (!strchr("CTRLALTSHIFTGUI-ctrlaltshiftgui", argv[seq_start][k])){
                 only_mods = 0;
                 break;
             }
         }

         if(temp_mods != 0 || only_mods) {
            modifiers = temp_mods;
            seq_start++;
         }
         // Otherwise, assume it's part of the sequence (e.g., typing "-")
    }


    /* Open the HID keyboard device */
    fd = open(g_keyboard_device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID keyboard device (%s): %s\n", g_keyboard_device, strerror(errno));
        return EXIT_FAILURE;
    }

    /* Set explicit modifiers in the report */
    report[0] = modifiers;

    /* Process key sequence or release */
    if (release_keys) {
         /* Release all keys and modifiers */
         memset(report, 0, KEYBOARD_REPORT_SIZE);
         if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
             fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
             close(fd);
             return EXIT_FAILURE;
         }
    } else if (seq_start < argc) {
        const char *sequence = argv[seq_start];
        int seq_len = strlen(sequence);

        /* Check if it's a special function key */
        uint8_t fn_usage = get_fn_key_usage(sequence);
        if (fn_usage != 0) {
            /* Function key */
            report[2] = fn_usage;

            /* Send key press */
            if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                fprintf(stderr, "Error writing keyboard report: %s\n", strerror(errno));
                close(fd);
                return EXIT_FAILURE;
            }

            /* If not holding, send key release */
            if (!hold_keys) {
                /* Clear key presses but keep explicit modifiers */
                memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
                // report[0] = modifiers; // Already set
                if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                    fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
                    close(fd);
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
                    usage = usage_table[(unsigned char)c];
                }

                if (usage != 0) {
                    /* Add shift modifier if needed */
                    if (strchr(shift_chars, c)) {
                        current_modifiers |= MOD_SHIFT_LEFT;
                    }

                    /* Set modifiers and key in report */
                    report[0] = current_modifiers;
                    report[2] = usage;

                    /* Send key press */
                    if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                        fprintf(stderr, "Error writing keyboard report for '%c': %s\n", c, strerror(errno));
                        close(fd);
                        return EXIT_FAILURE;
                    }

                    /* Send key release if not holding */
                    if (!hold_keys) {
                        /* Clear key press */
                        report[2] = 0;
                        /* Restore original explicit modifiers for the release report */
                        report[0] = modifiers;

                        if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                            fprintf(stderr, "Error writing keyboard release report for '%c': %s\n", c, strerror(errno));
                            close(fd);
                            return EXIT_FAILURE;
                        }

                        /* Small delay between keypresses */
                        usleep(10000); // 10ms
                    }
                } else {
                     fprintf(stderr, "Warning: Character '%c' (ASCII %d) not mapped to HID usage code.\n", c, (unsigned char)c);
                }
            }
             // If holding keys, the last key remains pressed with its modifiers.
             // If not holding, ensure a final release with only explicit modifiers is sent.
             if (!hold_keys && seq_len > 0) {
                  memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
                  report[0] = modifiers; // Restore original explicit modifiers
                  if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                       fprintf(stderr, "Error writing final keyboard release report: %s\n", strerror(errno));
                       close(fd);
                       return EXIT_FAILURE;
                  }
             }
        }
    } else if (modifiers != 0 && !release_keys) {
         // Only modifiers were given (and not --release)
         // Send a report with just modifiers pressed, no keys.
         // User must explicitly call --release later to clear modifiers.
         if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
             fprintf(stderr, "Error writing modifier-only report: %s\n", strerror(errno));
             close(fd);
             return EXIT_FAILURE;
         }
    }
    else {
        // No sequence, no --release, no modifiers specified
        fprintf(stderr, "Error: Keyboard command requires a sequence, --release, or modifiers.\n");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}


/* Process mouse commands */
int process_mouse(int argc, char *argv[]) {
    int fd;
    // Mouse report: [Buttons] [X delta] [Y delta] [VScroll delta] [HScroll delta (optional, depends on descriptor)]
    // Assuming standard 4-byte report: Buttons, X, Y, VScroll
    uint8_t report[MOUSE_REPORT_SIZE] = {0};
    // Some descriptors might use 5 bytes to include HScroll separately.
    // Adapt MOUSE_REPORT_SIZE and indexing if necessary.

    // --- Check if device path is valid ---
    if (!g_mouse_device) {
        fprintf(stderr, "Error: Mouse device path not set.\n");
        return EXIT_FAILURE;
    }
    // ---

    if (argc < 2) { // Need at least action (argv[0] is "mouse", argv[1] is action)
        fprintf(stderr, "Error: Mouse action required (e.g., move, click, scroll)\n");
        // print mouse usage?
        return EXIT_FAILURE;
    }

    /* Open the HID mouse device */
    fd = open(g_mouse_device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID mouse device (%s): %s\n", g_mouse_device, strerror(errno));
        return EXIT_FAILURE;
    }

    const char *action = argv[1]; // Action is now argv[1]

    if (strcmp(action, "move") == 0) {
        if (argc < 4) { // Need action + X + Y (argv[1], argv[2], argv[3])
            fprintf(stderr, "Error: move requires X Y parameters\n");
            close(fd);
            return EXIT_FAILURE;
        }

        long x_val_long = strtol(argv[2], NULL, 10);
        long y_val_long = strtol(argv[3], NULL, 10);

        // Clamp values to int8_t range (-127 to 127)
        // Note: HID uses signed 8-bit relative values.
        int8_t x = (x_val_long > 127) ? 127 : ((x_val_long < -127) ? -127 : (int8_t)x_val_long);
        int8_t y = (y_val_long > 127) ? 127 : ((y_val_long < -127) ? -127 : (int8_t)y_val_long);

        report[1] = x; // X movement
        report[2] = y; // Y movement

        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse move report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
        // Send a zero report immediately after move to stop it (good practice)
        memset(report, 0, MOUSE_REPORT_SIZE);
        usleep(1000); // Minimal delay needed? Maybe not for move.
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
             fprintf(stderr, "Warning: Error writing zero move report: %s\n", strerror(errno));
             // Non-fatal, movement likely still occurred.
        }

    } else if (strcmp(action, "click") == 0) {
        uint8_t button = MOUSE_BTN_LEFT;  /* Default to left button */

        if (argc > 2) { // Check argv[2] for button type
            if (strcasecmp(argv[2], "right") == 0) {
                button = MOUSE_BTN_RIGHT;
            } else if (strcasecmp(argv[2], "middle") == 0) {
                button = MOUSE_BTN_MIDDLE;
            } else if (strcasecmp(argv[2], "left") != 0) {
                 fprintf(stderr, "Warning: Unknown button '%s' for click, defaulting to left.\n", argv[2]);
            }
        }

        /* Press button */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button press report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        /* Short delay */
        usleep(30000); // 30ms

        /* Release button */
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button release report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "doubleclick") == 0) {
        uint8_t button = MOUSE_BTN_LEFT;  /* Double-click is typically left button */

        // Argument check (should not have extra args)
        if (argc > 2) {
             fprintf(stderr, "Warning: doubleclick does not take additional arguments.\n");
        }

        /* First click */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) { /* Press */
            fprintf(stderr, "Error writing mouse button press report (1st click): %s\n", strerror(errno)); close(fd); return EXIT_FAILURE;
        }
        usleep(30000);
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) { /* Release */
            fprintf(stderr, "Error writing mouse button release report (1st click): %s\n", strerror(errno)); close(fd); return EXIT_FAILURE;
        }

        /* Delay between clicks (adjust as needed for OS sensitivity) */
        usleep(100000); // 100ms

        /* Second click */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) { /* Press */
            fprintf(stderr, "Error writing mouse button press report (2nd click): %s\n", strerror(errno)); close(fd); return EXIT_FAILURE;
        }
        usleep(30000);
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) { /* Release */
            fprintf(stderr, "Error writing mouse button release report (2nd click): %s\n", strerror(errno)); close(fd); return EXIT_FAILURE;
        }
    } else if (strcmp(action, "down") == 0) {
        uint8_t button = MOUSE_BTN_LEFT;  /* Default to left button */

        if (argc > 2) { // Check argv[2] for button type
            if (strcasecmp(argv[2], "right") == 0) {
                button = MOUSE_BTN_RIGHT;
            } else if (strcasecmp(argv[2], "middle") == 0) {
                button = MOUSE_BTN_MIDDLE;
            } else if (strcasecmp(argv[2], "left") != 0) {
                 fprintf(stderr, "Warning: Unknown button '%s' for down, defaulting to left.\n", argv[2]);
            }
        }

        /* Press button without releasing */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button press report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "up") == 0) {
         // Argument check (should not have extra args)
        if (argc > 2) {
             fprintf(stderr, "Warning: up does not take additional arguments.\n");
        }
        /* Release all buttons */
        report[0] = 0;
        // Also clear movement/scroll state just in case
        report[1] = 0;
        report[2] = 0;
        report[3] = 0;
        // report[4] = 0; // If using 5-byte report

        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button release report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "scroll") == 0) {
        if (argc < 3) { // Need action + V [H] (argv[1], argv[2], [argv[3]])
            fprintf(stderr, "Error: scroll requires V [H] parameters (Vertical, optional Horizontal)\n");
            close(fd);
            return EXIT_FAILURE;
        }

        long v_val_long = strtol(argv[2], NULL, 10);
        long h_val_long = (argc > 3) ? strtol(argv[3], NULL, 10) : 0; // Optional H

        // Clamp values to int8_t range (-127 to 127)
        int8_t vertical = (v_val_long > 127) ? 127 : ((v_val_long < -127) ? -127 : (int8_t)v_val_long);
        int8_t horizontal = (h_val_long > 127) ? 127 : ((h_val_long < -127) ? -127 : (int8_t)h_val_long);

        // Standard HID mouse report places vertical scroll in byte 3.
        // Horizontal scroll is less standard; sometimes byte 4, sometimes needs a different report ID.
        // Assuming byte 3 for vertical scroll. We won't send horizontal scroll with this basic report.
        // If your HID descriptor supports horizontal scroll in byte 4, adjust MOUSE_REPORT_SIZE and use report[4].
        report[3] = vertical;   /* Vertical scroll wheel */

        // If horizontal scroll is needed, you might need a different HID descriptor
        // or check if your current descriptor uses byte 4 (or another byte).
        // report[4] = horizontal; /* Hypothetical horizontal scroll */
        if (horizontal != 0) {
             fprintf(stderr, "Warning: Horizontal scroll not implemented in this basic report structure.\n");
        }


        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse scroll report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
        // Send a zero report immediately after scroll to stop it
        memset(report, 0, MOUSE_REPORT_SIZE);
        usleep(10000); // Small delay before zero report
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
             fprintf(stderr, "Warning: Error writing zero scroll report: %s\n", strerror(errno));
             // Non-fatal, scroll likely still occurred.
        }

    } else {
        fprintf(stderr, "Error: Unknown mouse action '%s'\n", action);
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
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
        fprintf(stderr, "Error: Consumer control action required (e.g., PLAY, MUTE, VOL+)\n");
        // print consumer usage?
        return EXIT_FAILURE;
    }

    /* Open the HID consumer device */
    fd = open(g_consumer_device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID consumer device (%s): %s\n", g_consumer_device, strerror(errno));
        return EXIT_FAILURE;
    }

    const char *action = argv[1]; // Action is now argv[1]
    uint16_t usage = get_consumer_key_usage(action);

    if (usage == 0) {
        fprintf(stderr, "Error: Unknown consumer control action '%s'\n", action);
        close(fd);
        return EXIT_FAILURE;
    }

    /* Set usage code (little endian) */
    report[0] = usage & 0xFF;
    report[1] = (usage >> 8) & 0xFF;

    /* Send key press */
    if (write(fd, report, CONSUMER_REPORT_SIZE) != CONSUMER_REPORT_SIZE) {
        fprintf(stderr, "Error writing consumer report: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    /* Short delay (necessary for consumer controls) */
    usleep(50000); // 50ms

    /* Send key release */
    memset(report, 0, CONSUMER_REPORT_SIZE);
    if (write(fd, report, CONSUMER_REPORT_SIZE) != CONSUMER_REPORT_SIZE) {
        fprintf(stderr, "Error writing consumer release report: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    // Find devices *before* parsing arguments, as print_usage might be called early
    int devices_found = find_hidg_devices();
    if (devices_found < 3) {
        // Error message already printed by find_hidg_devices or print_usage
        cleanup_device_paths(); // Clean up any partially allocated paths
        return EXIT_FAILURE;
    }

    // Register cleanup function to free memory on exit
    atexit(cleanup_device_paths);

    if (argc < 2) {
        print_usage(argv[0]); // Will exit
    }

    const char *command = argv[1];

    // Shift arguments for sub-functions
    // The sub-function will receive its command name as argv[0]
    // and the subsequent arguments starting from argv[1]
    int result = EXIT_FAILURE; // Default result
    if (strcmp(command, "keyboard") == 0) {
        result = process_keyboard(argc - 1, &argv[1]);
    } else if (strcmp(command, "mouse") == 0) {
        result = process_mouse(argc - 1, &argv[1]);
    } else if (strcmp(command, "consumer") == 0) {
        result = process_consumer(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]); // Will exit
    }

    // cleanup_device_paths(); // Called automatically by atexit
    return result;
}
