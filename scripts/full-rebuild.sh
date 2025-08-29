#!/bin/bash
cd kernel
echo "→ Making kernel..."
make
cd ..
./scripts/load_disk.sh