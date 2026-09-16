// index.js - HID Gadget WebUI entrypoint.
// Author: kelexine <https://github.com/kelexine>
import '@material/web/tabs/tabs.js';
import '@material/web/tabs/primary-tab.js';
import '@material/web/button/filled-button.js';
import '@material/web/button/outlined-button.js';
import '@material/web/textfield/outlined-text-field.js';
import '@material/web/select/outlined-select.js';
import '@material/web/select/select-option.js';

import {
    sendText, sendCombo, mouseMove, mouseClick, mouseScroll,
    consumerAction, runDucky, reRunSetup, getVersion,
} from './utils/hid.js';

const PANELS = ['keyboard', 'mouse', 'consumer', 'ducky', 'status'];

function showToast(message) {
    const el = document.getElementById('toast');
    el.textContent = message;
    el.classList.remove('hidden');
    clearTimeout(showToast._t);
    showToast._t = setTimeout(() => el.classList.add('hidden'), 2200);
}

function logStatus(line) {
    const box = document.getElementById('status-log');
    box.textContent = `${box.textContent}\n${line}`.trim();
    box.scrollTop = box.scrollHeight;
}

async function runAndReport(promise, okMessage) {
    try {
        const { errno, stdout, stderr } = await promise;
        if (errno !== 0) {
            showToast(`Failed (errno ${errno})`);
            logStatus(`[!] ${stderr || stdout || 'unknown error'}`);
            return false;
        }
        if (okMessage) showToast(okMessage);
        if (stdout) logStatus(stdout.trim());
        return true;
    } catch (err) {
        showToast(err.message || 'Command failed');
        logStatus(`[!] ${err.message || err}`);
        return false;
    }
}

function setupTabs() {
    const tabs = document.getElementById('tabs');
    tabs.addEventListener('change', () => {
        const active = PANELS[tabs.activeTabIndex] || PANELS[0];
        PANELS.forEach((name) => {
            document.getElementById(`panel-${name}`).classList.toggle('hidden', name !== active);
        });
    });
}

function setupKeyboardPanel() {
    const activeMods = new Set();
    document.querySelectorAll('.mod-chip').forEach((chip) => {
        chip.addEventListener('click', () => {
            const mod = chip.dataset.mod;
            if (activeMods.has(mod)) {
                activeMods.delete(mod);
                chip.classList.remove('active');
            } else {
                activeMods.add(mod);
                chip.classList.add('active');
            }
        });
    });

    document.querySelectorAll('.key-btn').forEach((btn) => {
        btn.addEventListener('click', async () => {
            const combo = [...activeMods, btn.dataset.key].join('-');
            await runAndReport(sendCombo(combo), `Sent ${combo}`);
        });
    });

    document.getElementById('kb-send').addEventListener('click', async () => {
        const field = document.getElementById('kb-text');
        if (!field.value) return;
        await runAndReport(sendText(field.value), 'Text sent');
    });
}

function setupMousePanel() {
    document.querySelectorAll('[data-click]').forEach((btn) => {
        btn.addEventListener('click', async () => {
            await runAndReport(mouseClick(btn.dataset.click), `${btn.dataset.click} click`);
        });
    });

    document.getElementById('scroll-up').addEventListener('click', () => {
        runAndReport(mouseScroll(-5));
    });
    document.getElementById('scroll-down').addEventListener('click', () => {
        runAndReport(mouseScroll(5));
    });

    const pad = document.getElementById('mouse-pad');
    let last = null;
    let pending = false;

    const flush = async (dx, dy) => {
        if (pending) return;
        pending = true;
        await mouseMove(dx, dy).catch(() => {});
        pending = false;
    };

    pad.addEventListener('pointerdown', (e) => {
        pad.setPointerCapture(e.pointerId);
        last = { x: e.clientX, y: e.clientY };
    });
    pad.addEventListener('pointermove', (e) => {
        if (!last) return;
        const dx = e.clientX - last.x;
        const dy = e.clientY - last.y;
        last = { x: e.clientX, y: e.clientY };
        if (Math.abs(dx) < 1 && Math.abs(dy) < 1) return;
        flush(dx, dy);
    });
    ['pointerup', 'pointercancel', 'pointerleave'].forEach((ev) => {
        pad.addEventListener(ev, () => { last = null; });
    });
}

function setupConsumerPanel() {
    document.querySelectorAll('.action-btn').forEach((btn) => {
        btn.addEventListener('click', async () => {
            await runAndReport(consumerAction(btn.dataset.action), btn.textContent);
        });
    });
}

function setupDuckyPanel() {
    document.getElementById('ducky-run').addEventListener('click', async () => {
        const script = document.getElementById('ducky-script').value;
        const os = document.getElementById('ducky-os').value || 'WINDOWS';
        if (!script.trim()) {
            showToast('Script is empty');
            return;
        }
        await runAndReport(runDucky(script, os), 'DuckyScript executed');
    });
}

function setupStatusPanel() {
    document.getElementById('status-resetup').addEventListener('click', async () => {
        logStatus('[*] Re-running hid-setup...');
        await runAndReport(reRunSetup(), 'HID setup re-applied');
    });
}

async function init() {
    setupTabs();
    setupKeyboardPanel();
    setupMousePanel();
    setupConsumerPanel();
    setupDuckyPanel();
    setupStatusPanel();

    document.getElementById('module-version').textContent = `v${await getVersion()}`;
}

init();
