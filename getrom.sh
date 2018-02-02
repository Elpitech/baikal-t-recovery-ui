#!/bin/ash

VAR_PREFIX=/var/run/recoveryui/
URL=$1

cd ${VAR_PREFIX}
TIMEOUT=30
echo "I: Fetching ${URL}"
if [ -e /tmp/update-web.rom ]; then rm /tmp/update-web.rom; fi
echo "${VAR_PREFIX}: /usr/bin/wget -T ${TIMEOUT} "${URL}" -O /tmp/update-web.rom"
/usr/bin/wget -T ${TIMEOUT} "${URL}" -O /tmp/update-web.rom
RET=$?
if [ ${RET} -ne 0 ]; then
    if [ -e /tmp/update-web.rom ]; then rm /tmp/update-web.rom; fi
    echo "E: Failed to fetch ${URL}"
    read -n1 -r -p "Press any key to continue..." key
else
    printf "/tmp/update-web.rom" > ${VAR_PREFIX}web-update-rom-path
    echo "I: Fetch ${URL} done"
fi
