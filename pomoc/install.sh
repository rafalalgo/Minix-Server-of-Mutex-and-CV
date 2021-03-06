#!/bin/sh

# installing the server

service down cv

cp -r /mnt/minix/projekt/* /usr/src/

rm /mnt/minix/pomoc/out/*

cd /usr/src/etc/usr/ ; make install
cd /usr/src/include/ ; make ; make install
cd /usr/src/servers/cv ; make ; make install
cd /usr/src/servers/pm ; make ; make install
cd /usr/src/releasetools/ ; make ; make install
cd /usr/src/lib/ ; make ; make install

poweroff