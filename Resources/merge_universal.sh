#!/usr/bin/env bash
# merge_universal.sh <arm64_bundle> <x86_64_bundle> <out_dir>

set -euo pipefail

ARM_BUNDLE=$(realpath "${1:?arm64 bundle required}")
X86_BUNDLE=$(realpath "${2:?x86_64 bundle required}")
OUT_BUNDLE=${3:?Output bundle path required}

# if [[ -z "$(mdls -name kMDItemContentTypeTree "$ARM_BUNDLE" | grep 'com.apple.bundle')" ]]; then
#   echo "Error: '$ARM_BUNDLE' is not a valid app bundle." >&2
#   exit 1
# fi
# if [[ -z "$(mdls -name kMDItemContentTypeTree "$X86_BUNDLE" | grep 'com.apple.bundle')" ]]; then
#   echo "Error: '$X86_BUNDLE' is not a valid app bundle." >&2
#   exit 1
# fi

if [[ -e "$OUT_BUNDLE" ]]; then
  echo "Output directory '$OUT_BUNDLE' already exists, aborting." >&2
  exit 1
fi
mkdir -p $OUT_BUNDLE
cp -R -p "$ARM_BUNDLE"/* "$OUT_BUNDLE"
OUT_BUNDLE=$(realpath "$OUT_BUNDLE")

find "$ARM_BUNDLE" -type f \( -perm -u=x -o -perm -g=x -o -perm -o=x -o -name '*.dylib' -o -name '*.so' -o -name '*.bundle' \) |
while read -r arm_file; do
    rel=${arm_file#$ARM_BUNDLE/}
    # only care about Mach-O files
    file "$arm_file" | grep -q 'Mach-O' || continue
    # skip if already universal
    file "$arm_file" | grep -q 'universal binary' && continue

    out_file="$OUT_BUNDLE/$rel"
    x86_file="$X86_BUNDLE/$rel"

    if [[ -f "$x86_file" ]] && file "$x86_file" | grep -q 'Mach-O'; then
        lipo -create -output "$out_file" "$arm_file" "$x86_file"
        echo "✔ merged $rel"
    else
        echo "⚠ no x86_64 counterpart for $rel" >&2
    fi
done

