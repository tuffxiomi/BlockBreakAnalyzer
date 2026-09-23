#!/usr/bin/env python3
from pathlib import Path
import json
import sys

root = Path(sys.argv[1])
required = [
    root / "CMakeLists.txt",
    root / "levimod.json",
    root / "src/Api.cpp",
    root / "include/Analyzer.hpp",
    root / "include/Offsets.hpp",
    root / "include/Signatures.hpp",
    root / "resources/minecraft.ttf",
    root / "assets/icon.png",
]
missing = [str(p.relative_to(root)) for p in required if not p.is_file()]
assert not missing, f"missing files: {missing}"
manifest = json.loads((root / "levimod.json").read_text())
assert manifest["id"] == "blockbreakanalyzer"
assert manifest["info"]["name"] == "Block Break Analyzer"
for path in root.rglob("*"):
    if path.is_file() and path.name.lower().startswith("bedrock" + "tools"):
        raise AssertionError(f"forbidden reference-named file: {path}")
    if path.is_file() and path.suffix in {".cpp", ".hpp", ".json", ".md", ".sh", ".py"}:
        text = path.read_text(errors="ignore").lower()
        forbidden = "bedrock" + "tools"
        assert forbidden not in text, f"forbidden dependency text in {path}"
print("Project layout/dependency check passed.")
