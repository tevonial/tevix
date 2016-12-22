#include <core/gdt.h>

struct gdt_entry gdt[6];
struct tss_entry tss;
struct gdt_ptr gp;

void gdt_init() {
    /* Setup the GDT pointer and limit */
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = &gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment

    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment

    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    write_tss(5, 0x10, 0x0);

    /* Flush out the old GDT and install the new changes! */
    gdt_flush((uint32_t) &gp);
    tss_flush();
}

/* Setup a descriptor in the Global Descriptor Table */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    /* Setup the descriptor base address */
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

// Initialise our task state segment structure.
static void write_tss(int num, uint16_t ss0, uint32_t esp0) {
   // Firstly, let's compute the base and limit of our entry into the GDT.
   uint32_t base = (uint32_t) &tss;
   uint32_t limit = base + sizeof(tss);

   // Now, add our TSS descriptor's address to the GDT.
   gdt_set_gate(num, base, limit, 0xE9, 0x00);

   // Ensure the descriptor is initially zero.
   memset(&tss, 0, sizeof(tss));

   tss.ss0  = ss0;  // Set the kernel stack segment.
   tss.esp0 = esp0; // Set the kernel stack pointer.

   // Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
   // segments should be loaded when the processor switches to kernel mode. Therefore
   // they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
   // but with the last two bits set, making 0x0b and 0x13. The setting of these bits
   // sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
   // to switch to kernel mode from ring 3.
   tss.cs   = 0x0b;
   tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;
}
