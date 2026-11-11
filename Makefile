# CreoOS — NO BLOAT makefile
#   make            GNU cross toolchain (aarch64-linux-gnu-)
#   make LLVM=1     clang + lld + llvm-objcopy (no cross gcc needed)
ARCH      := aarch64
CROSS     ?= aarch64-linux-gnu-

ifdef LLVM
CC        := clang --target=aarch64-none-elf
OBJCOPY   := llvm-objcopy
LDDRIVER  := -fuse-ld=lld
else
CC        := $(CROSS)gcc
OBJCOPY   := $(CROSS)objcopy
LDDRIVER  :=
endif

# QEMU boot target first; beyond2lte board target comes after bring-up
DEFINES   := -DQEMU
# -mstrict-align: with the MMU off all RAM is Device memory, where an
# unaligned access is an alignment fault.
CFLAGS    := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
             -fno-omit-frame-pointer -mgeneral-regs-only -mstrict-align -Wall -Wextra \
             -O2 -g -Iinclude $(DEFINES)
ASFLAGS   := $(CFLAGS)
LDCOMMON  := $(LDDRIVER) -nostdlib -Wl,-z,max-page-size=4096 -Wl,--build-id=none
LDFLAGS   := $(LDCOMMON) -T linker.ld -Wl,-Map,build/creo.map

SRCS      := kern/entry.S kern/vectors.S kern/uart.c kern/kprintf.c \
             kern/mm.c kern/timer.c kern/sched.c kern/main.c kern/util.c \
             ui/ui.c user/init.c
OBJS      := $(patsubst %.c,build/%.o,$(SRCS))
OBJS      := $(OBJS:.S=.o)

QEMU      := qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -smp 1 -m 1024M \
             -kernel build/kern/creo.elf

all: image

build:
	@mkdir -p build/kern build/ui build/user

image: build/kern/creo.elf
	$(OBJCOPY) -O binary build/kern/creo.elf build/creo.img
	@echo "==> build/creo.img  [NO BLOAT]"

build/kern/creo.elf: $(OBJS) linker.ld
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS)

build/%.o: %.c include/creo/types.h | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.S | build
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

phone: build/phone/boot.img

PHONE_OBJS := $(subst build/,build/phone-objs/,$(OBJS)) \
              build/phone-objs/kern/tgt-exynos.o build/phone-objs/kern/tgt-fbprobe.o
PHONE_CFLAGS := $(CFLAGS) -DCREO_PHONE

build/phone-objs/%.o: %.c include/creo/types.h | build
	@mkdir -p $(dir $@)
	$(CC) $(PHONE_CFLAGS) -c $< -o $@

build/phone-objs/%.o: %.S | build
	@mkdir -p $(dir $@)
	$(CC) $(PHONE_CFLAGS) -c $< -o $@

build/phone/creo-phone.elf: $(PHONE_OBJS) linker-phone.ld
	@mkdir -p build/phone
	$(CC) $(PHONE_OBJS) -o $@ $(LDCOMMON) -T linker-phone.ld

build/phone/boot.img: build/phone/creo-phone.elf
	$(OBJCOPY) -O binary build/phone/creo-phone.elf build/phone/creo-phone.bin
	python3 scripts/mkbootimg.py --kernel build/phone/creo-phone.bin \
	  --cmdline "creo" --out build/phone/boot.img

# Always boot the ELF with -kernel; `-device loader` silently drops chunks.
run: image
	$(QEMU)

run-gdb: image
	$(QEMU) -gdb tcp::1234 -S

clean:
	rm -rf build

.PHONY: all image phone run run-gdb clean
