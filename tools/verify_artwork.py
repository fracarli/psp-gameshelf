#!/usr/bin/env python3
"""Regress indexed Atari cover decoding and tag-free artwork matching."""
from pathlib import Path
import struct
import tempfile
import zlib
from fetch_boxart import decode_cover_png,png_to_gsh,find_remote_name,normalize

def chunk(kind,data):
 return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
with tempfile.TemporaryDirectory() as d:
 root=Path(d);cover=root/'cover.png'
 raw=b''.join(b'\0'+bytes([1])*16 for _ in range(16))
 cover.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',16,16,8,3,0,0,0))+chunk(b'PLTE',bytes([255,0,0,0,255,0]))+chunk(b'tRNS',bytes([0,255]))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
 width,height,channels,rows=decode_cover_png(cover)
 assert (width,height,channels)==(16,16,4) and rows[0]==bytes([0,255,0,255])*16
 png_to_gsh(cover,root/'square.gsh',80,80,square_crop=True)
 assert (root/'square.gsh').read_bytes()==struct.pack('<H',0x07e0)*80*80
 index={normalize('Space Invaders (USA)'):'Space Invaders (USA).png',normalize('Space Invaders (Europe)'):'Space Invaders (Europe).png'}
 assert find_remote_name(index,'Space Invaders (1978) (Atari) [!]')=='Space Invaders (USA).png'
 assert find_remote_name(index,'Completely Different Hack') is None
print('PASS: indexed PNG palette/transparency, square texture and exact tag-free cover matching')
