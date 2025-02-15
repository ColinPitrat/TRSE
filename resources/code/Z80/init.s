


mul_8x8:

    sla h  ; optimised 1st iteration
    jr nc,mul_8x8_s1
    ld l,e
mul_8x8_s1:

    add hl,hl
    jr nc,mul_8x8_s2
    add hl,de
mul_8x8_s2:

    add hl,hl
    jr nc,mul_8x8_s3
    add hl,de
mul_8x8_s3:

    add hl,hl
    jr nc,mul_8x8_s4
    add hl,de
mul_8x8_s4:

    add hl,hl
    jr nc,mul_8x8_s5
    add hl,de
mul_8x8_s5:

    add hl,hl
    jr nc,mul_8x8_s6
    add hl,de
mul_8x8_s6:

    add hl,hl
    jr nc,mul_8x8_s7
    add hl,de
mul_8x8_s7:

    add hl,hl
    jr nc,mul_8x8_s8
    add hl,de
mul_8x8_s8:

    ret



mul_16x8:
    add a,a  ; optimised 1st iteration
    jr nc,mul_16x8_s1
    ld h,d
    ld l,e
mul_16x8_s1:

    add hl,hl
    rla
    jr nc,mul_16x8_s2
    add hl,de
    adc a,c
mul_16x8_s2:


    add hl,hl
    rla
    jr nc,mul_16x8_s3
    add hl,de
    adc a,c
mul_16x8_s3:

    add hl,hl
    rla
    jr nc,mul_16x8_s4
    add hl,de
    adc a,c
mul_16x8_s4:

    add hl,hl
    rla
    jr nc,mul_16x8_s5
    add hl,de
    adc a,c
mul_16x8_s5:

    add hl,hl
    rla
    jr nc,mul_16x8_s6
    add hl,de
    adc a,c
mul_16x8_s6:

    add hl,hl
    rla
    jr nc,mul_16x8_s7
    add hl,de
    adc a,c
mul_16x8_s7:

    add hl,hl
    rla
    jr nc,mul_16x8_s8
    add hl,de
    adc a,c
mul_16x8_s8:
    ret


div_16x8
;div_hl_c:
   xor	a
   ld	b, 16

_loop:
   add	hl, hl
   rla
   jr	c, $+5
   cp	c
   jr	c, $+4

   sub	c
   inc	l

   djnz	_loop

   ret


div_16x16:
   ld	hl, 0
   ld	b, 16

_loop16:
   sll	c
   rla
   adc	hl, hl
   sbc	hl, de
   jr	nc, $+4
   add	hl, de
   dec	c

   djnz	_loop16

   ret
