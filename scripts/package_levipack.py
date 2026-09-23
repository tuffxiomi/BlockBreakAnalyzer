#!/usr/bin/env python3
import argparse
import json
import sys
import zipfile
from pathlib import Path

EXPECTED = {"manifest.json", "libBlockBreakAnalyzer.so", "icon.png", "resources/minecraft.ttf"}

def build_manifest(version="1.0.0"):
    return {
        "type": "preload-native",
        "name": "Block Break Analyzer",
        "author": "OpenAI",
        "description": "Shows the targeted block, required tool, and live break-time estimate.",
        "version": version,
        "entry": "libBlockBreakAnalyzer.so",
        "icon": "icon.png",
        "overwrite_files": ["icon.png", "resources/minecraft.ttf"],
        "overwrite_folders": [],
    }

def package(library: Path, icon: Path, font: Path, output: Path):
    for p in (library, icon, font):
        if not p.is_file():
            raise FileNotFoundError(p)
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest = build_manifest()
    if output.exists():
        output.unlink()
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")
        z.write(library, "libBlockBreakAnalyzer.so")
        z.write(icon, "icon.png")
        z.write(font, "resources/minecraft.ttf")
    with zipfile.ZipFile(output, "r") as z:
        names = set(z.namelist())
        if names != EXPECTED:
            raise RuntimeError(f"Package entries mismatch: {sorted(names)}")
        if json.loads(z.read("manifest.json")) != manifest:
            raise RuntimeError("Manifest verification failed")
        for path in ("libBlockBreakAnalyzer.so", "icon.png", "resources/minecraft.ttf"):
            if z.getinfo(path).file_size == 0:
                raise RuntimeError(f"Empty package entry: {path}")

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--library", type=Path, required=True)
    ap.add_argument("--icon", type=Path, required=True)
    ap.add_argument("--font", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    try:
        package(args.library.resolve(), args.icon.resolve(), args.font.resolve(), args.output.resolve())
    except Exception as exc:
        print(f"package failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
    print(args.output.resolve())
