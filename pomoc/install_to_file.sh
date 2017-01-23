#!/bin/sh

# installing the server + saving logs to /mnt/minix/pomoc/results/installation

service down cv

cp -r /mnt/minix/project/* /usr/src/

cd /usr/src/etc/usr/
make install > /mnt/minix/pomoc/results/installation/1-etc-usr.out

cd /usr/src/include/
make > /mnt/minix/pomoc/results/installation/2-include-make.out
make install > /mnt/minix/pomoc/results/installation/3-include-make-install.out

cd /usr/src/servers/cv
make > /mnt/minix/pomoc/results/installation/4-cv-make.out
make install > /mnt/minix/pomoc/results/installation/5-cv-make-install.out

cd /usr/src/servers/pm
make > /mnt/minix/pomoc/results/installation/6-pm-make.out
make install > /mnt/minix/pomoc/results/installation/7-pm-make-install.out

cd /usr/src/releasetools/
make includes > /mnt/minix/pomoc/results/installation/8-make-includes.out
make install > /mnt/minix/pomoc/results/installation/9-make-install.out

cd /usr/src/lib/
make > /mnt/minix/pomoc/results/installation/10-lib-make.out
make install > /mnt/minix/pomoc/results/installation/11-lib-make-istall.out

poweroff