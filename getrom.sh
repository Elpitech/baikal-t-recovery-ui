#!/bin/ash

VAR_PREFIX=/var/run/recoveryui
URL=$1

cd ${VAR_PREFIX}
echo "I: Fetching ${URL}"
/usr/bin/wget "${URL}" -O update.rom
RET=$?
if [ ${RET} -ne 0 ]; then
    if [ -e update.rom ]; then rm update.rom; fi
    echo "E: Failed to fetch ${URL}"
    read -n1 -r -p "Press any key to continue..." key
fi
echo "I: Fetch ${URL} done"
sleep 2
