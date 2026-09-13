/*
 * Aurum - task context switching
 *
 * ATmega328P / AVR
 */

.equ AU_SREG, 0x3F
.equ AU_SPL,  0x3D
.equ AU_SPH,  0x3E

.global au_context_switch
.type   au_context_switch, @function

au_context_switch:
    ; r25:r24 = saved_sp, r23:r22 = next_sp
    ; PRECONDITION: must be called with interrupts already cleared
    ; cli down below protects only from altering the SP

    push r0
    in   r0, AU_SREG        ; SREG from exiting context, included I bit
    push r0
    cli
    push r1
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r18
    push r19
    push r20
    push r21
    push r22
    push r23
    push r24
    push r25
    push r26
    push r27
    push r28
    push r29
    push r30
    push r31

    ; --- *saved_sp = SP ---
    movw r30, r24           ; Z = saved_sp
    in   r18, AU_SPL
    in   r19, AU_SPH
    st   Z,   r18
    std  Z+1, r19

    ; --- SP = next_sp ---
    out  AU_SPH, r23
    out  AU_SPL, r22

    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r18
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1

    ; --- SREG from incoming context, fully restored
    pop  r0
    out  AU_SREG, r0        ; I bit is the one saved from incoming task

    pop  r0
    ret

.size au_context_switch, .-au_context_switch
