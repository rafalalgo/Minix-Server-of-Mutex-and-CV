#!/bin/sh

# rebuilds the server

service down cv
rm -r /usr/src/servers/cv
cp -r /mnt/project/servers/cv /usr/src/servers/
cd /usr/src/servers/cv ; make ; make install
cd /mnt/stuff/
service up /usr/sbin/cv