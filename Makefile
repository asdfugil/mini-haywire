RUSTARCH ?= armv7a-none-eabi

USE_CLANG = 1
ifeq ($(shell uname),Darwin)
USE_CLANG ?= 1
$(info INFO: Building on Darwin)
BREW ?= $(shell command -v brew)
TOOLCHAIN ?= $(shell $(BREW) --prefix llvm)/bin/
ifeq ($(shell ls $(TOOLCHAIN)/ld.lld 2>/dev/null),)
LLDDIR ?= $(shell $(BREW) --prefix lld)/bin/
else
LLDDIR ?= $(TOOLCHAIN)
endif
$(info INFO: Toolchain path: $(TOOLCHAIN))
endif

ifeq ($(shell uname -m),arm)
ARCH ?=
else
ARCH ?= arm-linux-gnueabi-
endif

ifneq ($(TOOLCHAIN),$(LLDDIR))
$(info INFO: LLD path: $(LLDDIR))
endif

ifeq ($(USE_CLANG),1)
CC := $(TOOLCHAIN)clang --target=$(ARCH)
AS := $(TOOLCHAIN)clang --target=$(ARCH)
LD := $(LLDDIR)ld.lld
OBJCOPY := $(TOOLCHAIN)llvm-objcopy
CLANG_FORMAT ?= $(TOOLCHAIN)clang-format
EXTRA_CFLAGS ?=
else
CC := $(TOOLCHAIN)$(ARCH)gcc
AS := $(TOOLCHAIN)$(ARCH)gcc
LD := $(TOOLCHAIN)$(ARCH)ld
OBJCOPY := $(TOOLCHAIN)$(ARCH)objcopy
CLANG_FORMAT ?= clang-format
EXTRA_CFLAGS ?= -Wstack-usage=1024
endif

ifeq ($(V),)
QUIET := @
else
ifeq ($(V),0)
QUIET := @
else
QUIET :=
endif
endif

BASE_CFLAGS := -O2 -Wall -g -Wundef -Werror=strict-prototypes -fno-common -fno-PIE \
	-Werror=implicit-function-declaration -Werror=implicit-int \
	-Wsign-compare -Wunused-parameter -Wno-multichar \
	-ffreestanding -fPIC -ffunction-sections -fdata-sections \
	-nostdinc -isystem $(shell $(CC) -print-file-name=include) -isystem sysinc \
	-fno-stack-protector -mstrict-align -mcpu=cortex-a5 -marm -mfloat-abi=soft \
	$(EXTRA_CFLAGS)

CFLAGS := $(BASE_CFLAGS)

CFG :=
ifeq ($(RELEASE),1)
CFG += RELEASE
endif

LDFLAGS := -EL -marmelf --no-undefined -X -Bsymbolic \
	-z notext --no-apply-dynamic-relocs --orphan-handling=warn \
	-z nocopyreloc --gc-sections -pie

ifeq ($(shell uname),Darwin)
	LDFLAGS += $(wildcard /Applications/ArmGNUToolchain/*/arm-none-eabi//lib/gcc/arm-none-eabi/*/libgcc.a)
else
	LDFLAGS += /usr/arm-none-eabi/lib/libgcc.a
endif

MINILZLIB_OBJECTS := $(patsubst %,minilzlib/%, \
        dictbuf.o inputbuf.o lzma2dec.o lzmadec.o rangedec.o xzstream.o)

TINF_OBJECTS := $(patsubst %,tinf/%, \
        adler32.o crc32.o tinfgzip.o tinflate.o tinfzlib.o)

DLMALLOC_OBJECTS := dlmalloc/malloc.o

LIBFDT_OBJECTS := $(patsubst %,libfdt/%, \
        fdt_addresses.o fdt_empty_tree.o fdt_ro.o fdt_rw.o fdt_strerror.o fdt_sw.o \
        fdt_wip.o fdt.o)

OBJECTS := \
	adt.o \
	clkrstgen.o \
	firmware.o \
	exception_asm.o \
	exception.o \
	heapblock.o \
	iodev.o \
	main.o \
	start.o \
	startup.o \
	string.o \
	uart.o \
	utils.o \
	vsprintf.o \
	chickens.o \
	smp.o \
	timer.o \
	proxy.o \
	kboot.o \
	memory.o \
	memory_asm.o \
	payload.o \
	ringbuffer.o \
	uartproxy.o \
	wdt.o \
	usb.o \
	usb_dwc2.o \
	$(LIBFDT_OBJECTS) \
	$(MINILZLIB_OBJECTS) \
	$(TINF_OBJECTS) \
	$(DLMALLOC_OBJECTS)

BUILD_OBJS := $(patsubst %,build/%,$(OBJECTS))
BUILD_ALL_OBJS := $(BUILD_OBJS)
NAME := mini
TARGET := mini.macho
TARGET_IMG3 := mini.img3

DEPDIR := build/.deps

.PHONY: all clean format invoke_cc always_rebuild
all: build/$(TARGET) build/$(TARGET_IMG3)
clean:
	rm -rf build/* build/.deps
format:
	$(CLANG_FORMAT) -i src/*.c src/*.h sysinc/*.h
format-check:
	$(CLANG_FORMAT) --dry-run --Werror src/*.c src/dcp/*.c src/math/*.c src/*.h src/dcp/*.h src/math/*.h sysinc/*.h
rustfmt:
	cd rust && cargo fmt
rustfmt-check:
	cd rust && cargo fmt --check

build/%.o: src/%.S
	$(QUIET)echo "  AS    $@"
	$(QUIET)mkdir -p $(DEPDIR)
	$(QUIET)mkdir -p "$(dir $@)"
	$(QUIET)$(AS) -c $(BASE_CFLAGS) -MMD -MF $(DEPDIR)/$(*F).d -MQ "$@" -MP -o $@ $<

build/%.o: src/%.c build-tag build-cfg
	$(QUIET)echo "  CC    $@"
	$(QUIET)mkdir -p $(DEPDIR)
	$(QUIET)mkdir -p "$(dir $@)"
	$(QUIET)$(CC) -c $(CFLAGS) -MMD -MF $(DEPDIR)/$(*F).d -MQ "$@" -MP -o $@ $<

# special target for usage by mini.loadobjs
invoke_cc:
	$(QUIET)$(CC) -c $(CFLAGS) -Isrc -o $(OBJFILE) $(CFILE)

build/$(NAME).elf: $(BUILD_ALL_OBJS) mini.ld
	$(QUIET)echo "  LD    $@"
	$(QUIET)$(LD) -T mini.ld $(LDFLAGS) -o $@ $(BUILD_ALL_OBJS)

build/$(NAME).macho: build/$(NAME).elf
	$(QUIET)echo "  MACHO $@"
	$(QUIET)$(OBJCOPY) -O binary --strip-debug $< $@

build/$(NAME).img3: build/$(NAME).macho
	$(QUIET)echo "  IMG3  $@"
	oldimgtool -m IMG3 -T krnl --comp $< $@

.INTERMEDIATE: build-tag build-cfg
build-tag src/../build/build_tag.h &:
	$(QUIET)mkdir -p build
	$(QUIET)./version.sh > build/build_tag.tmp
	$(QUIET)cmp -s build/build_tag.h build/build_tag.tmp 2>/dev/null || \
	( mv -f build/build_tag.tmp build/build_tag.h && echo "  TAG   build/build_tag.h" )

build-cfg src/../build/build_cfg.h &:
	$(QUIET)mkdir -p build
	$(QUIET)for i in $(CFG); do echo "#define $$i"; done > build/build_cfg.tmp
	$(QUIET)cmp -s build/build_cfg.h build/build_cfg.tmp 2>/dev/null || \
	( mv -f build/build_cfg.tmp build/build_cfg.h && echo "  CFG   build/build_cfg.h" )

build/%.bin: data/%.bin
	$(QUIET)echo "  IMG   $@"
	$(QUIET)mkdir -p "$(dir $@)"
	$(QUIET)cp $< $@

build/%.o: build/%.bin
	$(QUIET)echo "  BIN   $@"
	$(QUIET)mkdir -p "$(dir $@)"
	$(QUIET)$(OBJCOPY) -I binary -B aarch64 -O elf32-littleaarch32 $< $@

build/%.bin: font/%.bin
	$(QUIET)echo "  CP    $@"
	$(QUIET)mkdir -p "$(dir $@)"
	$(QUIET)cp $< $@

-include $(DEPDIR)/*
