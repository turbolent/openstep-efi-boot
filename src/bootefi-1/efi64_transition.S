        .text
        .globl bridge_gate64
bridge_gate64:
        /* EDI=IA32 stack, ESI=return PC, EBX=compatibility selector,
         * ECX=pointer to fixed-width dispatcher arguments. */
        movl %edi, %r12d
        movl %esi, %r13d
        movl %ebx, %r14d
        movl %ecx, %ecx
        andq $-16, %rsp
        subq $32, %rsp
        call bridge_dispatch
        cli
        movq %r12, %rsp
        pushq %r14
        pushq %r13
        lretq

        .globl bridge_enter32
bridge_enter32:
        /* RCX=entry, RDX=low stack top, R8=bridge, R9=GDT descriptor. */
        cli
        lgdt (%r9)
        movzwl 8(%r8), %eax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %ss
        movq %rdx, %rsp
        subq $8, %rsp
        movl $0, (%rsp)
        movl %r8d, 4(%rsp)
        movzwl 6(%r8), %eax
        pushq %rax
        pushq %rcx
        lretq
