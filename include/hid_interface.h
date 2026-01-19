#ifndef HID_INTERFACE_H
#define HID_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

/* HID Core Primitives */
int send_keyboard_report(uint8_t modifiers, uint8_t key1, uint8_t key2,
                         uint8_t key3, uint8_t key4, uint8_t key5,
                         uint8_t key6);
int send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                      int8_t hwheel);
int send_mouse_click(uint8_t buttons);
int send_mouse_move(int8_t x, int8_t y);
int send_mouse_scroll(int8_t wheel);
int send_consumer_key(const char *action);

int set_hid_locale(const char *name);
int send_raw_hid_report(const uint8_t *report, size_t size);

/* High Level Helpers */
int send_key_sequence(const char *modifiers_str, const char *sequence);
int hold_key(const char *key_name);
int release_key(const char *key_name);
int release_all_keys(void);

/* Utilities */
void hid_sleep(int ms);

#endif // HID_INTERFACE_H
