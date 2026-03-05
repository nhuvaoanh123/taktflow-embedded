/* Diagnostic blinky for LAUNCHXL2-570LC43
 * LEDs may be GIOB[6:7] per FreeRTOS tutorial, OR GIOB[4:5] per schematic.
 * v9 drives ALL four pins (bits 4-7, mask 0xF0) to cover both possibilities.
 *
 * v12: CLKCNTL read-modify-write + FULL ESM re-clear (CCMR4F + ESMSR1-4 + ESMEKR + VIM).
 *      v8-v10 re-clear was incomplete (missed CCMR4F, ESMSR4, ESMSSR2, VIM).
 * v11: DIAGNOSTIC — no CLKCNTL → red off. Confirmed CLKCNTL is the trigger.
 * v10: Read-modify-write CLKCNTL (only set PENA bit 8) — still red.
 * v9: Try GIOB[4:7] = 0xF0 (covers both pin assignments).
 * v8: Write fixed value 0x00010100 to CLKCNTL (no read-modify-write).
 *     Then re-clear ESM in case CLKCNTL write triggers new CCM error.
 *     Abort handler: no CLKCNTL, avoids infinite abort loop.
 *
 * Register map (from HALCoGen reg_system.h):
 *   systemREG1 = 0xFFFFFF00
 *   MINITGCR   = base+0x5C = 0xFFFFFF5C
 *   MSINENA    = base+0x60 = 0xFFFFFF60
 *   MSTCGSTAT  = base+0x68 = 0xFFFFFF68
 *   PLLCTL1    = base+0x70 = 0xFFFFFF70  <-- v5 bug wrote PENA here!
 *   CLKCNTL    = base+0xD0 = 0xFFFFFFD0  <-- correct PENA location
 */

    .arm
    .global _c_int00

    .text

/* ---- Vector table (0x00 - 0x1F) ---- */
    b   _c_int00        @ 0x00: Reset
    b   _abort_blink    @ 0x04: Undefined
    b   _abort_blink    @ 0x08: SVC
    b   _abort_blink    @ 0x0C: Prefetch abort
    b   _abort_blink    @ 0x10: Data abort
    nop                 @ 0x14: Reserved
    b   _abort_blink    @ 0x18: IRQ
    b   _abort_blink    @ 0x1C: FIQ

/* ---- Abort handler: PCR + GIO only, no CLKCNTL (avoid infinite abort) ---- */
_abort_blink:
    @ PCR1 PSPWRDWNCLR0 (0xFFFF10A0)
    mvn r1, #0                  @ r1 = 0xFFFFFFFF
    movw r0, #0x10A0
    movt r0, #0xFFFF
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    @ PCR2 (0xFCFF10A0)
    movw r0, #0x10A0
    movt r0, #0xFCFF
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    @ PCR3 (0xFFF780A0)
    movw r0, #0x80A0
    movt r0, #0xFFF7
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    @ GIO global enable (GIOGCR0 = 0xFFF7BC00)
    movw r0, #0xBC00
    movt r0, #0xFFF7
    mov r1, #1
    str r1, [r0]

    @ GIOB DIR: bits 4-7 = output (GIODIRB = 0xFFF7BC54)
    movw r0, #0xBC54
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

    @ LED ON (GIODSETB = 0xFFF7BC60)
    movw r0, #0xBC60
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

_abort_hang:
    b   _abort_hang

/* ==================================================================
 * Normal entry point
 * ================================================================== */
_c_int00:

    /* ===== STEP 0: Initialize ALL core registers (avoid CCM error) ===== */

    @ Supervisor mode (M=10011)
    mov r0, #0
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ FIQ mode (M=10001)
    cps #17
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ IRQ mode (M=10010)
    cps #18
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ Abort mode (M=10111)
    cps #23
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ Undefined mode (M=11011)
    cps #27
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ System mode (M=11111)
    cps #31
    mov r13, #0
    mov r14, #0
    mrs r1, cpsr
    msr spsr_cxsf, r1

    @ Back to Supervisor mode
    cps #19

    @ Pipeline flush
    bl _flush1
_flush1:
    bl _flush2
_flush2:
    bl _flush3
_flush3:
    bl _flush4
_flush4:

    /* ===== STEP 1: Initialize RAM (hardware auto-init for ECC) ===== */

    mvn r0, #0xA3              @ r0 = 0xFFFFFF5C (MINITGCR)
    mov r1, #0xA
    str r1, [r0]

    mvn r0, #0x9F              @ r0 = 0xFFFFFF60 (MSINENA)
    mov r1, #0x1
    str r1, [r0]

_mem_wait:
    mvn r0, #0x97              @ r0 = 0xFFFFFF68 (MSTCGSTAT)
    ldr r1, [r0]
    tst r1, #0x100
    beq _mem_wait

    mvn r0, #0xA3              @ r0 = 0xFFFFFF5C (MINITGCR)
    mov r1, #5
    str r1, [r0]

    /* ===== STEP 2: PCR power up (no PENA yet) ===== */

    mvn r1, #0                 @ r1 = 0xFFFFFFFF
    movw r0, #0x10A0
    movt r0, #0xFFFF           @ PCR1
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    movw r0, #0x10A0
    movt r0, #0xFCFF           @ PCR2
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    movw r0, #0x80A0
    movt r0, #0xFFF7           @ PCR3
    str r1, [r0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]

    /* ===== STEP 3: Clear ESM/CCM errors + NERROR ===== */

    movw r0, #0xF518
    movt r0, #0xFFFF           @ ESMSR1
    mvn r1, #0
    str r1, [r0]

    movw r0, #0xF51C
    movt r0, #0xFFFF           @ ESMSR2
    str r1, [r0]

    movw r0, #0xF520
    movt r0, #0xFFFF           @ ESMSR4/Group2
    str r1, [r0]

    movw r0, #0xF538
    movt r0, #0xFFFF           @ ESMEKR
    mov r1, #5
    str r1, [r0]               @ Deassert NERROR

    movw r0, #0xF53C
    movt r0, #0xFFFF           @ ESMSSR2
    mov r1, #0x04
    str r1, [r0]

    movw r0, #0xF600
    movt r0, #0xFFFF           @ CCMR4F
    movw r1, #0x0000
    movt r1, #0x0001
    str r1, [r0]

    movw r0, #0xFE20
    movt r0, #0xFFFF           @ VIM INTREQ0
    mov r1, #1
    str r1, [r0]

    @ Re-clear in case CCM refired
    movw r0, #0xF518
    movt r0, #0xFFFF
    mvn r1, #0
    str r1, [r0]

    movw r0, #0xF51C
    movt r0, #0xFFFF
    str r1, [r0]

    movw r0, #0xF538
    movt r0, #0xFFFF
    mov r1, #5
    str r1, [r0]

    /* ===== STEP 4: Enable PENA via read-modify-write ===== */

    mvn r0, #0x2F              @ r0 = 0xFFFFFFD0 (CLKCNTL)
    ldr r1, [r0]               @ Read current CLKCNTL value
    orr r1, r1, #0x100         @ Set PENA (bit 8)
    str r1, [r0]               @ Write back with PENA enabled

    /* ===== STEP 4b: FULL ESM re-clear (CLKCNTL write triggers CCM error) ===== */

    @ Clear CCM error source FIRST
    movw r0, #0xF600
    movt r0, #0xFFFF           @ CCMR4F
    movw r1, #0x0000
    movt r1, #0x0001
    str r1, [r0]

    @ Clear ESM status registers
    movw r0, #0xF518
    movt r0, #0xFFFF           @ ESMSR1
    mvn r1, #0
    str r1, [r0]

    movw r0, #0xF51C
    movt r0, #0xFFFF           @ ESMSR2
    str r1, [r0]

    movw r0, #0xF520
    movt r0, #0xFFFF           @ ESMSR4/Group2
    str r1, [r0]

    movw r0, #0xF53C
    movt r0, #0xFFFF           @ ESMSSR2
    mov r1, #0x04
    str r1, [r0]

    movw r0, #0xFE20
    movt r0, #0xFFFF           @ VIM INTREQ0
    mov r1, #1
    str r1, [r0]

    @ Deassert NERROR
    movw r0, #0xF538
    movt r0, #0xFFFF           @ ESMEKR
    mov r1, #5
    str r1, [r0]

    @ Double-clear ESMSR1/SR2 + NERROR in case CCM refired
    movw r0, #0xF518
    movt r0, #0xFFFF
    mvn r1, #0
    str r1, [r0]

    movw r0, #0xF51C
    movt r0, #0xFFFF
    str r1, [r0]

    movw r0, #0xF538
    movt r0, #0xFFFF
    mov r1, #5
    str r1, [r0]

    /* ===== STEP 5: GIO enable + LED blink ===== */

    @ GIO global enable
    movw r0, #0xBC00
    movt r0, #0xFFF7
    mov r1, #1
    str r1, [r0]

    @ GIOB DIR: bits 4-7 = output
    movw r0, #0xBC54
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

    @ LED ON immediately
    movw r0, #0xBC60
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

    /* ===== STEP 6: Blink loop ===== */
_blink:
    @ ON
    movw r0, #0xBC60
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

    movw r2, #0xC6C0
    movt r2, #0x002D           @ r2 = 3000000
_d1:
    subs r2, r2, #1
    bne _d1

    @ OFF
    movw r0, #0xBC64
    movt r0, #0xFFF7
    mov r1, #0xF0
    str r1, [r0]

    movw r2, #0xC6C0
    movt r2, #0x002D           @ r2 = 3000000
_d2:
    subs r2, r2, #1
    bne _d2

    b   _blink

    .end
