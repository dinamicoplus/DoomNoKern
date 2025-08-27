# Instrucciones

## Compilar
- Compilar con -O2 evita que el compilador meta traps
- Compilat con -g para depuracion
- Hay que activar el punto flotante con -m80387 y ademas con las siguientes instrucciones en boot.S:
```
    mov %cr4, %eax
    or  $(1<<9),  %eax      /* OSFXSR=1: habilita XMM/SSE */
    or  $(1<<10), %eax      /* OSXMMEXCPT=1: excepciones XMM por #XF */
    mov %eax, %cr4
```
- Cuidado con las mascaras de las interrupciones. Activo PIT y Keyboard

```
make
./create_disk.sh
qemu-system-i386 -cpu pentium3 -drive format=raw,file=disk.img -no-reboot -no-shutdown
```
Para hacer debugging con lldb
```
qemu-system-i386 -cpu pentium3 -drive format=raw,file=disk.img -no-reboot -no-shutdown -S -gdb tcp::1234,ipv4 -d int,guest_errors
lldb -o "settings set target.x86-disassembly-flavor intel" -o "gdb-remote 127.0.0.1:1234" --arch i386 -- kernel.elf
```