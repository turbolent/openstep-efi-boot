# OPENSTEP EFI boot loader

Boots OPENSTEP 4.2 Patch 4 on IA32 and x64 EFI firmware.
Based on [raynorpat's RhapsodiOS EFI loader](https://github.com/RhapsodiOS/RhapsodiOS/tree/acc310afb8857e1807f0f9d0be9732d215dbac06/src/bootefi-1).

Tested with QEMU/OVMF and MacBook7,1,
including installation and booting the installed system.
The x64 loader hands off to the 32-bit kernel
and preserves the firmware framebuffer for OPENSTEP's VBE driver.

## Build

Requires GNU Make, a POSIX shell and utilities, Clang, LLD (`lld-link`),
and Python 3.10 or newer on `PATH`.
Host tests also require the C headers and runtime libraries used by Clang.

From a shell in this directory:

```sh
make -j8
make test
```

The build produces `build/BOOTIA32.EFI` and `build/BOOTX64.EFI`.
Use `make ia32` or `make x64` to build one architecture.
Use `make clean` to remove build products.

Build options:

- `BOOT_STRING="rootdev=hd0a -v"` — default kernel flags.
  Add `-s` to select the single-user shell.
  Choose the root device for your installation.
- `DIAGNOSTICS=1` — show loader diagnostics.
  Disabled by default; independent of the kernel's `-v` flag.
- `PAUSE=1` — wait for B or Escape before the x64 loader enters its 32-bit core.
  Disabled by default.

For example:

```sh
make x64 BOOT_STRING='rootdev=sd0a -v'
```

The loader appends the installation's `Kernel Flags` to the built-in boot string.
Changing build options automatically rebuilds affected files.

## Use

Use [QuickStep](https://github.com/turbolent/quickstep) to build a USB installer
and prepare the destination disk.
Supply a loader binary or the `build` directory containing both binaries.

For manual setup, place the appropriate binary at
`EFI/BOOT/BOOTX64.EFI` or `EFI/BOOT/BOOTIA32.EFI` on a FAT EFI System Partition.
The disk also needs an OPENSTEP installation
with its kernel, `sarld`, configuration, and drivers.
Supply installation media and driver binaries separately.

The loader limits kernel memory to 512 MiB.
EFI64 requires a usable GOP or Apple UGA framebuffer.
