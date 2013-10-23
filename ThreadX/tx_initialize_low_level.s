;/**************************************************************************/ 
;/*                                                                        */ 
;/*            Copyright (c) 1996-2006 by Express Logic Inc.               */
;/*                                                                        */ 
;/*  This software is copyrighted by and is the sole property of Express   */ 
;/*  Logic, Inc.  All rights, title, ownership, or other interests         */ 
;/*  in the software remain the property of Express Logic, Inc.  This      */ 
;/*  software may only be used in accordance with the corresponding        */ 
;/*  license agreement.  Any unauthorized use, duplication, transmission,  */ 
;/*  distribution, or disclosure of this software is expressly forbidden.  */ 
;/*                                                                        */
;/*  This Copyright notice may not be removed or modified without prior    */ 
;/*  written consent of Express Logic, Inc.                                */ 
;/*                                                                        */ 
;/*  Express Logic, Inc. reserves the right to modify this software        */ 
;/*  without notice.                                                       */ 
;/*                                                                        */ 
;/*  Express Logic, Inc.                     info@expresslogic.com         */
;/*  11423 West Bernardo Court               http://www.expresslogic.com   */
;/*  San Diego, CA  92127                                                  */
;/*                                                                        */
;/**************************************************************************/
;
;
;/**************************************************************************/
;/**************************************************************************/
;/**                                                                       */ 
;/** ThreadX Component                                                     */ 
;/**                                                                       */
;/**   Initialize                                                          */
;/**                                                                       */
;/**************************************************************************/
;/**************************************************************************/
;
;
;/* Define the ARC Xtimer registers.  */ 
;  
    .extAuxRegister aux_timer,      0x21,   r|w  
    .extAuxRegister aux_tcontrol,   0x22,   r|w   
    .extAuxRegister aux_tlimit,     0x23,   r|w

;
;/* Define several equates used in initialization.  */
;
    .equ    RESET_VECTOR_ADDR,      0
    .equ    VECTOR_TABLE_BASE_ADDR, 0x00019000
    .equ    ISERVICE3_VECTOR_ADDR,  0x18 
    .equ    ISERVICE4_VECTOR_ADDR,  0x20 
    .equ    ISERVICE5_VECTOR_ADDR,  0x28 
    .equ    TIMER_INT_ENABLE,       0x3 
    .extern TIMER_INT_VALUE     
    .extern TIMER_WRAP 
    .equ    LONG_ALIGN_MASK,        0xFFFFFFFC

    ; Define this so that this module doesn't need to be specially built 
    ; to support profiling.  It causes negligible overhead here. 
    .define profile

;
;/**************************************************************************/ 
;/*                                                                        */ 
;/*  FUNCTION                                               RELEASE        */ 
;/*                                                                        */ 
;/*    _tx_initialize_low_level                         ARC600/MetaWare    */
;/*                                                           5.0          */
;/*  AUTHOR                                                                */ 
;/*                                                                        */ 
;/*    William E. Lamie, Express Logic, Inc.                               */ 
;/*                                                                        */ 
;/*  DESCRIPTION                                                           */ 
;/*                                                                        */ 
;/*    This function is responsible for any low-level processor            */ 
;/*    initialization, including setting up interrupt vectors, setting     */ 
;/*    up a periodic timer interrupt source, saving the system stack       */ 
;/*    pointer for use in ISR processing later, and finding the first      */ 
;/*    available RAM memory address for tx_application_define.             */ 
;/*                                                                        */ 
;/*  INPUT                                                                 */ 
;/*                                                                        */ 
;/*    None                                                                */ 
;/*                                                                        */ 
;/*  OUTPUT                                                                */ 
;/*                                                                        */ 
;/*    None                                                                */ 
;/*                                                                        */ 
;/*  CALLS                                                                 */ 
;/*                                                                        */ 
;/*    None                                                                */ 
;/*                                                                        */ 
;/*  CALLED BY                                                             */ 
;/*                                                                        */ 
;/*    _tx_initialize_kernel_enter           ThreadX entry function        */ 
;/*                                                                        */ 
;/*  RELEASE HISTORY                                                       */ 
;/*                                                                        */ 
;/*    DATE              NAME                      DESCRIPTION             */ 
;/*                                                                        */ 
;/*  12-12-2005     William E. Lamie         Initial version 5.0           */
;/*                                                                        */ 
;/**************************************************************************/ 
;VOID   _tx_initialize_low_level(VOID)
;{
    .global _tx_initialize_low_level
    .type   _tx_initialize_low_level, @function 
_tx_initialize_low_level:

;
;    /* Save the system stack pointer.  */
;    _tx_thread_system_stack_ptr = (VOID_PTR) (sp);
;
    st      sp, [gp, _tx_thread_system_stack_ptr@sda]   ; Save system stack pointer
;
;
;    /* Pickup the first available memory address.  */
;
    mov     r0, _end                                    ; Pickup first free memory address
    add     r0, r0, 4                                   ; Add an additional amount
    and     r0, r0, LONG_ALIGN_MASK                     ; Insure proper alignment
;
;    /* Save the first available memory address.  */
;    _tx_initialize_unused_memory =  (VOID_PTR) _end;
;
    st      r0, [gp, _tx_initialize_unused_memory@sda]
;
;
;    /* Set interrupt vector base address.  */
;
    mov      r0, VECTOR_TABLE_BASE_ADDR
    sr       r0, [int_vector_base]
;
;    /* Construct vector table.  */
;
    mov     r0, VECTOR_TABLE_BASE_ADDR-4                ; Point to exception vectors
    mov     r1, _tx_vector_table-4                      ; Point to undefined exception vectors table
;
_tx_next_vector:
    ld.a    r2, [r1, 4]                                 ; Pickup first word of new vector
    sub.f   0, r2, 0                                    ; Set the flags
    ld.a    r3, [r1, 4]                                 ; Pickup final word of new vector
    beq     _tx_complete_vector_table                   ; No more vectors
    st.a    r2, [r0, 4]                                 ; Overwrite first word of actual vector
    b.d     _tx_next_vector                             ; Get next vector
    st.a    r3, [r0, 4]                                 ; Overwrite final word of actual vector
_tx_complete_vector_table:
;
;    /* Setup Timer for periodic interrupts at interrupt vector 3.  The vector is actually
;       setup for ThreadX down below.  */
; 
    .ifdef profile 
    ld      r0, [_mwprof_countdown_ticks]               ; Compute value for loading timer 
    sr      r0, [aux_tlimit]                            ; Setup timer count limit
    .else 
    sr TIMER_INT_VALUE, [aux_tlimit]                    ; Setup timer count limit
    .endif 
    sr 0, [aux_timer]                                   ; Clear the timer count register
    sr TIMER_INT_ENABLE, [aux_tcontrol]                 ; Enable timer interrupts
;
;    /* Done, return to caller.  */
;
    jal.d   [blink]                                     ; Return to caller
    nop
;}
;
; 
;    /* Define the undefined exception vectors. These are copied to the actual exception vector
;       area during initialization (above).  */
_tx_vector_table:
    j   _tx_vector_reset
    j   _tx_vector_memory_exception
    j   _tx_vector_instruction_error
    j   _tx_vector_interrupt_3
    j   _tx_vector_interrupt_4
    j   _tx_vector_interrupt_5
    j   _tx_vector_interrupt_6
    j   _tx_vector_interrupt_7
    .word   0,0                                         ; 0 ends vectors table
;
;
;    /* Define the reset vector.  */
;
    .global _tx_vector_reset
    .type   _tx_vector_reset, @function
_tx_vector_reset:
    flag    1
    nop
    nop
    nop
;
;    /* Define the memory exception vector.  */
;
    .global _tx_vector_memory_exception
    .type   _tx_vector_memory_exception, @function
_tx_vector_memory_exception:
    flag    1
    nop
    nop
    nop
;
;    /* Define the instruction error exception vector.  */
;
    .global _tx_vector_instruction_error
    .type   _tx_vector_instruction_error, @function
_tx_vector_instruction_error:
    flag    1
    nop
    nop
    nop
;
;    /* Define the interrupt 3 execution vector.  */
;
    .global _tx_vector_interrupt_3
    .type   _tx_vector_interrupt_3, @function
_tx_vector_interrupt_3:
;
;    /* By default, setup interrupt 3 as the auxiliary timer interrupt.  */
;
    sub     sp, sp, 172                                 ; Allocate an interrupt stack frame
    st      r0, [sp, 0]                                 ; Save r0
    st      r1, [sp, 4]                                 ; Save r1
    st      r2, [sp, 8]                                 ; Save r2   
    .ifdef profile
    ld      r0, [_mwprof_countdown_ticks]               ; Compute value for reloading timer
    sr      r0, [aux_tlimit]                            ; Setup timer count
    .else
    sr      TIMER_INT_VALUE, [aux_tlimit]               ; Setup timer count
    .endif
    sr      TIMER_INT_ENABLE, [aux_tcontrol]            ; Enable timer interrupts
                                                        ; End of target specific timer interrupt 
                                                        ;   handling.
    b       _tx_timer_interrupt                         ; Jump to generic ThreadX timer interrupt
                                                        ;   handler
;
;    /* End of ThreadX default timer setup.  */
;
;    /* Original vector interrupt 3 code - commented out for the demo.  */
;    flag    1
;    nop
;    nop
;    nop
;
;    /* Define the interrupt 4 execution vector.  */
;
    .global _tx_vector_interrupt_4
    .type   _tx_vector_interrupt_4, @function
_tx_vector_interrupt_4:
    flag    1
    nop
    nop
    nop
;
;    /* Define the interrupt 5 execution vector.  */ 
; 
    .global _tx_vector_interrupt_5 
    .type   _tx_vector_interrupt_5, @function 
_tx_vector_interrupt_5: 
    flag    1 
    nop 
    nop 
    nop
;
;    /* Define the interrupt 6 execution vector.  */ 
; 
    .global _tx_vector_interrupt_6 
    .type   _tx_vector_interrupt_6, @function 
_tx_vector_interrupt_6: 
    flag    1 
    nop 
    nop 
    nop
;
;    /* Define the interrupt 7 execution vector.  */ 
; 
    .global _tx_vector_interrupt_7 
    .type   _tx_vector_interrupt_7, @function 
_tx_vector_interrupt_7: 
    flag    1 
    nop 
    nop 
    nop
    .end
