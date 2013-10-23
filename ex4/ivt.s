;this macro calls a given function to handle an interrupt
.macro call_user_isr, isr_func
	sub sp,sp,172
	st blink, [sp,16]
	bl _tx_thread_context_save
	sub sp,sp,16
	bl isr_func
	add sp,sp,16
	jal _tx_thread_context_restore
.endm
 
;define ARC Xtimer registers
	.extAuxRegister aux_timer, 0x21, r|w
	.extAuxRegister aux_tcontrol, 0x22, r|w
	.extAuxRegister aux_tlimit, 0x23, r|w
	.equ TIMER_INT_ENABLE, 0x3
	.equ TIMER_INT_VALUE, 50000

.seg "INTERRUPT_TABLE", text
.global InterruptTable
InterruptTable:
	FLAG 0							; IRQ 0
	b       _start
	jal 	MEM_ERROR_ISR			; IRQ 1
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 2
	jal 	_tx_timer_isr	        ; IRQ 3
	jal 	BAD_INSTRUCTION_ISR	 	; IRQ 4	
	jal 	flashISRWrapper			; IRQ 5
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 6	
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 7
	jal     BAD_INSTRUCTION_ISR 	; IRQ 8
	jal     buttonPressedISRWrapper	; IRQ 9
	jal     BAD_INSTRUCTION_ISR     ; IRQ 10
	jal     BAD_INSTRUCTION_ISR     ; IRQ 11
	jal     BAD_INSTRUCTION_ISR     ; IRQ 12
	jal     BAD_INSTRUCTION_ISR     ; IRQ 13
	jal     networkISRWrapper		; IRQ 14
	jal     displayISRWrapper		; IRQ 15
	  
_tx_timer_isr:
	sub sp, sp, 172							; Allocate an interrupt stack frame
	st r0, [sp, 0] 							; Save r0
	st r1, [sp, 4] 							; Save r1
	st r2, [sp, 8] 							; Save r2
	sr TIMER_INT_VALUE, [aux_tlimit] 		; Setup timer count
	sr TIMER_INT_ENABLE, [aux_tcontrol] 	; Enable timer interrupts

; End of target specific timer interrupt handling.
; Jump to generic ThreadX timer interrupt handler

	b _tx_timer_interrupt
	
buttonPressedISRWrapper:
	call_user_isr buttonPressedISR
	
networkISRWrapper:
	call_user_isr networkISR
	
displayISRWrapper:
	call_user_isr displayISR
	
flashISRWrapper:
    call_user_isr flashISR
	
MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
	flag 1
	nop
	nop
	nop
