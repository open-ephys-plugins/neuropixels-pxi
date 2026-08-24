#!/usr/bin/env python3
"""
Checks that every probe part number listed in probe_features.json's
"neuropixels_probes" section is referenced somewhere in Probes/Geometry.cpp.

Usage:
    python check_probe_coverage.py

Prints any part numbers found in probe_features.json that are NOT
detected (as a whole-word, case-insensitive match) in Geometry.cpp.
"""

import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
JSON_PATH = SCRIPT_DIR / "probe_features.json"
CPP_PATH = SCRIPT_DIR / ".." / "Source" / "Probes" / "Geometry.cpp"


def load_part_numbers(json_path: Path) -> list[str]:
    with json_path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    try:
        probes = data["neuropixels_probes"]
    except KeyError:
        sys.exit(f'Error: "neuropixels_probes" key not found in {json_path}')

    return list(probes.keys())


def load_cpp_text(cpp_path: Path) -> str:
    return cpp_path.read_text(encoding="utf-8")


def main() -> None:
    if not JSON_PATH.exists():
        sys.exit(f"Error: {JSON_PATH} not found")
    if not CPP_PATH.exists():
        sys.exit(f"Error: {CPP_PATH} not found")

    part_numbers = load_part_numbers(JSON_PATH)
    cpp_text = load_cpp_text(CPP_PATH)

    missing = []
    for part_number in part_numbers:
        # Whole-word, case-insensitive search so e.g. "NP1000" doesn't
        # spuriously match as a substring of "NP10000".
        pattern = r"\b" + re.escape(part_number) + r"\b"
        if not re.search(pattern, cpp_text, re.IGNORECASE):
            missing.append(part_number)

    print(f"Checked {len(part_numbers)} part numbers from {JSON_PATH.name}")
    print(f"against {CPP_PATH}\n")

    if missing:
        print(f"Missing ({len(missing)}):")
        for part_number in missing:
            print(f"  {part_number}")
    else:
        print("All part numbers were found in Geometry.cpp.")


if __name__ == "__main__":
    main()
