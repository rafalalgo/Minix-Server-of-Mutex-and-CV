How to install the server and run tests?

steps 2 and 3 can be performed simply by running
/mnt/stuff/install.sh
/mnt/stuff/install_to_file.sh

if permission is denied run before
chmod u+rwx install.sh

=================================
0. Copy files to the shared folder
=================================
shared/project
shared/tests
shared/stuff

=================================
1. Run MINIX
=================================
login to root
mount -t vbfs -o share=Shared none /mnt/

=================================
2. Copy files to the system
=================================
cp -r /mnt/project/* /usr/src/

=================================
3. Install the server
=================================
cd /usr/src/etc/usr/ ; make install
cd /usr/src/include/ ; make ; make install
cd /usr/src/servers/cv ; make ; make install
cd /usr/src/servers/pm ; make ; make install
cd /usr/src/releasetools/ ; make includes ; make install
cd /usr/src/lib/ ; make ; make install
poweroff

=================================
4. Reboot
=================================
login to root
mount -t vbfs -o share=Shared none /mnt/

=================================
5. Start the server
=================================
service up /usr/sbin/cv

=================================
6. Run tests
=================================
cd /mnt/stuff ; ./run_all.sh
cd /mnt/stuff ; ./run_all_to_file.sh

