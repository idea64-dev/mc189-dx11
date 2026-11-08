; The x87 FSIN and FCOS instructions, which HotSpot uses for Math.sin and Math.cos when |x| <= pi/4
; (MacroAssembler::trigfunc). The FPU runs with the Windows x64 default control word (53-bit precision), as in
; the JVM.

.code

; double x87Sin(double x)
x87Sin PROC
    movsd qword ptr [rsp + 8], xmm0
    fld qword ptr [rsp + 8]
    fsin
    fstp qword ptr [rsp + 8]
    movsd xmm0, qword ptr [rsp + 8]
    ret
x87Sin ENDP

; double x87Cos(double x)
x87Cos PROC
    movsd qword ptr [rsp + 8], xmm0
    fld qword ptr [rsp + 8]
    fcos
    fstp qword ptr [rsp + 8]
    movsd xmm0, qword ptr [rsp + 8]
    ret
x87Cos ENDP

END
