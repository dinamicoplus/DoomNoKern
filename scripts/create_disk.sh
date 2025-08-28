hdiutil attach -nomount disk.img
sudo mkdir -p /Volumes/LIMINE
sudo mount -t msdos /dev/disk5s1 /Volumes/LIMINE
cp limine/limine-bios.sys /Volumes/LIMINE/
cp limine.cfg             /Volumes/LIMINE/
cp kernel.elf             /Volumes/LIMINE/

sync
sudo umount /Volumes/LIMINE
hdiutil detach /dev/disk5
echo ok