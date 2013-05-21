.seg "INTERRUPT_TABLE", text
.global InterruptTable
	
InterruptTable:
	FLAG 0					      ; IRQ 0
	b	_start	
	jal	MEM_ERROR_ISR			      ; IRQ 1
	jal	BAD_INSTRUCTION_ISR		      ; IRQ 2
	jal	TIMER_ISR			      ; IRQ3
	jal	FLASH_ISR			      ; IRQ5

MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
TIMER_ISR:
FLASH_ISR:
	flag	1
	nop
	nop
	nop
