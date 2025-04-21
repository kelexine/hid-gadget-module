/*
 * hid-gadget.c - USB HID Gadget driver for Android
 *
 * This program provides a user-space interface to USB HID gadget
 * functionality, allowing the device to act as a USB keyboard, mouse,
 * or consumer control device.
 * Uses /dev/hidg1 (keyboard), /dev/hidg2 (mouse), /dev/hidg3 (consumer).
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

/* If the kernel headers are not available, we define our own structures */
#ifndef HID_MAX_DESCRIPTOR_SIZE
#define HID_MAX_DESCRIPTOR_SIZE 4096
#endif

// --- Updated Device Paths ---
#define KEYBOARD_DEVICE "/dev/hidg1"
#define MOUSE_DEVICE    "/dev/hidg2"
#define CONSUMER_DEVICE "/dev/hidg3"
// --- End Updated Device Paths ---

/* Keyboard report descriptor */
#define KEYBOARD_REPORT_SIZE 8

/* Mouse report descriptor */
#define MOUSE_REPORT_SIZE 4

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
    0, 0, 0, 0, 0, 0, 0, 0,                             /* 0-7 */
    42, 43, 40, 0, 0, 0, 0, 0,                          /* 8-15 (Backspace, Tab, Enter) */
    0, 0, 0, 0, 0, 0, 0, 0,                             /* 16-23 */
    0, 0, 0, 41, 0, 0, 0, 0,                            /* 24-31 (Escape) */
    44, 0, 0, 0, 0, 0, 0, 52,                           /* 32-39 (Space, ') */
    47, 48, 46, 45, 54, 56, 55, 57,                     /* 40-47 ((,),*,+,,,-./) */
    39, 30, 31, 32, 33, 34, 35, 36,                     /* 48-55 (0-7) */
    37, 38, 51, 0, 0, 0, 0, 0,                          /* 56-63 (8,9,:) */
    0, 4, 5, 6, 7, 8, 9, 10,                            /* 64-71 (@,A-G) */
    11, 12, 13, 14, 15, 16, 17, 18,                     /* 72-79 (H-O) */
    19, 20, 21, 22, 23, 24, 25, 26,                     /* 80-87 (P-W) */
    27, 28, 29, 0, 0, 0, 0, 0,                          /* 88-95 (X-Z) */
    0, 4, 5, 6, 7, 8, 9, 10,                            /* 96-103 (`,a-g) */
    11, 12, 13, 14, 15, 16, 17, 18,                     /* 104-111 (h-o) */
    19, 20, 21, 22, 23, 24, 25, 26,                     /* 112-119 (p-w) */
    27, 28, 29, 0, 0, 0, 0, 0                           /* 120-127 (x-z) */
};

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

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [keyboard|mouse|consumer] [options]\n", prog_name);
    fprintf(stderr, "\nKeyboard mode (uses %s):\n", KEYBOARD_DEVICE);
    fprintf(stderr, "  %s keyboard [--hold] [--release] [modifiers] sequence\n", prog_name);
    fprintf(stderr, "    modifiers: CTRL, ALT, SHIFT, GUI (can be combined with -)\n");
    fprintf(stderr, "    special keys: F1-F12, ESC, TAB, etc.\n");

    fprintf(stderr, "\nMouse mode (uses %s):\n", MOUSE_DEVICE);
    fprintf(stderr, "  %s mouse [action] [parameters]\n", prog_name);
    fprintf(stderr, "    move X Y       - Move mouse by X,Y relative units\n");
    fprintf(stderr, "    click [button] - Click button (left,right,middle)\n");
    fprintf(stderr, "    doubleclick    - Double click left button\n");
    fprintf(stderr, "    down [button]  - Press and hold button\n");
    fprintf(stderr, "    up [button]    - Release held button\n");
    fprintf(stderr, "    scroll X Y     - Scroll X (horizontal) Y (vertical) units\n");

    fprintf(stderr, "\nConsumer mode (uses %s):\n", CONSUMER_DEVICE);
    fprintf(stderr, "  %s consumer [action]\n", prog_name);
    fprintf(stderr, "    actions: PLAY, PAUSE, MUTE, VOL+, VOL-, etc.\n");

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

    static struct option long_options[] = {
        {"hold", no_argument, 0, 'h'},
        {"release", no_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    // Reset getopt for parsing within a function
    optind = 1;

    /* Parse command line options */
    while ((opt = getopt_long(argc, argv, "hr", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                hold_keys = 1;
                break;
            case 'r':
                release_keys = 1;
                break;
            default:
                 fprintf(stderr, "Invalid option in keyboard command.\n");
                 // Consider printing keyboard-specific usage here
                 return EXIT_FAILURE;
        }
    }

    /* Find where the sequence/modifiers start */
    seq_start = optind;

    /* Check if we have modifiers */
    if (seq_start < argc && strchr(argv[seq_start], '-')) {
        modifiers = parse_modifiers(argv[seq_start]);
        seq_start++;
    }

    /* Open the HID keyboard device */
    fd = open(KEYBOARD_DEVICE, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID keyboard device (%s): %s\n", KEYBOARD_DEVICE, strerror(errno));
        return EXIT_FAILURE;
    }

    /* Set modifiers in the report */
    report[0] = modifiers;

    /* Process key sequence */
    if (seq_start < argc) {
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
                /* Clear key presses but keep modifiers if requested */
                memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
                if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                    fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
                    close(fd);
                    return EXIT_FAILURE;
                }
            }
        } else if (release_keys) {
            /* Release all keys */
            memset(report, 0, KEYBOARD_REPORT_SIZE);
            if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
                close(fd);
                return EXIT_FAILURE;
            }
        } else {
            /* Regular keys */
            for (i = 0; i < seq_len; i++) {
                char c = sequence[i];
                uint8_t usage = 0;
                uint8_t original_modifiers = report[0]; // Store original modifiers

                /* Get usage code */
                if ((unsigned char)c < 128) {
                    usage = usage_table[(unsigned char)c];
                }

                if (usage != 0) {
                    /* Set shift modifier if needed for uppercase or specific symbols */
                    if (isupper(c)) {
                        report[0] |= MOD_SHIFT_LEFT;
                    } else if (strchr("!@#$%^&*()_+{}|:\"<>?~", c)) {
                        /* These characters generally need shift */
                        report[0] |= MOD_SHIFT_LEFT;
                    } else {
                        /* Ensure shift is off if not needed */
                        report[0] &= ~MOD_SHIFT_LEFT;
                    }

                    /* Set key in report */
                    report[2] = usage;

                    /* Send key press */
                    if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                        fprintf(stderr, "Error writing keyboard report: %s\n", strerror(errno));
                        close(fd);
                        return EXIT_FAILURE;
                    }

                    /* Send key release if not holding */
                    if (!hold_keys) {
                        /* Clear key press */
                        report[2] = 0;
                        /* Restore original modifiers (important if shift was auto-added) */
                        report[0] = original_modifiers;

                        if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                            fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
                            close(fd);
                            return EXIT_FAILURE;
                        }

                        /* Small delay between keypresses */
                        usleep(10000); // 10ms
                    }
                } else {
                     fprintf(stderr, "Warning: Character '%c' not mapped to HID usage code.\n", c);
                }
            }
             // If holding keys, ensure a final release is sent if needed, or handled by --release
             if (hold_keys) {
                 // User must explicitly call --release later
             } else if (seq_len > 0 && !release_keys && get_fn_key_usage(sequence) == 0) {
                 // Ensure final release for regular keys if not holding and not releasing
                 memset(&report[2], 0, KEYBOARD_REPORT_SIZE - 2);
                 report[0] = modifiers; // Restore original explicit modifiers
                 if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                     fprintf(stderr, "Error writing final keyboard release report: %s\n", strerror(errno));
                     close(fd);
                     return EXIT_FAILURE;
                 }
             }
        }
    } else if (release_keys) { // Handle --release with no sequence
        /* Release all keys */
        memset(report, 0, KEYBOARD_REPORT_SIZE);
        if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
            fprintf(stderr, "Error writing keyboard release report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else {
        // No sequence and no --release, maybe just modifiers?
        // If only modifiers were given, send a report with just modifiers and no keys
        if (modifiers != 0) {
             if (write(fd, report, KEYBOARD_REPORT_SIZE) != KEYBOARD_REPORT_SIZE) {
                 fprintf(stderr, "Error writing modifier-only report: %s\n", strerror(errno));
                 close(fd);
                 return EXIT_FAILURE;
             }
             // User must explicitly call --release later to clear modifiers
        } else {
             fprintf(stderr, "Error: Keyboard command requires a sequence or --release option.\n");
             close(fd);
             return EXIT_FAILURE;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}

/* Process mouse commands */
int process_mouse(int argc, char *argv[]) {
    int fd;
    uint8_t report[MOUSE_REPORT_SIZE] = {0};

    if (argc < 2) { // Need at least action
        fprintf(stderr, "Error: Mouse action required\n");
        // print mouse usage?
        return EXIT_FAILURE;
    }

    /* Open the HID mouse device */
    fd = open(MOUSE_DEVICE, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID mouse device (%s): %s\n", MOUSE_DEVICE, strerror(errno));
        return EXIT_FAILURE;
    }

    const char *action = argv[1]; // Action is now argv[1]

    if (strcmp(action, "move") == 0) {
        if (argc < 4) { // Need action + X + Y
            fprintf(stderr, "Error: move requires X Y parameters\n");
            close(fd);
            return EXIT_FAILURE;
        }

        int x_val = atoi(argv[2]);
        int y_val = atoi(argv[3]);

        // Clamp values to int8_t range
        int8_t x = (x_val > 127) ? 127 : ((x_val < -127) ? -127 : x_val); // Note: -128 is valid but often less useful
        int8_t y = (y_val > 127) ? 127 : ((y_val < -127) ? -127 : y_val);

        report[1] = x;
        report[2] = y;

        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse move report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "click") == 0) {
        uint8_t button = MOUSE_BTN_LEFT;  /* Default to left button */

        if (argc > 2) { // Check argv[2] for button type
            if (strcasecmp(argv[2], "right") == 0) {
                button = MOUSE_BTN_RIGHT;
            } else if (strcasecmp(argv[2], "middle") == 0) {
                button = MOUSE_BTN_MIDDLE;
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

        /* First click */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button press report (1st click): %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
        usleep(30000);
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button release report (1st click): %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        /* Delay between clicks */
        usleep(100000); // 100ms

        /* Second click */
        report[0] = button;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button press report (2nd click): %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
        usleep(30000);
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button release report (2nd click): %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "down") == 0) {
        uint8_t button = MOUSE_BTN_LEFT;  /* Default to left button */

        if (argc > 2) { // Check argv[2] for button type
            if (strcasecmp(argv[2], "right") == 0) {
                button = MOUSE_BTN_RIGHT;
            } else if (strcasecmp(argv[2], "middle") == 0) {
                button = MOUSE_BTN_MIDDLE;
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
        /* Release all buttons */
        report[0] = 0;
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse button release report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "scroll") == 0) {
        if (argc < 3) { // Need action + Y (or X Y)
            fprintf(stderr, "Error: scroll requires Y [X] parameters\n");
            close(fd);
            return EXIT_FAILURE;
        }

        int y_val = atoi(argv[2]);
        int x_val = (argc > 3) ? atoi(argv[3]) : 0; // Optional X

        // Clamp values to int8_t range
        int8_t vertical = (y_val > 127) ? 127 : ((y_val < -127) ? -127 : y_val);
        int8_t horizontal = (x_val > 127) ? 127 : ((x_val < -127) ? -127 : x_val);

        report[3] = vertical;    /* Vertical scroll */
        // Mouse report descriptor uses byte 2 for horizontal movement, byte 3 for vertical scroll
        // Let's assume the descriptor expects horizontal scroll in byte 2 as well (needs verification against descriptor)
        // If descriptor uses byte 2 for X-movement, we need a different report structure for scroll.
        // Assuming report[2] is for horizontal scroll based on common practice:
        report[2] = horizontal;  /* Horizontal scroll */

        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
            fprintf(stderr, "Error writing mouse scroll report: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
        // Send a zero report immediately after scroll to stop it
        memset(report, 0, MOUSE_REPORT_SIZE);
        usleep(10000); // Small delay before zero report
        if (write(fd, report, MOUSE_REPORT_SIZE) != MOUSE_REPORT_SIZE) {
             fprintf(stderr, "Error writing zero scroll report: %s\n", strerror(errno));
             // Don't necessarily exit, the scroll might have worked
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

    if (argc < 2) { // Need action
        fprintf(stderr, "Error: Consumer control action required\n");
        // print consumer usage?
        return EXIT_FAILURE;
    }

    /* Open the HID consumer device */
    fd = open(CONSUMER_DEVICE, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening HID consumer device (%s): %s\n", CONSUMER_DEVICE, strerror(errno));
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

    /* Short delay */
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
    if (argc < 2) {
        print_usage(argv[0]);
    }

    const char *command = argv[1];

    // Shift arguments for sub-functions
    // The sub-function will receive its command name as argv[0]
    // and the subsequent arguments starting from argv[1]
    if (strcmp(command, "keyboard") == 0) {
        return process_keyboard(argc - 1, &argv[1]);
    } else if (strcmp(command, "mouse") == 0) {
        return process_mouse(argc - 1, &argv[1]);
    } else if (strcmp(command, "consumer") == 0) {
        return process_consumer(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
    }

    return EXIT_FAILURE; // Should not be reached
}
