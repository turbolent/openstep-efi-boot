"""Run ABI compilation checks and the upstream EFI helper regression tests."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[3]
EFI = ROOT/'src/bootefi-1'
BUILD = ROOT/'build/tests'


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TMP=str(BUILD), TEMP=str(BUILD))
    startup = BUILD/'efi64_startup.exe'
    # The low image base exercises the real bridge-address check. Windows
    # AppContainer rejects this layout; run this suite outside that sandbox.
    for diagnostics in (0, 1):
        subprocess.run(['clang', '-DOPENSTEP_EFI64', '-DEFI64_HOST_TEST', f'-DOPENSTEP_DIAGNOSTICS={diagnostics}',
            '-fshort-wchar', '-Wall', '-Werror', '-I'+str(EFI),
            str(ROOT/'src/bootefi-1/tests/efi64_startup.c'), str(ROOT/'src/bootefi-1/efi64_main.c'),
            str(ROOT/'src/bootefi-1/firmware_console.c'), str(ROOT/'src/bootefi-1/efi_reservations.c'), str(ROOT/'src/bootefi-1/efi_map.c'), '-o', str(startup),
            '-Xlinker', '/base:0x00400000', '-Xlinker', '/dynamicbase:no'], check=True, env=env)
        for case in range(8):
            subprocess.run([str(startup), str(case)], check=True)
    subprocess.run(['clang', '-target', 'x86_64-unknown-windows', '-std=c11',
        '-ffreestanding', '-DOPENSTEP_EFI64', '-I'+str(EFI),
        '-c', str(ROOT/'src/bootefi-1/tests/efi64_abi.c'), '-o', str(BUILD/'efi64_abi.obj')],
        check=True, env=env)
    for arch in ('ia32', 'x64'):
        flags = ['-m32'] if arch == 'ia32' else ['-DOPENSTEP_EFI64']
        for suite, sources in [
            ('efi_map', [ROOT/'src/bootefi-1/efi_map.c']),
            ('efi_disk_alignment', [ROOT/'src/bootefi-1/efi_block_read.c',
                EFI/'efi_disk_select.c', EFI/'efi_sector.c']),
        ]:
            output = BUILD/(suite+'_'+arch+'.exe')
            subprocess.run(['clang', *flags, '-std=c11', '-Wall', '-Werror',
                '-I'+str(EFI), str(ROOT/'src/bootefi-1/tests'/(suite+'.c')),
                *map(str, sources), '-o', str(output)], check=True, env=env)
            subprocess.run([str(output)], check=True)
        output = BUILD/('efi_reservations_'+arch+'.exe')
        flags = ['-m32'] if arch == 'ia32' else ['-DOPENSTEP_EFI64']
        subprocess.run(['clang', *flags, '-fshort-wchar', '-Wall', '-Werror',
            '-I'+str(EFI), str(ROOT/'src/bootefi-1/tests/efi_reservations.c'),
            str(ROOT/'src/bootefi-1/efi_reservations.c'), str(ROOT/'src/bootefi-1/efi_map.c'), '-o', str(output)], check=True, env=env)
        subprocess.run([str(output)], check=True)
        output = BUILD/('firmware_graphics_'+arch+'.exe')
        flags = ['-m32'] if arch == 'ia32' else ['-DOPENSTEP_EFI64']
        subprocess.run(['clang', *flags, '-Wall', '-Werror', '-I'+str(EFI),
            '-I'+str(ROOT/'src/bootefi-1'), str(ROOT/'src/bootefi-1/tests/firmware_graphics.c'),
            str(ROOT/'src/bootefi-1/firmware_graphics.c'), '-o', str(output)], check=True, env=env)
        subprocess.run([str(output)], check=True)
    subprocess.run(['clang', '-target', 'i386-unknown-windows', '-std=c11',
        '-ffreestanding', '-Wno-gnu-folding-constant',
        '-I'+str(EFI),
        '-I'+str(ROOT/'src/boot-2/i386/libsa'), '-c',
        str(ROOT/'src/bootefi-1/tests/abi.c'), '-o', str(BUILD/'abi.obj')], check=True, env=env)
    for name, source in [
        ('efi_bootargs', 'efi_bootargs'), ('efi_disk_select', 'efi_disk_select'),
        ('efi_error', None), ('efi_sector', 'efi_sector'),
        ('efi_splash_rule', 'efi_splash_rule'), ('efi_pci_acpi', 'efi_pci_acpi'),
    ]:
        output = BUILD/(name+'.exe')
        command = ['clang', '-std=gnu89', '-Wall', '-Werror',
                   '-D_CRT_SECURE_NO_WARNINGS', '-I'+str(EFI),
                   str(EFI/'tests'/(name+'_test.c'))]
        if source:
            command.append(str(EFI/(source+'.c')))
        subprocess.run([*command, '-o', str(output)], check=True, env=env)
        subprocess.run([str(output)], check=True)
    output = BUILD/'pic.exe'
    subprocess.run(['clang', '-std=c99', '-Wall', '-Werror',
                    '-I'+str(ROOT/'src/bootefi-1'), str(ROOT/'src/bootefi-1/tests/pic.c'),
                    str(ROOT/'src/bootefi-1/openstep_pic.c'), '-o', str(output)],
                   check=True, env=env)
    subprocess.run([str(output)], check=True)
    output = BUILD/'gop.exe'
    subprocess.run(['clang', '-m32', '-std=c99', '-Wall', '-Werror',
                    '-Wno-gnu-folding-constant', '-I'+str(ROOT/'src/bootefi-1'),
                    '-I'+str(EFI), '-I'+str(ROOT/'src/boot-2/i386/libsa'),
                    str(ROOT/'src/bootefi-1/tests/gop.c'), str(ROOT/'src/bootefi-1/gop_record.c'),
                    '-o', str(output)], check=True, env=env)
    subprocess.run([str(output)], check=True)
    output = BUILD/'framebuffer_console.exe'
    subprocess.run(['clang', '-m32', '-std=c99', '-Wall', '-Werror',
                    '-Wno-gnu-folding-constant', '-I'+str(ROOT/'src/bootefi-1'),
                    '-I'+str(EFI), '-I'+str(ROOT/'src/boot-2/i386/libsa'),
                    str(ROOT/'src/bootefi-1/tests/framebuffer_console.c'),
                    str(ROOT/'src/bootefi-1/framebuffer_console.c'), str(ROOT/'src/bootefi-1/gop_record.c'),
                    '-o', str(output)], check=True, env=env)
    subprocess.run([str(output)], check=True)
    # make test compiles these with the loader's freestanding SDK flags.
    output = BUILD/'ufs_endian.exe'
    subprocess.run(['clang', '-m32', str(BUILD/'ufs_endian.obj'),
                    str(ROOT/'build/ia32/ufs_byteorder.obj'), '-o', str(output)],
                   check=True, env=env)
    subprocess.run([str(output)], check=True)
    print('EFI64 startup failures, OPENSTEP ABI, UFS layouts/endian conversion, PIC fix, GOP conversion, framebuffer bounds, '
          'and six upstream helper suites passed.')


if __name__ == '__main__':
    main()
