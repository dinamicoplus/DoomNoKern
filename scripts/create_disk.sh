#!/usr/bin/env bash
set -euo pipefail

DISK_IMG="ramdisk.img"

if command -v hdiutil >/dev/null 2>&1; then
    echo "→ Creando disco..."
    # Adjuntar imagen sin montar
    dd if=/dev/zero of="$DISK_IMG" bs=1m count=64
    DEV=$(hdiutil attach -nomount "$DISK_IMG" | awk 'NR==1{print $1}')
    echo "Dispositivo asignado: $DEV"
    sudo fdisk -e "$DEV" <<'EOF'
y
edit 1
4
n
2024
129048
write
flag 1
write
quit
EOF

    hdiutil detach "$DEV"
    DEV=$(hdiutil attach -nomount "$DISK_IMG" | awk 'NR==1{print $1}')

    sudo newfs_msdos -F 16 -v LIMINE "${DEV}s1"

    sudo mkdir -p /Volumes/LIMINE
    sudo mount -t msdos "${DEV}s1" /Volumes/LIMINE || true

    cp ./limine/limine-bios.sys /Volumes/LIMINE/
    cp ./limine.conf                 /Volumes/LIMINE/
    cp ./kernel/kernel.elf          /Volumes/LIMINE/

    sync
    sudo umount /Volumes/LIMINE || true
    hdiutil detach "$DEV" || true

    ./limine/limine bios-install "$DISK_IMG"

elif command -v parted >/dev/null 2>&1 && command -v mformat >/dev/null 2>&1 && command -v mcopy >/dev/null 2>&1; then
    echo "→ Usando parted + mtools (Linux)..."
    parted -s "$DISK_IMG" mklabel msdos
    parted -s "$DISK_IMG" mkpart primary fat16 1MiB 100%
    parted -s "$DISK_IMG" set 1 boot on

    # 1MiB = 1048576 bytes offset
    mformat -i "$DISK_IMG"@@1048576 -F ::

    mcopy -i "$DISK_IMG"@@1048576 limine/bin/limine-bios.sys ::/
    mcopy -i "$DISK_IMG"@@1048576 limine.conf ::/
    mcopy -i "$DISK_IMG"@@1048576 kernel/kernel.elf ::/

    ./limine/limine bios-install "$DISK_IMG"
    echo "ok"

else
    echo "Error: no se encontró ni hdiutil (macOS) ni parted+mtools (Linux)." >&2
    exit 1
fi
