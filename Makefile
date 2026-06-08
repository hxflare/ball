CC          := gcc
CFLAGS      := -static
LIBS_DIR    := libs
PROG_DIR    := programs
LINUX_DIR   := building/.linux
OUTDIR      := building/isoroot/initramfs/bin
OUTDIR_STAMP := $(OUTDIR)/.stamp
ISO         := building/ball-linux.iso
INITRD      := building/isoroot/boot/initrd.img
VMLINUZ     := building/isoroot/boot/vmlinuz

LIB_SRCS := $(wildcard $(LIBS_DIR)/*.c)
PROG_SRCS := $(wildcard $(PROG_DIR)/*.c)
BINS := $(PROG_SRCS:.c=)

.PHONY: all iso run clean rebuild rebuild-linux menuconfig

all: $(BINS)
iso: all $(ISO)

# run iso
run: iso
	qemu-system-x86_64 -cdrom $(ISO)

# compilation
$(PROG_DIR)/%: $(PROG_DIR)/%.c $(LIB_SRCS) btools.h bsys.h
	$(CC) $(CFLAGS) -o $@ $< $(LIB_SRCS)

# copy into initramfs/bin
$(OUTDIR_STAMP): $(BINS)
	mkdir -p $(OUTDIR)
	cp $(BINS) $(OUTDIR)/
	touch $(OUTDIR_STAMP)

# build initrd
$(INITRD): $(OUTDIR_STAMP)
	cd building/isoroot/initramfs && \
		find . | cpio -oH newc | gzip > ../boot/initrd.img

# build iso
$(ISO): $(INITRD) $(VMLINUZ)
	grub-mkrescue -o $(ISO) building/isoroot/

# clean up
clean:
	rm -f $(BINS)
	rm -rf $(OUTDIR)
	rm -f $(INITRD) $(ISO)

# rebuild linux kernel
rebuild-linux:
	$(MAKE) -j$(nproc) -C $(LINUX_DIR)
	rm -f $(VMLINUZ)
	cp $(LINUX_DIR)/arch/x86/boot/bzImage $(VMLINUZ)

# rebuild all
rebuild: clean all rebuild-linux

# menuconfig then rebuild
menuconfig:
	$(MAKE) -C $(LINUX_DIR) menuconfig
	$(MAKE) rebuild
