#!/bin/bash
echo "=== Serial Port Diagnostics ==="
echo ""
echo "1. USB Devices:"
lsusb | grep 2e8a
echo ""
echo "2. Serial Ports:"
ls -l /dev/ttyACM* 2>/dev/null || echo "No /dev/ttyACM* found"
echo ""
echo "3. Processes Using Port:"
lsof /dev/ttyACM0 2>/dev/null || echo "No process using /dev/ttyACM0"
echo ""
echo "4. User Groups:"
groups $USER
echo ""
echo "5. Port Permissions:"
ls -l /dev/ttyACM0 2>/dev/null || echo "Port not found"
