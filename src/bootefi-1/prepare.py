"""Adapt legacy headers and embed/check PE images; compilation belongs to Make."""
from pathlib import Path
import re
import struct
import sys


def header(source, target):
    # NeXT's compiler accepted concatenated strings in #include; Clang does not.
    contents = source.read_text(encoding='utf-8')
    contents = re.sub(
        r'(?m)^(\s*#\s*(?:include|import))\s+ARCH_INCLUDE\(([^,]+),\s*([^)]+)\)',
        lambda m: f'{m[1]} <{m[2].strip()}i386/{m[3].strip()}>', contents)
    # The SDK's GCC 2.5 check otherwise discards Clang's packed attributes.
    if source.as_posix().endswith('/bsd/sys/cdefs.h'):
        contents = contents.replace(
            '#if !defined(__GNUC__) || __GNUC__ < 2 || __GNUC_MINOR__ < 5',
            '#if !defined(__GNUC__) || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(contents, encoding='utf-8', newline='\n')


def pe_image(source):
    data = source.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0':
        raise ValueError('not a PE image')
    return data, pe, pe+24


def require(condition, message):
    if not condition:
        raise ValueError(message)


def embed(source, target):
    data, pe, optional = pe_image(source)
    require(struct.unpack_from('<H', data, pe+4)[0] == 0x14c, 'core must be IA32')
    require(struct.unpack_from('<H', data, optional)[0] == 0x10b, 'core must be PE32')
    require(struct.unpack_from('<I', data, optional+28)[0] == 0x08000000,
            'unexpected core image base')
    size = struct.unpack_from('<I', data, optional+56)[0]
    entry = struct.unpack_from('<I', data, optional+16)[0]
    require(entry < size < 0x1000000, 'invalid core entry or image size')
    sections = optional + struct.unpack_from('<H', data, pe+20)[0]
    for i in range(struct.unpack_from('<H', data, pe+6)[0]):
        vs, va, raw, offset = struct.unpack_from('<IIII', data, sections+i*40+8)
        require(va + max(vs, raw) <= size and offset + raw <= len(data),
                'core section outside image')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text('const unsigned char core_image[] = {\n' +
        ',\n'.join(','.join(str(v) for v in data[i:i+32]) for i in range(0, len(data), 32)) +
        '\n};\n', encoding='utf-8', newline='\n')


def check(source):
    data, pe, optional = pe_image(source)
    require(struct.unpack_from('<H', data, pe+4)[0] == 0x8664 and
            struct.unpack_from('<H', data, optional)[0] == 0x20b, 'loader must be PE32+ x64')
    reloc_rva, reloc_size = struct.unpack_from('<II', data, optional+112+5*8)
    require(reloc_rva and reloc_size and not (struct.unpack_from('<H', data, pe+22)[0] & 1),
            'EFI64 loader must retain relocations')
    require(struct.unpack_from('<HH', data, optional+48) == (1, 0),
            'EFI64 loader must use subsystem version 1.0 for Apple EFI')


if __name__ == '__main__':
    try:
        action, *paths = sys.argv[1:]
        {'header': header, 'embed': embed, 'check': check}[action](*map(Path, paths))
    except (KeyError, TypeError, ValueError, OSError, struct.error) as error:
        sys.exit(f'prepare: {error}')
