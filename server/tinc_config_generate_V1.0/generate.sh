#!/bin/sh
CLIENT_IP=$1
OLD_CLIENT_IP=172.16.0.3
SERVER_IP=$2

GUEST_ID=$(cat /proc/sys/kernel/random/uuid|md5sum|head -c 10)
OLD_GUEST_ID=69dc618619

#SERVER_KEY_PATH=/root/Desktop/tinc_config
SERVER_KEY_PATH=/usr/local/etc/tinc/gfw/hosts

if [ ! -n "$SERVER_IP" ]
then
echo please input server_ip
exit
fi

if [ ! -d $SERVER_IP ]
then
echo $SERVER_IP not exist
exit
fi

if [ -d $CLIENT_IP ]
then
echo $CLIENT_IP exist
exit
fi

rm -rf /etc/keys
mkdir /etc/keys
echo -e '\n\n\n\n' | tinc -c /etc/keys generate-keys

mkdir -p $CLIENT_IP/gfw/hosts
cp -r $SERVER_KEY_PATH/gfw_server $CLIENT_IP/gfw/hosts

#client priv key
cp -r /etc/keys/rsa_key.priv $CLIENT_IP/gfw/rsa_key.priv
cp -r /etc/keys/ed25519_key.priv $CLIENT_IP/gfw/ed25519_key.priv

#client pub key, copy to server hosts
cat /etc/keys/rsa_key.pub > $CLIENT_IP/gfw/hosts/$GUEST_ID
cat /etc/keys/ed25519_key.pub >> $CLIENT_IP/gfw/hosts/$GUEST_ID
cp -r $CLIENT_IP/gfw/hosts/$GUEST_ID $SERVER_KEY_PATH/$GUEST_ID

#client tinc.conf
cp -r gfw_client/tinc.conf $CLIENT_IP/gfw/tinc.conf
sed -i 's/'"$OLD_GUEST_ID"'/'"$GUEST_ID"'/g' $CLIENT_IP/gfw/tinc.conf

#client tinc-up, tinc-down
cp -r gfw_client/tinc-up $CLIENT_IP/gfw/tinc-up
cp -r gfw_client/tinc-down $CLIENT_IP/gfw/tinc-down
sed -i 's/'"$OLD_CLIENT_IP"'/'"$CLIENT_IP"'/g' $CLIENT_IP/gfw/tinc-up

cd $CLIENT_IP
tar -zcvf $CLIENT_IP.tar.gz gfw/



