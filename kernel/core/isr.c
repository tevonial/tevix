#include <driver/vga.h>
#include <core/port.h>
#include <core/interrupt.h>

isr_t isr[256];

const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_install_handler(int isr_no, isr_t handler) {
    isr[isr_no] = handler;
}

void irq_install_handler(int irq_no, isr_t handler) {
    isr[irq_no + 32] = handler;
}

void isr_handler(registers_t regs) {
    // This line is important. When the processor extends the 8-bit interrupt number
    // to a 32bit value, it sign-extends, not zero extends. So if the most significant
    // bit (0x80) is set, regs.int_no will be very large (about 0xffffff80).
    uint8_t int_no = regs.int_no & 0xFF;

    if (isr[int_no]) {  // Call appropriate ISR
        isr[regs.int_no](&regs);
    } else {            // Default handler
        vga_puts("Unhandled interrupt: ");
        vga_put_hex(int_no);
        vga_puts(" [");
        vga_puts(exception_messages[int_no]);
        vga_puts("]\n");
        
        if (regs.err_code) {
            vga_puts("Error code: ");
            vga_put_hex(regs.err_code);
        }
        for(;;);
    }
}

// This gets called from our ASM interrupt handler stub.
void irq_handler(registers_t regs) {
    // Send an EOI (end of interrupt) signal to the PICs.

    

    if (regs.int_no >= 40)
        outportb(0xA0, 0x20);   // Send reset signal to slave.

    outportb(0x20, 0x20);       // Send reset signal to master.

    if (isr[regs.int_no])
        isr[regs.int_no](&regs);
}