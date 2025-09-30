#!/bin/bash

whichOutput=$(which python3)

if [[ $? -ne 0 ]]; then
	echo '[-] Python3 not found, cannot proceed.'
	exit
fi

echo '#!'$whichOutput > tmp.py
cat darch-list.py >> tmp.py
chmod +x tmp.py

sudo cp ./tmp.py /usr/local/bin/darch-list

if [ $? -eq 0 ]; then
	echo
	echo '[+] Installed darch-list script successfully -> /usr/local/bin/darch-list,'
else
	echo
	echo '[-] Installation failure.'
fi

rm tmp.py