#!/usr/bin/env python3
"""Download matching box art for the ROMs found on a PSP memory stick.

Images are fetched from the public Libretro thumbnail index and stored only on
the user's SD card. No ROMs, BIOS files or images are added to the repository.
"""

from __future__ import annotations

import argparse
from difflib import SequenceMatcher
import re
import struct
import sys
import zlib
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import quote, unquote
from urllib.request import Request, urlopen


BASE_URL = "https://thumbnails.libretro.com"
SYSTEMS = {
    "nes": ("Nintendo - Nintendo Entertainment System", "PSP/GAME/Nester_AoEX_R3/roms", (".nes",)),
    "gba": ("Nintendo - Game Boy Advance", "PSP/GAME/TempGBA4PSP-Mod/roms", (".gba",)),
    "gb": ("Nintendo - Game Boy", "PSP/GAME/TempGBA4PSP-Mod/roms", (".gb",)),
    "gbc": ("Nintendo - Game Boy Color", "PSP/GAME/TempGBA4PSP-Mod/roms", (".gbc",)),
}


def normalize(value: str) -> str:
    """Normalize names for matching filesystem ROM names to database names."""
    value = unquote(value).lower()
    return re.sub(r"[^a-z0-9]+", "", value)


def candidate_keys(title: str) -> list[str]:
    """Build conservative fallback keys for region-tagged ROM filenames."""
    candidates = [title]
    without_tags = re.sub(r"\([^)]*\)|\[[^]]*\]", "", title)
    candidates.append(without_tags)
    candidates.append(re.sub(r"^\s*\d+\s*-\s*", "", without_tags))
    return list(dict.fromkeys(normalize(candidate) for candidate in candidates if candidate.strip()))


def find_remote_name(index: dict[str, str], title: str) -> str | None:
    """Find an exact or very close thumbnail without guessing hack titles."""
    keys = candidate_keys(title)
    for key in keys:
        if key in index:
            return index[key]

    best_key = None
    best_score = 0.0
    for key in index:
        score = SequenceMatcher(None, keys[-1], key).ratio()
        if score > best_score:
            best_key = key
            best_score = score
    if best_key is not None and best_score >= 0.86:
        return index[best_key]
    return None


def list_boxarts(playlist: str) -> dict[str, str]:
    """Return normalized filename -> original filename from the remote index."""
    index_url = f"{BASE_URL}/{quote(playlist)}/Named_Boxarts/"
    request = Request(index_url, headers={"User-Agent": "PSP-GameShelf/1.01"})
    with urlopen(request, timeout=20) as response:
        html = response.read().decode("utf-8", errors="replace")
    names = re.findall(r'href="([^"]+\.png)"', html, flags=re.IGNORECASE)
    return {normalize(Path(unquote(name)).stem): unquote(Path(unquote(name)).name) for name in names}


def download_cover(playlist: str, remote_name: str, destination: Path) -> None:
    url = f"{BASE_URL}/{quote(playlist)}/Named_Boxarts/{quote(remote_name)}"
    request = Request(url, headers={"User-Agent": "PSP-GameShelf/1.01"})
    with urlopen(request, timeout=30) as response:
        data = response.read()
    destination.write_bytes(data)


def png_to_gsh(source: Path, destination: Path, width: int = 128, height: int = 160) -> None:
    """Convert a simple RGB/RGBA PNG to a PSP-friendly 16-bit texture.

    The PSP launcher deliberately reads this tiny raw format instead of
    decoding PNG at runtime.  This keeps the EBOOT small and avoids the
    instability that previously affected the launcher on real hardware.
    """
    data = source.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("formato PNG non riconosciuto")
    pos = 8
    idat = bytearray()
    image_width = image_height = color_type = bit_depth = None
    while pos + 12 <= len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            image_width, image_height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
            if bit_depth != 8 or interlace != 0 or color_type not in (2, 6):
                raise ValueError("PNG non supportato: usare RGB/RGBA non interlacciato")
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if image_width is None or image_height is None:
        raise ValueError("PNG senza IHDR")
    channels = 4 if color_type == 6 else 3
    stride = image_width * channels
    decoded = zlib.decompress(bytes(idat))
    rows: list[bytes] = []
    offset = 0
    previous = bytearray(stride)
    for _ in range(image_height):
        filter_type = decoded[offset]
        offset += 1
        current = bytearray(decoded[offset:offset + stride])
        offset += stride
        for index in range(stride):
            left = current[index - channels] if index >= channels else 0
            up = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 1:
                current[index] = (current[index] + left) & 0xFF
            elif filter_type == 2:
                current[index] = (current[index] + up) & 0xFF
            elif filter_type == 3:
                current[index] = (current[index] + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                estimate = left + up - upper_left
                distances = (abs(estimate - left), abs(estimate - up), abs(estimate - upper_left))
                predictor = (left, up, upper_left)[distances.index(min(distances))]
                current[index] = (current[index] + predictor) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"filtro PNG non supportato: {filter_type}")
        rows.append(bytes(current))
        previous = current

    output = bytearray(width * height * 2)
    for y in range(height):
        source_y = min(image_height - 1, (y * image_height) // height)
        row = rows[source_y]
        for x in range(width):
            source_x = min(image_width - 1, (x * image_width) // width)
            pixel = source_x * channels
            red, green, blue = row[pixel], row[pixel + 1], row[pixel + 2]
            value = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
            output[(y * width + x) * 2:(y * width + x) * 2 + 2] = struct.pack("<H", value)
    destination.write_bytes(output)


def scan_roms(sd_root: Path) -> list[tuple[str, str, Path]]:
    results: list[tuple[str, str, Path]] = []
    for platform, (_, relative_dir, extensions) in SYSTEMS.items():
        rom_dir = sd_root / relative_dir
        if not rom_dir.is_dir():
            continue
        for path in sorted(rom_dir.iterdir()):
            # Finder creates AppleDouble metadata files named ._* on FAT media.
            if path.name.startswith("."):
                continue
            if path.is_file() and path.suffix.lower() in extensions:
                results.append((platform, path.stem, path))
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sd-root", type=Path, default=Path("/Volumes/SD-FAT32"))
    parser.add_argument("--force", action="store_true", help="redownload existing covers")
    parser.add_argument("--platform", choices=SYSTEMS, action="append", help="limit to a platform (repeatable)")
    args = parser.parse_args()

    sd_root = args.sd_root.expanduser().resolve()
    if not sd_root.is_dir():
        print(f"Scheda SD non trovata: {sd_root}", file=sys.stderr)
        return 2

    covers_root = sd_root / "PSP/GAME/GAMESHELF/boxart"
    roms = scan_roms(sd_root)
    if args.platform:
        roms = [item for item in roms if item[0] in args.platform]
    if not roms:
        print("Nessuna ROM supportata trovata sulla scheda.")
        return 0

    indexes: dict[str, dict[str, str]] = {}
    downloaded = 0
    missing = 0
    for platform, title, rom_path in roms:
        playlist, _, _ = SYSTEMS[platform]
        destination = covers_root / platform / f"{title}.png"
        texture = covers_root / platform / f"{title}.gsh"
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.exists() and not args.force:
            if not texture.exists():
                try:
                    png_to_gsh(destination, texture)
                    print(f"MAKE  {texture.relative_to(sd_root)}")
                except (OSError, ValueError, zlib.error) as error:
                    print(f"ERR   texture {destination.name} ({error})", file=sys.stderr)
            print(f"SKIP  {platform}: {title}")
            continue
        try:
            if playlist not in indexes:
                print(f"Indice {playlist}...")
                indexes[playlist] = list_boxarts(playlist)
            remote_name = find_remote_name(indexes[playlist], title)
            if remote_name is None:
                print(f"MISS  {platform}: {rom_path.name}")
                missing += 1
                continue
            download_cover(playlist, remote_name, destination)
            png_to_gsh(destination, texture)
            print(f"OK    {destination.relative_to(sd_root)}")
            print(f"MAKE  {texture.relative_to(sd_root)}")
            downloaded += 1
        except (HTTPError, URLError, TimeoutError, OSError) as error:
            print(f"ERR   {platform}: {rom_path.name} ({error})", file=sys.stderr)

    print(f"Completato: {downloaded} scaricate, {missing} non trovate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
