#!/bin/ash

IP=$1
NM=$2
GW=$3
DNS1=$4
MAXDELAY=60

echo "I: Requested params:"
echo "I: IP     : ${IP}"
echo "I: NETMASK: ${NM}"
echo "I: GATEWAY: ${GW}"
echo "I: DNS    : ${DNS1}"

echo "I: bringing eth0 up"
/sbin/ifconfig eth0 ${IP} netmask ${NM}

echo "I: waiting interface"
TE=0
while [ ${TE} -lt ${MAXDELAY} ] ; do
    let TE=${TE}+1
    S=$( /sbin/ifconfig | grep eth0 )
    if [ ! -z "${S}" ]; then
        sleep 1
        break
    fi
done
if [ ${TE} -eq ${MAXDELAY} ]; then
    echo "E: interface timed out"
fi
echo "I: setting default gateway"
/sbin/route add default gw ${GW}
echo "I: setting DNS"
cat > /etc/resolv.conf <<EOF
nameserver ${DNS1}
EOF

/sbin/ifconfig
