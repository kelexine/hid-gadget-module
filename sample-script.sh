#!/system/bin/sh
# sample-script.sh
# Advanced script for opening a browser and searching for keywords
# Works across Android, iOS, Linux, Windows, and macOS
# Needs further improvements as i don't have access to all these devices
# For use with Magisk HID Gadget Module

# Print usage information
print_usage() {
  echo "Usage: $0 [options] \"search query\""
  echo "Options:"
  echo "  -h, --help      Show this help message"
  echo "  -w, --wait N    Wait N seconds between operations (default: 2)"
  echo "  -o, --os TYPE   Specify OS type (windows, macos, linux, android, ios)"
  echo "                  If not specified, will attempt auto-detection"
  echo "  -b, --browser NAME  Specify browser to use (chrome, firefox, safari, edge)"
  echo "                      If not specified, will use system default"
  exit 1
}

# Set default values
WAIT_TIME=2
OS_TYPE=""
BROWSER=""
SEARCH_QUERY=""

# Parse command line arguments
while [ "$1" != "" ]; do
  case $1 in
    -h | --help)
      print_usage
      ;;
    -w | --wait)
      shift
      WAIT_TIME=$1
      ;;
    -o | --os)
      shift
      OS_TYPE=$1
      ;;
    -b | --browser)
      shift
      BROWSER=$1
      ;;
    *)
      SEARCH_QUERY="$1"
      ;;
  esac
  shift
done

# Check if search query is provided
if [ -z "$SEARCH_QUERY" ]; then
  echo "Error: No search query provided"
  print_usage
fi

# Function for simulating wait
wait_sec() {
  sleep "$1"
}

# Function to send keyboard combination
send_key_combo() {
  hid-keyboard "$1"
  wait_sec 0.5
}

# Function to type text
type_text() {
  hid-keyboard "$1"
  wait_sec 0.5
}

# Function to press enter
press_enter() {
  hid-keyboard ENTER
  wait_sec 0.5
}

# Function to encode URL
encode_url() {
  # Basic URL encoding - replace spaces with plus signs
  echo "$1" | sed 's/ /+/g'
}

# Function to detect OS type if not specified
detect_os() {
  if [ -z "$OS_TYPE" ]; then
    echo "OS type not specified, attempting auto-detection via keyboard shortcuts..."
    echo "Press Ctrl+Esc and check device response to determine if Windows..."
    send_key_combo "CTRL-ESC"
    wait_sec 2
    echo "Press Command+Space and check device response to determine if macOS..."
    send_key_combo "GUI-SPACE"
    wait_sec 2
    echo "Auto-detection limited. Please specify OS with -o option for better results."
    echo "Defaulting to Windows keyboard shortcuts."
    OS_TYPE="windows"
  fi
  echo "Using OS type: $OS_TYPE"
}

# Function to open browser and search on Windows
open_browser_windows() {
  echo "Opening browser on Windows..."
  
  if [ "$BROWSER" = "chrome" ]; then
    # Open Chrome
    send_key_combo "GUI-r"
    wait_sec "$WAIT_TIME"
    type_text "chrome"
    press_enter
  elif [ "$BROWSER" = "firefox" ]; then
    # Open Firefox
    send_key_combo "GUI-r"
    wait_sec "$WAIT_TIME"
    type_text "firefox"
    press_enter
  elif [ "$BROWSER" = "edge" ]; then
    # Open Edge
    send_key_combo "GUI-r"
    wait_sec "$WAIT_TIME"
    type_text "msedge"
    press_enter
  else
    # Open default browser
    send_key_combo "GUI-r"
    wait_sec "$WAIT_TIME"
    type_text "https://www.google.com"
    press_enter
  fi
  
  wait_sec "$WAIT_TIME"
  
  # Wait for browser to load
  wait_sec 3
  
  # Open a new tab just in case
  if [ "$BROWSER" != "" ]; then
    send_key_combo "CTRL-t"
    wait_sec "$WAIT_TIME"
  fi
  
  # Type the search query
  encoded_query=$(encode_url "$SEARCH_QUERY")
  type_text "https://www.google.com/search?q=$encoded_query"
  press_enter
}

# Function to open browser and search on macOS
open_browser_macos() {
  echo "Opening browser on macOS..."
  
  if [ "$BROWSER" = "chrome" ]; then
    # Open Chrome
    send_key_combo "GUI-SPACE"
    wait_sec "$WAIT_TIME"
    type_text "chrome"
    press_enter
  elif [ "$BROWSER" = "firefox" ]; then
    # Open Firefox
    send_key_combo "GUI-SPACE"
    wait_sec "$WAIT_TIME"
    type_text "firefox"
    press_enter
  elif [ "$BROWSER" = "safari" ]; then
    # Open Safari
    send_key_combo "GUI-SPACE"
    wait_sec "$WAIT_TIME"
    type_text "safari"
    press_enter
  else
    # Open default browser with direct URL
    send_key_combo "GUI-SPACE"
    wait_sec "$WAIT_TIME"
    type_text "https://www.google.com"
    press_enter
  fi
  
  wait_sec "$WAIT_TIME"
  
  # Wait for browser to load
  wait_sec 3
  
  # Open a new tab just in case
  if [ "$BROWSER" != "" ]; then
    send_key_combo "GUI-t"
    wait_sec "$WAIT_TIME"
  fi
  
  # Type the search query
  encoded_query=$(encode_url "$SEARCH_QUERY")
  type_text "https://www.google.com/search?q=$encoded_query"
  press_enter
}

# Function to open browser and search on Linux
open_browser_linux() {
  echo "Opening browser on Linux..."
  
  # Open terminal first
  send_key_combo "CTRL-ALT-t"
  wait_sec "$WAIT_TIME"
  
  if [ "$BROWSER" = "chrome" ]; then
    # Open Chrome
    type_text "google-chrome"
    press_enter
  elif [ "$BROWSER" = "firefox" ]; then
    # Open Firefox
    type_text "firefox"
    press_enter
  elif [ "$BROWSER" = "brave-browser" ]; then
    # Open Brave
    type_text "brave"
  else
    # Try to detect and use default browser
    type_text "xdg-open https://www.google.com"
    press_enter
  fi
  
  wait_sec "$WAIT_TIME"
  
  # Wait for browser to load
  wait_sec 3
  
  # Open a new tab
  if [ "$BROWSER" != "" ]; then
    send_key_combo "CTRL-t"
    wait_sec "$WAIT_TIME"
  fi
  
  # Type the search query
  encoded_query=$(encode_url "$SEARCH_QUERY")
  type_text "https://www.google.com/search?q=$encoded_query"
  press_enter
}

# Function to open browser and search on Android
open_browser_android() {
  echo "Opening browser on Android..."
  
  # Press home button first
  hid-keyboard HOME
  wait_sec "$WAIT_TIME"
  
  # Open search/app drawer
  send_key_combo "GUI"
  wait_sec "$WAIT_TIME"
  
  if [ "$BROWSER" = "chrome" ]; then
    # Type Chrome
    type_text "chrome"
  elif [ "$BROWSER" = "firefox" ]; then
    # Type Firefox
    type_text "firefox"
  else
    # Type Browser (generic)
    type_text "brave"
  fi
  
  wait_sec "$WAIT_TIME"
  press_enter
  wait_sec 3
  
  # Tap on address bar (usually)
  send_key_combo "ALT-d"
  wait_sec "$WAIT_TIME"
  
  # Type the search query
  encoded_query=$(encode_url "$SEARCH_QUERY")
  type_text "https://www.google.com/search?q=$encoded_query"
  press_enter
}

# Function to open browser and search on iOS
open_browser_ios() {
  echo "Opening browser on iOS..."
  
  # Press home button first
  hid-keyboard HOME
  wait_sec "$WAIT_TIME"
  
  # Swipe down to open Spotlight search
  hid-mouse down
  hid-mouse move 0 100
  hid-mouse up
  wait_sec "$WAIT_TIME"
  
  if [ "$BROWSER" = "chrome" ]; then
    # Type Chrome
    type_text "chrome"
  elif [ "$BROWSER" = "firefox" ]; then
    # Type Firefox
    type_text "firefox"
  else
    # Type Safari (default)
    type_text "safari"
  fi
  
  wait_sec "$WAIT_TIME"
  press_enter
  wait_sec 3
  
  # Tap on address bar (usually at top)
  hid-mouse move 0 -50
  hid-mouse click
  wait_sec "$WAIT_TIME"
  
  # Type the search query
  encoded_query=$(encode_url "$SEARCH_QUERY")
  type_text "https://www.google.com/search?q=$encoded_query"
  press_enter
}

# Main script execution
echo "HID Browser Search Tool"
echo "----------------------"
echo "Query: $SEARCH_QUERY"

# Detect OS if not specified
detect_os

# Open browser and search based on OS type
case "$OS_TYPE" in
  windows)
    open_browser_windows
    ;;
  macos)
    open_browser_macos
    ;;
  linux)
    open_browser_linux
    ;;
  android)
    open_browser_android
    ;;
  ios)
    open_browser_ios
    ;;
  *)
    echo "Error: Unknown OS type: $OS_TYPE"
    echo "Supported OS types: windows, macos, linux, android, ios"
    exit 1
    ;;
esac

echo "Search operation completed"
exit 0
