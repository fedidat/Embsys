.seg "INTERRUPT_TABLE", text
.global InterruptTable
	
InterruptTable:
	FLAG 0					      ; IRQ 0
	b	_start	
	jal	MEM_ERROR_ISR			      ; IRQ 1
	jal	BAD_INSTRUCTION_ISR		      ; IRQ 2
	jal	timer_isr			      ; IRQ 3
	jal	FLASH_ISR			      ; IRQ 4

MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
FLASH_ISR:
	flag	1
	nop
	nop
	nop
