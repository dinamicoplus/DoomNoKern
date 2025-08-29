# Instrucciones

## Compilar

```bash
git submodule update --init --recursive
```
sincronizar submodulos
```bash
git submodule sync --recursive
```

### Newlib
```bash
sudo apt update
sudo apt install clang lld parted -q -y
./scripts/compile-newlib.sh
```

### Limine
```bash
sudo apt install nasm mtools parted -q -y
cd limine
make
cd ..
./scripts/create_disk.sh
```

### Kernel
- Compilar con -O2 evita que el compilador meta traps
- Compilat con -g para depuracion
- Hay que activar el punto flotante con -m80387 y ademas con las siguientes instrucciones en boot.S:
```asm
    mov %cr4, %eax
    or  $(1<<9),  %eax      /* OSFXSR=1: habilita XMM/SSE */
    or  $(1<<10), %eax      /* OSXMMEXCPT=1: excepciones XMM por #XF */
    mov %eax, %cr4
```
- Cuidado con las mascaras de las interrupciones. Activo PIT y Keyboard

```bash
cd kernel
make
cd ..
./scripts/load_disk.sh
qemu-system-i386 -cpu pentium3 -drive format=raw,file=disk.img -no-reboot -no-shutdown
```
Para hacer debugging con lldb
```bash
qemu-system-i386 -cpu pentium3 -drive format=raw,file=disk.img -no-reboot -no-shutdown -S -gdb tcp::1234,ipv4 -d int,guest_errors
lldb -o "settings set target.x86-disassembly-flavor intel" -o "gdb-remote 127.0.0.1:1234" --arch i386 -- kernel/kernel.elf
```

## VBox
Crear una imagen vdi
```bash
VBoxManage convertfromraw disk.img disk.vdi --format VDI --variant Standard
```

Crear una VM
```bash
VBoxManage createvm \
    --name "kernel-test" \
    --ostype "Other" \  
    --register
VBoxManage modifyvm "kernel-test" \     
    --memory 64 \       
    --vram 9 \ 
    --acpi on \         
    --ioapic off \
    --chipset piix3 \
    --boot1 disk \
    --boot2 none \
    --boot3 none \
    --boot4 none \
    --nictype1 Am79C973 \
    --nic1 nat \
    --graphicscontroller vboxvga
VBoxManage storagectl "kernel-test" \                 
    --name "IDE" \      
    --add ide \
    --controller PIIX4 \
    --bootable on
VBoxManage storageattach "kernel-test" \              
    --storagectl "IDE" \
    --port 0 \ 
    --device 0 \        
    --type hdd \ 
    --medium $PWD/disk.vdi
VBoxManage startvm "kernel-test" --type gui
```

No funciona en MacOSX M1 (ARM)

## UTM

```bash
qemu-img convert -O qcow2 disk.vdi disk.qcow2
```

# FAT12 disk creation for doom1.wad

```bash
dd if=/dev/zero of=fat12.img bs=1024 count=$((8*1024))
mkfs.fat -F 12 -C fat12.img $((8*1024))
mcopy -i fat12.img doom1.wad ::/
xxd -i fat12.img kernel/include/disk/fat12_img.h
```