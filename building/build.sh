echo "Compiling the coreutils..."
rm -f isoroot/boot/initrd.img
cd ../programs
./compileall.sh
cd ../building
echo "Complete.
Building the ISO"
rm -f ball-linux.iso
cd isoroot/initramfs
find . | cpio -oH newc | gzip > ../initrd.img
cd ../boot
mv ../initrd.img initrd.img
cd ../..
grub-mkrescue -o ball-linux.iso isoroot/
echo "Complete.
Launching QEMU test"
qemu-system-x86_64 -cdrom ball-linux.iso
