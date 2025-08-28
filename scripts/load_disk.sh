#!/usr/bin/env bash
set -euo pipefail

DISK_IMG="disk.img"

if command -v hdiutil >/dev/null 2>&1; then
    echo "→ Usando hdiutil (macOS)..."
    # Adjuntar imagen sin montar
    DEV=$(hdiutil attach -nomount "$DISK_IMG" | awk 'NR==1{print $1}')
    echo "Dispositivo asignado: $DEV"

    sudo mkdir -p /Volumes/LIMINE
    sudo mount -t msdos "${DEV}s1" /Volumes/LIMINE || true

    cp ./limine/limine-bios.sys /Volumes/LIMINE/
    cp ./limine.conf                 /Volumes/LIMINE/
    cp ./kernel/kernel.elf          /Volumes/LIMINE/

    sync
    sudo umount /Volumes/LIMINE || true
    hdiutil detach "$DEV" || true
    echo "ok"
fi