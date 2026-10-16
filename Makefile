# GNU Make with a POSIX shell (MSYS2 on Windows), Clang, LLD, and Python 3.
# make [ia32|x64|all|test] BOOT_STRING='rootdev=hd0a -v' DIAGNOSTICS=1 PAUSE=1
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
SHELL := /bin/sh
CC = clang
LD = lld-link
PYTHON ?= python
BOOT_STRING ?= rootdev=hd0a -v -s
DIAGNOSTICS ?= 0
PAUSE ?= 0

# Keep MSYS2 from treating LLD's /option arguments as filesystem paths.
export MSYS2_ARG_CONV_EXCL := *

EFI := src/bootefi-1
BOOT := src/boot-2/i386
SDK := build/openstep-include
PREPARE := $(EFI)/prepare.py
quote = '$(subst ','"'"',$(1))'

IA32_FLAGS = -target i386-unknown-windows -std=gnu89 -ffreestanding \
    -fno-stack-protector -fshort-wchar -Wall -Os -fno-ms-extensions \
    -Wno-implicit-int -Wno-implicit-function-declaration \
    -Wno-error=int-conversion -Wno-error=incompatible-pointer-types \
    -Wno-error=incompatible-function-pointer-types \
    '-D__ARCHITECTURE__="i386"' -D_NEXT_SOURCE -DNeXT \
    -D__LITTLE_ENDIAN__=1 -D__BIG_ENDIAN__=0 -DDONT_USE_ASM -nostdlibinc \
    $(call quote,-DBOOTEFI_BOOT_STRING="$(BOOT_STRING)") \
    -include $(EFI)/bootefi_memory_override.h \
    -I$(EFI) -I$(BOOT)/libsaio -I$(BOOT)/libsa -I$(BOOT)/util \
    -I$(SDK) -I$(SDK)/bsd -I$(SDK)/ansi
CORE_FLAGS = $(IA32_FLAGS) -DOPENSTEP_EFI64_CORE -mno-sse -mno-mmx
X64_FLAGS = -target x86_64-unknown-windows -ffreestanding \
    -fno-stack-protector -fshort-wchar -mno-red-zone -mno-stack-arg-probe \
    -mgeneral-regs-only -Wall -Werror -Os -DOPENSTEP_EFI64 -I$(EFI)
ifeq ($(DIAGNOSTICS),1)
IA32_FLAGS += -DOPENSTEP_DIAGNOSTICS=1
X64_FLAGS += -DOPENSTEP_DIAGNOSTICS=1
endif
ifeq ($(PAUSE),1)
X64_FLAGS += -DOPENSTEP_PAUSE_BEFORE_HANDOFF
endif

IA32_LINK = /machine:x86 /subsystem:efi_application /nodefaultlib \
    /base:0x08000000 /fixed /safeseh:no /timestamp:0
X64_LINK = /machine:x64 /subsystem:efi_application,1.0 /nodefaultlib \
    /entry:efi_main /base:0x09000000 /fixed:no /include:efi64_entry /timestamp:0

COMMON_NAMES := efi_main efi_console efi_disk efi_disk_select efi_bootargs \
    efi_memmap efi_memory efi_vga efi_gfx efi_splash efi_splash_rule efi_pci \
    efi_sector efi_acpi handoff \
    disk ufs_byteorder cache load table drivers stringTable choose localize \
    sys bitmap unpackbits font \
    sprintf prf string1 memcpy getsegbyname bsearch qsort strtol bitmaps \
    handoff_tramp openstep_handoff efi_map efi_block_read openstep_pic \
    openstep_vbe_pitch openstep_reboot efi_reboot openstep_gop gop_record \
    framebuffer_console
IA32_OBJECTS := $(addprefix build/ia32/,$(addsuffix .obj,$(COMMON_NAMES) firmware_graphics efi_reservations))
CORE_OBJECTS := $(addprefix build/core32/,$(addsuffix .obj,$(COMMON_NAMES) efi64_core efi64_call_tramp))
X64_OBJECTS := $(addprefix build/x64/,$(addsuffix .obj,efi64_main efi64_transition firmware_graphics firmware_console efi_reservations efi_map core_image))
UFS_OBJECTS := build/tests/ufs_abi.obj build/tests/ufs_endian.obj
OBJECTS := $(IA32_OBJECTS) $(CORE_OBJECTS) $(X64_OBJECTS) $(UFS_OBJECTS)

vpath %.c $(EFI) $(BOOT)/libsaio $(BOOT)/libsa $(BOOT)/boot2
vpath %.S $(EFI)

# Native Windows Clang writes backslashes into depfiles. Normalize path
# separators so generated SDK headers share the same Make dependency nodes.
define compile
$(CC) $(CPPFLAGS) $(1) $(CFLAGS) -MMD -MP -c $< -o $@
@sed -i 's|\\\([^[:space:]]\)|/\1|g' $(@:.obj=.d)
endef

.PHONY: all ia32 x64 test clean FORCE
all: ia32 x64
ia32: build/BOOTIA32.EFI
x64: build/BOOTX64.EFI

# Adapt SDK headers individually; originals under include/ stay untouched.
$(SDK)/%.h: include/%.h $(PREPARE)
	$(PYTHON) $(PREPARE) header $< $@

SDK_HEADERS := $(shell find include -type f -name '*.h')
STAGED_HEADERS := $(patsubst include/%,$(SDK)/%,$(SDK_HEADERS))
$(IA32_OBJECTS) $(CORE_OBJECTS) $(UFS_OBJECTS): | $(STAGED_HEADERS)

# Save command options only when they change, including on a return to defaults.
# Header dependencies come from Clang; option changes invalidate their objects.
build/ia32/.flags: FLAGS = $(IA32_FLAGS)
build/core32/.flags: FLAGS = $(CORE_FLAGS)
build/x64/.flags: FLAGS = $(X64_FLAGS)
build/ia32/.flags build/core32/.flags build/x64/.flags: FORCE
	@mkdir -p $(@D)
	@printf '%s\n' $(call quote,$(CC) $(CPPFLAGS) $(FLAGS) $(CFLAGS) $(LD) $(LDFLAGS)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(IA32_OBJECTS) $(UFS_OBJECTS): build/ia32/.flags
$(CORE_OBJECTS): build/core32/.flags
$(X64_OBJECTS): build/x64/.flags

build/ia32/%.obj: %.c Makefile
	$(call compile,$(IA32_FLAGS))
build/ia32/%_tramp.obj: %.S Makefile
	$(call compile,$(IA32_FLAGS))
build/core32/%.obj: %.c Makefile
	$(call compile,$(CORE_FLAGS))
build/core32/%_tramp.obj: %.S Makefile
	$(call compile,$(CORE_FLAGS))
build/x64/%.obj: %.c Makefile
	$(call compile,$(X64_FLAGS))
build/x64/%.obj: %.S Makefile
	$(call compile,$(X64_FLAGS))

build/BOOTIA32.EFI: $(IA32_OBJECTS)
	$(LD) $(IA32_LINK) $(LDFLAGS) /entry:efi_main /map:build/ia32/loader.map /out:$@ $(IA32_OBJECTS)
build/core32/core32.exe: $(CORE_OBJECTS)
	$(LD) $(IA32_LINK) $(LDFLAGS) /entry:core_main /map:build/core32/loader.map /out:$@ $(CORE_OBJECTS)
build/x64/core_image.c: build/core32/core32.exe $(PREPARE)
	$(PYTHON) $(PREPARE) embed $< $@
build/x64/core_image.obj: build/x64/core_image.c Makefile
	$(call compile,$(X64_FLAGS))
build/BOOTX64.EFI: $(X64_OBJECTS) $(PREPARE)
	$(LD) $(X64_LINK) $(LDFLAGS) /map:build/x64/loader.map /out:$@ $(X64_OBJECTS)
	$(PYTHON) $(PREPARE) check $@

build/tests/%.obj: $(EFI)/tests/%.c Makefile
	@mkdir -p $(@D)
	$(call compile,$(IA32_FLAGS))
test: all $(UFS_OBJECTS)
	$(PYTHON) $(EFI)/tests/run.py

# Delete only loader build products, preserving disk images and other build/ data.
clean:
	rm -rf build/ia32 build/core32 build/x64 build/openstep-include build/tests
	rm -f build/BOOTIA32.EFI build/BOOTX64.EFI

-include $(OBJECTS:.obj=.d)
