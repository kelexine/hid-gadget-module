#!/usr/bin/env python3
import os
import sys
import subprocess
import glob

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TEST_DIR)
MOCK_BIN = os.path.join(TEST_DIR, "hid-gadget-test")
CASES_DIR = os.path.join(TEST_DIR, "cases")

def compile_mock():
    print("[*] Compiling mock executable...")
    cmd = [
        "gcc", "-Wall", "-Wextra", "-O2", "-Iinclude",
        "-DMOCK_HID",
        "-o", MOCK_BIN,
        os.path.join(ROOT_DIR, "src/hid-gadget.c"),
        os.path.join(ROOT_DIR, "src/tui.c"),
        os.path.join(ROOT_DIR, "src/ducky.c")
    ]
    try:
        subprocess.check_call(cmd, cwd=ROOT_DIR)
        print("[+] Compilation successful.")
        return True
    except subprocess.CalledProcessError:
        print("[-] Compilation failed.")
        return False

def run_test_case(ducky_file):
    case_name = os.path.basename(ducky_file)
    expected_file = ducky_file.replace(".ducky", ".expected")

    if not os.path.exists(expected_file):
        print(f"[!] Warning: No expected output for {case_name}. Skipping.")
        return False

    # Run the mock executable
    try:
        # We need to set up environment variables if tests depend on them,
        # but for now basic tests shouldn't need them.
        result = subprocess.run(
            [MOCK_BIN, "ducky", ducky_file],
            capture_output=True,
            text=True,
            timeout=5
        )
    except subprocess.TimeoutExpired:
        print(f"[-] {case_name}: Timed out.")
        return False

    # Filter standard output
    # We only care about "[HID-MOCK] Writing ..." lines and maybe "[Ducky]" logs
    # But strictly speaking, the binary output (Writing X bytes) is what matters for HID correctness.
    actual_lines = [
        line.strip()
        for line in result.stdout.splitlines()
        if line.strip() and not line.strip().startswith("[HID-MOCK] Mocking device paths")
    ]

    with open(expected_file, "r") as f:
        expected_lines = [line.strip() for line in f.read().splitlines() if line.strip()]

    # Simple comparison
    if actual_lines == expected_lines:
        print(f"[+] {case_name}: PASS")
        return True
    else:
        print(f"[-] {case_name}: FAIL")
        print("    Expected:")
        for l in expected_lines[:5]: print(f"      {l}")
        if len(expected_lines) > 5: print("      ...")
        print("    Actual:")
        for l in actual_lines[:5]: print(f"      {l}")
        if len(actual_lines) > 5: print("      ...")
        return False

def main():
    if not compile_mock():
        sys.exit(1)

    print(f"[*] Running tests from {CASES_DIR}...")
    ducky_files = sorted(glob.glob(os.path.join(CASES_DIR, "*.ducky")))

    if not ducky_files:
        print("[!] No test cases found.")
        sys.exit(0)

    passed = 0
    total = 0

    for df in ducky_files:
        total += 1
        if run_test_case(df):
            passed += 1

    print("-" * 40)
    print(f"Results: {passed}/{total} passed.")

    if os.path.exists(MOCK_BIN):
        os.remove(MOCK_BIN)

    if passed != total:
        sys.exit(1)

if __name__ == "__main__":
    main()
