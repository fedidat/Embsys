.seg "INTERRUPT_TABLE", text
.global InterruptTable
	
InterruptTable:
	FLAG 0					          ; IRQ 0
	b	_start	
	jal	MEM_ERROR_ISR			      ; IRQ 1
	jal	BAD_INSTRUCTION_ISR		      ; IRQ 2
	jal	<ISR function name goes here> ; IRQ 3
	jal	<ISR function name goes here> ; IRQ 4
	.
	.
	.
	.

MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
	flag	1
	nop
	nop
	nop