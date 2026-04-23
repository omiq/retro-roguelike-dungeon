.include 	"zeropage.inc"
.include 	"vic20.inc"

.export		_load2
.import		SETLFS, SETNAM, LOAD, popax, _printscr
; ---------------------------------------------------------------
; void __near__ __fastcall__ load2 (char *s, unsigned char nsize)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_load2: near

.segment	"CODE"

	;jsr     pusha
	;jsr     popa
	sta     tmp1
	jsr     popax
	sta     ptr2
	stx     ptr2+1
;jsr	_printscr
	lda     #1
	ldx     #8
	ldy     #1
	jsr     SETLFS
	lda     tmp1
	ldx     ptr2
	ldy     ptr2+1
	jsr     SETNAM
	lda	#0
	tax
	tay
	jmp     LOAD

.endproc

