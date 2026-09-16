// utils/hid.js - exec wrapper and shell-safe command builders for hid-gadget.
// Author: kelexine <https://github.com/kelexine>
import { exec } from '@mysu-org/mysu';

const BIN = '/system/bin/hid-gadget';
const SETUP = '/system/bin/hid-setup';

// Base64 round-trip keeps arbitrary user text (quotes, newlines, unicode)
// out of the shell grammar entirely: the base64 alphabet is shell-safe,
// so nothing the user types can break out of the command string.
function toB64(str) {
    return btoa(unescape(encodeURIComponent(str)));
}

/** Run a hid-gadget subcommand. Returns {errno, stdout, stderr}. */
async function runHid(args) {
    return exec(`${BIN} ${args}`);
}

/** Type literal text via the keyboard function. */
export function sendText(text) {
    const b64 = toB64(text);
    return runHid(`keyboard "$(echo '${b64}' | base64 -d)"`);
}

/** Send a modifier+key combo, e.g. "CTRL-ALT-DEL". Only safe chars allowed. */
export function sendCombo(combo) {
    if (!/^[A-Z0-9-]+$/.test(combo)) {
        throw new Error(`Invalid key combo: ${combo}`);
    }
    return runHid(`keyboard ${combo}`);
}

/** Relative mouse move, clamped to the -127..127 protocol range. */
export function mouseMove(dx, dy) {
    const clamp = (n) => Math.max(-127, Math.min(127, Math.round(n)));
    return runHid(`mouse move ${clamp(dx)} ${clamp(dy)}`);
}

const CLICK_BUTTONS = new Set(['left', 'right', 'middle']);
export function mouseClick(button) {
    if (!CLICK_BUTTONS.has(button)) {
        throw new Error(`Invalid mouse button: ${button}`);
    }
    return runHid(`mouse click ${button}`);
}

export function mouseScroll(vertical) {
    const v = Math.max(-127, Math.min(127, Math.round(vertical)));
    return runHid(`mouse scroll ${v}`);
}

const CONSUMER_ACTIONS = new Set([
    'PLAY', 'PAUSE', 'MUTE', 'VOL+', 'VOL-', 'BRIGHTNESS+', 'BRIGHTNESS-',
]);
export function consumerAction(action) {
    if (!CONSUMER_ACTIONS.has(action)) {
        throw new Error(`Invalid consumer action: ${action}`);
    }
    return runHid(`consumer ${action}`);
}

const DUCKY_OS = new Set(['WINDOWS', 'MACOS', 'LINUX', 'ANDROID']);
/** Write a DuckyScript to a scoped temp file and execute it. */
export async function runDucky(script, os) {
    if (!DUCKY_OS.has(os)) {
        throw new Error(`Invalid target OS: ${os}`);
    }
    const b64 = toB64(script);
    // Setup steps are chained with && (abort on failure); cleanup runs
    // unconditionally via `;` so a failing script never leaks the temp file.
    const setup = [
        'T=$(mktemp /data/local/tmp/hidducky.XXXXXX)',
        `echo '${b64}' | base64 -d > "$T"`,
        `${BIN} ducky "$T" --os ${os}`,
    ].join(' && ');
    return exec(`${setup}; rm -f "$T"`);
}

/** Re-run the HID gadget setup script (recreates gadget functions/nodes). */
export function reRunSetup() {
    return exec(SETUP);
}

/** Read the installed module version, checking the MySU path first. */
export async function getVersion() {
    const paths = [
        '/data/adb/mysu/modules/hid-gadget/module.prop',
        '/data/adb/modules/hid-gadget/module.prop',
    ];
    const cmd = paths.map((p) => `grep '^version=' '${p}' 2>/dev/null`).join(' || ');
    const { errno, stdout } = await exec(cmd);
    if (errno !== 0 || !stdout) return 'unknown';
    return stdout.trim().replace(/^version=/, '');
}
