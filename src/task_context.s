/*
 * Aurum - task context switching
 *
 * ATmega328P / AVR
 */

#include <avr/io.h>

.equ SREG, 0x3F
.equ SPL,  0x3D
.equ SPH,  0x3E

.global au_context_switch

/*
 * Context frame
 *
 * After saving the context, SP points to:
 *
 *   +0   R31
 *   +1   R30
 *   +2   R29
 *   +3   R28
 *   +4   R27
 *   +5   R26
 *   +6   R25
 *   +7   R24
 *   +8   R23
 *   +9   R22
 *   +10  R21
 *   +11  R20
 *   +12  R19
 *   +13  R18
 *   +14  R17
 *   +15  R16
 *   +16  R15
 *   +17  R14
 *   +18  R13
 *   +19  R12
 *   +20  R11
 *   +21  R10
 *   +22  R9
 *   +23  R8
 *   +24  R7
 *   +25  R6
 *   +26  R5
 *   +27  R4
 *   +28  R3
 *   +29  R2
 *   +30  R1
 *   +31  SREG
 *   +32  R0
 *   +33  return address HIGH
 *   +34  return address LOW
 *
 *
 * AVR-GCC ABI:
 *
 *   saved_sp = r25:r24
 *   next_sp  = r23:r22
 */

.global au_context_switch
.type au_context_switch, @function

.global au_context_switch
.type   au_context_switch, @function

au_context_switch:

    ; --- salvataggio del contesto corrente ---
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
    push r24            ; slot argomento (context del task)
    push r25
    push r28
    push r29

    ; --- *saved_sp = SP ---
    movw r30, r24       ; Z = saved_sp (push non tocca r24/r25)
    in   r18, SPL
    in   r19, SPH
    st   Z,   r18
    std  Z+1, r19

    ; --- SP = next_sp (atomico rispetto agli interrupt) ---
    in   r0, SREG
    cli
    out  SPH, r23
    out  SPL, r22
    out  SREG, r0

    ; --- ripristino del contesto di destinazione ---
    pop r29
    pop r28
    pop r25
    pop r24
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

    ret

.size au_context_switch, .-au_context_switch
