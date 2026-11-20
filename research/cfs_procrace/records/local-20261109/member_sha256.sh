#!/bin/sh
# Called by GNU tar --to-command once per regular member: hash the member's bytes from stdin.
# Appends "sha256<TAB>size<TAB>path" to $HASHOUT (size = TAR_SIZE from the tar header; the hash covers the bytes themselves).
h=$(sha256sum | cut -d' ' -f1)
printf '%s\t%s\t%s\n' "$h" "$TAR_SIZE" "$TAR_FILENAME" >> "$HASHOUT"
