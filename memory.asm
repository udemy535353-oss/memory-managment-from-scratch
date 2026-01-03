[BITS 32]

%define MULTIBOOT_MAGIC    0x1BADB002
%define MULTIBOOT_FLAGS    0x00000003
%define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
    align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .data
gdt_start:
    dq 0x0000000000000000
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

idt_descriptor:
    dw idt_end - idt_start - 1
    dd idt_start

section .bss
align 16
stack_bottom:
    resb 65536
stack_top:
idt_start:
    resb 2048
idt_end:

section .text
global _start

_start:
    cli
    mov esp, stack_top

    lgdt [gdt_descriptor]
    jmp 0x08:.reload_segments

.reload_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call setup_idt
    call setup_paging
    
    mov edi, 0xB8000
    mov esi, boot_msg
    call kprint

    mov esi, status_msg
    call kprint

    sti

.idle:
    hlt
    jmp .idle

setup_idt:
    mov ecx, 256
    mov edi, idt_start
.idt_loop:
    mov eax, isr_stub
    mov [edi], ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 10001110b
    shr eax, 16
    mov [edi+6], ax
    add edi, 8
    loop .idt_loop
    lidt [idt_descriptor]
    ret

setup_paging:
    mov eax, page_directory
    mov ecx, 0
.page_dir_loop:
    mov edx, 0x00000002
    mov [eax + ecx*4], edx
    inc ecx
    cmp ecx, 1024
    jne .page_dir_loop

    mov eax, page_table
    mov ecx, 0
    mov edx, 0x00000003
.page_table_loop:
    mov [eax + ecx*4], edx
    add edx, 4096
    inc ecx
    cmp ecx, 1024
    jne .page_table_loop

    mov eax, page_directory
    mov edx, page_table
    or edx, 0x00000003
    mov [eax], edx

    mov eax, page_directory
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

kprint:
    mov ah, 0x0A
.print_loop:
    lodsb
    test al, al
    jz .done
    mov [edi], al
    mov [edi+1], ah
    add edi, 2
    jmp .print_loop
.done:
    ret

isr_stub:
    pushad
    ; Interrupt handling logic here
    popad
    iret

section .data
boot_msg db "SYSTEM: X86 PROTECTED MODE ACTIVE", 10, 0
status_msg db "SYSTEM: PAGING AND IDT INITIALIZED", 0

section .bss
align 4096
page_directory:
    resb 4096
page_table:
    resb 4096
