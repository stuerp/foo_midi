
Frac proc

  fld     st(0)
  sub     esp, 4
  fnstcw  word ptr [ESP]             ; save
  fnstcw  word ptr [ESP + 2]         ; scratch
  fwait
  or      word ptr [ESP + 2], 0F00h  ; trunc toward zero, full precision
  fldcw   word ptr [ESP + 2]
  frndint
  fwait
  fldcw   word ptr [ESP]
  add     ESP, 4
  fsub
 
  ret

Frac endp

;-----------------------------------------------------------

Trunc proc
  ; ->    FST(0)   Extended argument
  ; <-    EDX:EAX  Result

  sub     esp,12
  fnstcw  word ptr [esp]            ; save
  fnstcw  Word ptr [esp + 2]        ; scratch
  fwait
  or      word ptr [esp + 2], 0F00h ; trunc toward zero, full precision
  fldcw   word ptr [esp + 2]
  fistp   qword ptr [esp + 4]
  fwait
  fldcw   word ptr [esp]
  pop     ecx
  pop     eax
  pop     edx
  ret

Trunc endp

;-----------------------------------------------------------
; Log.10(X) := Log.2(X) * Log.10(2)

Log10 proc Value : REAL8

  fldlg2
  fld Value
  fyl2x
  ret

Log10 endp

;-----------------------------------------------------------

Power proc uses ebx

  ; st0 = Base (X), st1 = Exponent (Y)

  ftst                   ; st0 = X = 0 ?
  fstsw   ax
  sahf
  jz      @@Zero         ; Yes, result = 0 (CF = NC = 0)
  mov     bl, ah         ; BL and 1 = 1 when X < 0, 0 when X > 0
  ja      @@PositiveX    ; X > 0 ?

  fxch                   ; change st0 <-> st1 (st0 = Y, st1 = X)
  fld     st(0)          ; st2 = st1 = X, st1 = st0 = Y, st0 = st0 = Y
  frndint                ; st0 = Round(st0) = Round(Y)
  fcomp                  ; compare st0 and st1; st0 = st1 = Y, st1 = st2 = X, st3 = nil
  fstsw   ax             ; in AH flag ZF = ZR = 1 when whole Y
  sahf                   ; Y whole?
  jnz     @@Error        ; no

  fld1
  fld1
  fadd                   ; st2 = st1 = X, st1 = st0 = Y, st0 = 2
  fld     st(1)          ; st3 = st2 = X, st2 = st1 = Y, st1 = st0 = 2, st0 = st1 = Y
  fprem                  ; st0 = st0 mod st1 = Y mod 2
  ftst                   ; st0 = 0 (Y mod 2 = 0, i.e. even) ?
  fstsw   ax             ; in AH flag ZF = ZR = 1 when even Y (CF = NC = 0)
  fstp    st(0)          ; remove the remainder of the division
  fstp    st(0)          ; remove number 2 (st0), st0 = st1 = Y, st1 = st2 = X, st2 = nil
  fxch                   ; change st0 <-> st1 (st0 = X, st1 = Y)

@@PositiveX:
  fabs                   ; st0 = |st0| = |X|
  fyl2x                  ; st0 = st1 * log2(st0) = Y * log2(|X|)
  fld     st(0)          ; st1 = st0
  frndint                ; st0 = Round(st0)
  fsub    st(1), st(0)   ; st1 = st1 - st0
  fld1                   ; st1 = st0, st0 = 1
  fscale                 ; st0 = st0 * 2^st1
  fstp    st(1)          ; remove st1
  fincstp                ; st7 = st0, st0 = st1
  f2xm1                  ; st0 = (2^st0) - 1
  fmul    st(0), st(7)   ; st0 = st0 * st7
  fdecstp                ; st1 = st0, st0 = st7
  fadd                   ; st0 = st0 + st1, st0 = nil
  ; result in st0 !!!

  test    bl, 1          ; X < 0 ? (CF = NC = 0)
  jz      @@End          ; no, complete
  sahf                   ; Y even
  jz      @@End          ; yes, complete
  fchs                   ; if X < 0, and Y is odd, then change the sign of result

@@End:
  ret

@@Error:
  fldz                   ; load 0
  fstp    st(1)          ; remove X
  stc                    ; CF = CY = 1 - error

@@Zero:
  fstp    st(1)          ; remove Y
  ret

Power endp
