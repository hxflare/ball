#!/bin/bash
DIR="${1:-.}"
shopt -s nullglob
files=("$DIR"/*.c)
if [ ${#files[@]} -eq 0 ]; then
    echo "No .c files found in $DIR"
    exit 1
fi
BTOOLS="$(dirname "$0")/../btools.c"
OUTDIR="$(dirname "$0")/../building/isoroot/initramfs/bin"
echo "Cleaning old binaries in $DIR..."
for file in "${files[@]}"; do
    name="${file%.c}"
    echo "Compiling $file -> $name"
    rm $name
    gcc -static -o "$name" "$file" "$BTOOLS" || { echo "Compilation failed: $file"; exit 1; }
done
echo "Wiping $OUTDIR."
rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"
echo "Copying to $OUTDIR."
find "$DIR" -maxdepth 1 -type f ! -name "*.c" -exec cp {} "$OUTDIR" \;
echo "Done."
