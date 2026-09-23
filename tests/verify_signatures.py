#!/usr/bin/env python3
import re
import sys
from pathlib import Path

header = (Path(__file__).resolve().parents[1] / "include" / "Signatures.hpp").read_text()
binary = Path(sys.argv[1]).read_bytes()

pairs = re.findall(r'inline constexpr std::string_view (\w+) = "([^"]+)";', header)
for name, pattern_text in pairs:
    if name == "module":
        continue
    toks = pattern_text.split()
    # Anchor on the longest contiguous non-wildcard byte run, then verify the full mask.
    best_start = best_bytes = None
    cur_start = None
    cur = bytearray()
    runs = []
    for idx, token in enumerate(toks + ["?"]):
        if token != "?":
            if cur_start is None:
                cur_start = idx
            cur.append(int(token, 16))
        elif cur_start is not None:
            runs.append((len(cur), cur_start, bytes(cur)))
            cur_start = None
            cur = bytearray()
    if not runs:
        raise SystemExit(f"{name}: no fixed bytes")
    _, anchor_start, anchor = max(runs, key=lambda r: r[0])
    pat = [None if t == "?" else int(t, 16) for t in toks]
    hits = []
    search_from = 0
    while True:
        anchor_at = binary.find(anchor, search_from)
        if anchor_at < 0:
            break
        base = anchor_at - anchor_start
        if base >= 0 and base + len(pat) <= len(binary):
            chunk = binary[base:base+len(pat)]
            if all(p is None or c == p for c, p in zip(chunk, pat)):
                hits.append(base)
                if len(hits) > 2:
                    break
        search_from = anchor_at + 1
    status = "OK" if len(hits) == 1 else "BAD"
    print(f"{name}: {status} ({len(hits)} match{'es' if len(hits) != 1 else ''})" + (f" @ 0x{hits[0]:X}" if len(hits) == 1 else ""))
    if len(hits) != 1:
        raise SystemExit(1)
print("All required signatures resolve uniquely in the supplied Minecraft binary.")
