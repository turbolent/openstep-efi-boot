/* Called in IA32 compatibility mode; paging remains on until final handoff.
 * Interrupts are enabled only inside the native firmware dispatcher. */
        .text
        .globl _bridge_call
_bridge_call:
        pushl %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        movl %esp, %edi
        leal 20(%esp), %ecx
        movl $bridge_resume32, %esi
        movl _bridge_code32, %ebx
        ljmpl *_bridge_gate
bridge_resume32:
        popl %edi
        popl %esi
        popl %ebx
        popl %ebp
        ret
