#!/usr/bin/env python3
"""Inspect the built PE exports and SKSE runtime metadata."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
args = parser.parse_args()
data = args.dll.read_bytes()
u16 = lambda offset: struct.unpack_from('<H', data, offset)[0]
u32 = lambda offset: struct.unpack_from('<I', data, offset)[0]
assert data[:2] == b'MZ', 'Missing DOS header'
pe = u32(0x3c)
assert data[pe:pe+4] == b'PE\0\0', 'Missing PE header'
assert u16(pe+4) == 0x8664, 'Plugin must be x86-64'
optional = pe + 24
assert u16(optional) == 0x20b, 'Plugin must use PE32+'
sections = optional + u16(pe+20)

def offset(rva):
    for i in range(u16(pe+6)):
        section = sections + i * 40
        size, va, raw_size, raw = struct.unpack_from('<IIII', data, section+8)
        if va <= rva < va + max(size, raw_size):
            return raw + rva - va
    raise ValueError(f'Section mapping failed for RVA {rva:x}')

exports = offset(u32(optional+112))
count = u32(exports+24)
functions, names, ordinals = (offset(u32(exports+i)) for i in (28,32,36))
symbols = {}
for i in range(count):
    name_at = offset(u32(names+i*4))
    name = data[name_at:data.index(b'\0', name_at)].decode('ascii')
    symbols[name] = u32(functions+u16(ordinals+i*2)*4)
for name in ('SKSEPlugin_Load','SKSEPlugin_Query','SKSEPlugin_Version'):
    assert name in symbols, f'Missing required export: {name}'
version = offset(symbols['SKSEPlugin_Version'])
assert u32(version) == 1, 'Unexpected SKSE metadata layout'
flags_ex, flags = u32(version+772), u32(version+776)
assert flags & 1, 'Address Library independence is required'
assert flags_ex & 1, 'Runtime-aware layouts must be declared'
assert flags_ex & 2, 'Current Address Library metadata flag is required'
assert not any(u32(version+780+i*4) for i in range(16)), 'Runtime compatibility uses Address Library metadata'
print(json.dumps({'architecture':'x86-64','exports':sorted(symbols),'address_library':True,
                  'runtime_aware_layouts':True,'runtime_whitelist':False},indent=2))
