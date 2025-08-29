#!/usr/bin/env bash
set -euo pipefail

DISK_IMG="disk.img"

echo "→ Cargando disco $DISK_IMG..."
# Check if DISK_IMG exists
if [ ! -f "$DISK_IMG" ]; then
    echo "Error: $DISK_IMG no encontrado." >&2
    exit 1
fi

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
elif command -v parted >/dev/null 2>&1 && command -v mformat >/dev/null 2>&1 && command -v mcopy >/dev/null 2>&1; then
    echo "→ Usando mtools (Linux)..."
    PART_OFF=$((2048*512))

    mcopy -o -i "$DISK_IMG"@@$PART_OFF limine/limine-bios.sys ::/
    mcopy -o -i "$DISK_IMG"@@$PART_OFF limine.conf ::/
    mcopy -o -i "$DISK_IMG"@@$PART_OFF kernel/kernel.elf ::/

    ./limine/limine bios-install "$DISK_IMG"

    echo "ok"

else
    echo "Error: no se encontró ni hdiutil (macOS) ni parted+mtools (Linux)." >&2
    exit 1
fi