typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139
#define RX_BUF_SIZE 8192

uint32_t io_base;
uint8_t rx_buffer[RX_BUF_SIZE + 1500];

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data) {
    uint32_t address = (uint32_t)((uint32_t)0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    asm volatile ("outl %0, $0xCF8" : : "a"(address));
    asm volatile ("outl %0, $0xCFC" : : "a"((uint32_t)data));
}

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    asm volatile ("outl %0, $0xCF8" : : "a"(address));
    uint32_t res;
    asm volatile ("inl $0xCFC, %0" : "=a"(res));
    return res;
}

void init_rtl8139(uint8_t bus, uint8_t slot, uint8_t func) {
    io_base = pci_read_config(bus, slot, func, 0x10) & (~0x3);
    
    pci_config_write_word(bus, slot, func, 0x04, 0x0005);

    outb(io_base + 0x52, 0x00);
    outb(io_base + 0x37, 0x10);
    while((inb(io_base + 0x37) & 0x10) != 0);

    uint32_t rb_ptr = (uint32_t)&rx_buffer;
    asm volatile ("outl %0, %1" : : "a"(rb_ptr), "Nd"((uint16_t)(io_base + 0x30)));

    asm volatile ("outl %0, %1" : : "a"(0x0005), "Nd"((uint16_t)(io_base + 0x3C)));
    outb(io_base + 0x37, 0x0C);
    
    asm volatile ("outl %0, %1" : : "a"(0x0000000F), "Nd"((uint16_t)(io_base + 0x44)));
}

void ethernet_handler() {
    uint16_t status = inb(io_base + 0x3E);
    if(status & 0x01) {
        // Packet Received - Kernel Network Stack Trigger
    }
    asm volatile ("outw %0, %1" : : "a"(0x05), "Nd"((uint16_t)(io_base + 0x3E)));
}

void kmain() {
    for(uint8_t bus = 0; bus < 256; bus++) {
        for(uint8_t slot = 0; slot < 32; slot++) {
            uint32_t id = pci_read_config(bus, slot, 0, 0);
            if((id & 0xFFFF) == RTL8139_VENDOR_ID) {
                init_rtl8139(bus, slot, 0);
                break;
            }
        }
    }

    while(1) {
        asm volatile("hlt");
    }
}
