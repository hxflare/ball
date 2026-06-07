CC      := gcc
CFLAGS  := -static -Wall -Wextra

LIBS_DIR    := libs
PROG_DIR    := programs
OUTDIR      := building/isoroot/initramfs/bin
ISO         := building/ball-linux.iso
INITRD      := building/isoroot/boot/initrd.img

LIB_SRCS := $(wildcard $(LIBS_DIR)/*.c)

PROG_SRCS := $(wildcard $(PROG_DIR)/*.c)

BINS := $(PROG_SRCS:.c=)

# targets

.PHONY: all iso run clean

all: $(BINS)

iso: all $(ISO)
# run iso
run: iso
	qemu-system-x86_64 -cdrom $(ISO)

# compilation

$(PROG_DIR)/%: $(PROG_DIR)/%.c $(LIB_SRCS) btools.h bsys.h
	$(CC) $(CFLAGS) -o $@ $< $(LIB_SRCS)

# copy into initramfs/bin dir

$(OUTDIR): $(BINS)
	mkdir -p $(OUTDIR)
	cp $(BINS) $(OUTDIR)/

# build iso

$(INITRD): $(OUTDIR)
	cd building/isoroot/initramfs && \
		find . | cpio -oH newc | gzip > ../boot/initrd.img

$(ISO): $(INITRD)
	grub-mkrescue -o $(ISO) building/isoroot/

#clean up

clean:
	rm -f $(BINS)
	rm -rf $(OUTDIR)
	rm -f $(INITRD) $(ISO)
