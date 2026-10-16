/* OPENSTEP 4.2 uses the 8259 PIC and does not initialize the local APIC. */
void openstep_prepare_interrupts(void)
{
    unsigned int a, b, c, d;
    unsigned int low, high;

    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                     : "a"(1), "c"(0));
    if (!(d & (1U << 9)))
        return;

    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1b));
    if (low & (1U << 11)) {
        /* Return interrupt delivery to the legacy PIC path instead of
         * inheriting the firmware's APIC configuration.
         * No EFI services may run after this.
         * Clear x2APIC mode too when leaving the enabled state. */
        low &= ~((1U << 11) | (1U << 10));
        __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0x1b)
                         : "memory");
    }
}
