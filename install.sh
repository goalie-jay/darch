#!/bin/bash

./build.sh
sudo cp ./build/darch /usr/local/bin/darch

if [ $? -eq 0 ]; then
	echo
	echo "[+] Installed darch successfully -> /usr/local/bin/darch."
else
	echo
	echo "[-] Installation failure."
fi