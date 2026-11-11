# CreoOS — NO BLOAT makefile
ARCH      := aarch64
CROSS     ?= aarch64-linux-gnu-
CC        := $(CROSS)gcc

# QEMU boot target first; beyond2lte board target comes after bring-up
DEFINES   := -DQEMU
CFLAGS    := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
             -fno-omit-frame-pointer -mgeneral-regs-only -Wall -Wextra \
             -O2 -g -Iinclude $(DEFINES)
ASFLAGS   := $(CFLAGS)
LDFLAGS   := -nostdlib -nostartfiles -T linker.ld -Wl,-z,max-page-size=4096 -Wl,--build-id=none \
             -Wl,-Map,build/creo.map

SRCS      := kern/entry.S kern/vectors.S kern/uart.c kern/kprintf.c \
             kern/mm.c kern/timer.c kern/sched.c kern/main.c kern/util.c \
             ui/ui.c user/init.c
OBJS      := $(patsubst %.c,build/%.o,$(SRCS))
OBJS      := $(OBJS:.S=.o)

all: image

build:
	@mkdir -p build/kern build/ui build/user

image: build/kern/creo.elf
	$(CROSS)objcopy -O binary build/kern/creo.elf build/creo.img
	@echo "==> build/creo.img  [NO BLOAT]"

build/kern/creo.elf: $(OBJS)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS)

build/%.o: %.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.S | build
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

phone: build/phone/boot.img

PHONE_OBJS := $(subst build/,build/phone-objs/,$(OBJS)) \
              build/phone-objs/kern/tgt-exynos.o build/phone-objs/kern/tgt-fbprobe.o
PHONE_CFLAGS := $(CFLAGS) -DCREO_PHONE

build/phone-objs/%.o: %.c | build
	@mkdir -p $(dir $@)
	$(CC) $(PHONE_CFLAGS) -c $< -o $@

$(subst .c,.o,$(PHONE_SOURCES)) build/phone/creo-phone-ua: ;

build/phone/creo-phone.elf: $(PHONE_OBJS)
	@mkdir -p build/phone
	$(CC) $(PHONE_OBJS) -o $@ -nostdlib -nostartfiles \
	  -T linker-phone.ld -Wl,-z,max-page-size=4096 -Wl,--build-id=none

build/phone/boot.img: build/phone/creo-phone.elf
	$(CROSS)objcopy -O binary build/phone/creo-phone.elf build/phone/creo-phone.bin
	python3 scripts/mkbootimg.py --kernel build/phone/creo-phone.bin \
	  --cmdline "creo" --out build/phone/boot.img

run: image
	qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic \
	  -device loader,file=build/creo.elf,addr=0x40080000,cpu-num=0 \
	  -smp 1 -m 1024M

run-gdb: image
	qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic \
	  -device loader,file=build/creo.elf,addr=0x40080000,cpu-num=0 \
	  -smp 1 -m 1024M -gdb tcp::1234 -S

clean:
	rm -rf build

.PHONY: all image phone run run-gdb clean
