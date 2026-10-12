#!/bin/sh
# macOS / Linux: build the console version (the GUI version is Windows only)
gcc -Wall -Wextra -o laundry laundry_console.c laundry_core.c && echo "Done. Run ./laundry"
