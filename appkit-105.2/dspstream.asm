;	dspsound.asm - dma stream support for the NeXT dsp560001
;
;	Written by Lee Boynton
;	Copyright 1988, 1989 NeXT, Inc.
;
; Modification history
; --------------------
;
; 89 Jul 12 r:	Tweaked example file of Lee's to create this file, added
;		documentation.
;
; Documentation
; -------------
;
; This file is intended as an include file for general DSP programs.
; It provides the following subroutines:
;	
;	flush	Flush the current DMA output buffer.
;	stop	Flush the last buffer and signal host to terminate execution.
;	putHost	Put a sample (contained in register A) to the output stream.
;		On return, a set carry bit indicates that the host wants the
;		program to terminate.
;	getHost	Get a sample from the input stream into register A.  On
;		return, a set carry bit indicates that the host wants the
;		program to terminate.
;
; Typical usage:
;
;		include	"dspstream.asm"
;	main
;		<any initialization>
;	loop
;		jsr	getHost		;called zero or more times
;		<some calculations>
;		jsr	putHost		;called one or more times
;		jcs	exit		;in case host aborts
;		<some loop condition ??>
;		j??	loop		;
;	exit
;		<user shutdown, i.e. ramp down for graceful abort>
;		jmp	stop
;

	include 'ioequ.asm'


;;;------------------------- Equates
;;;

XRAMLO		equ	8192		;Low address of external ram
XRAMSIZE	equ	8192		;External ram size
XRAMHI		equ	XRAMLO+XRAMSIZE	;High address + 1 of external ram

LEADPAD		equ	2		;# of buffers before stream begins
TRAILPAD	equ	4		;# of buffers after stream ends
DM_R_REQ	equ	$050001		;message -> host to request dma
VEC_R_DONE	equ	$0024		;host command indicating dma complete


;;;------------------------- Variable locations
;;;

x_sFlags	equ	$00ff		;dspstream flags
DMA_DONE	equ	0		;  indicates that dma is complete
y_sCurSamp	equ	$00ff		;current offset in buffer

x_sPutVal	equ	$00fe		;last value to putHost
y_sDmaSize	equ	$00fe		;dma size


;;;------------------------- Interrupt vectors
;;;
	org	p:$0
	jmp	reset
;;;
	org	p:VEC_R_DONE
	bset	#DMA_DONE,x:x_sFlags
;;;
	org	p:$40

;;;flush - flush the current dma output buffer.
;;;
flush
	move	x:x_sPutVal,a
	move	y:y_sCurSamp,b
	tst	b
	jeq	_flush1			;if (curSamp != 0) {
	jsr	putHost			;   pad last buffer
	jmp	flush
_flush1
	rts

;;; stop - flush last buffer, signal to host and terminate execution.
;;;
stop
	move	x:x_sPutVal,a		;repeat last written value for
	do	#TRAILPAD,_trail	;several buffers (needed if we're
					;writing sndout)
	jsr	putHost
	jsr	flush
	nop
_trail
	movep	#>$000018,x:m_hcr	;set both HF0 and HF1 to
					;  indicate that we have halted
_grave
	jclr	#m_hrdf,x:m_hsr,_grave	;loop until data ready to read.
	movep	x:m_hrx,a		;  read and discard any input data
	jmp	_grave			;  until the cows come home...

;;; putHost - puts a sample to the output stream. Does not return until
;;; successful. Note that this must be called once for each sample in a
;;; sample-frame.
;;; On return, the carry bit indicates that the program should terminate;
;;; normally it is clear.
;;;
;;; This routine destroys the b register, and may affect a0 and a2.
;;; It also uses and depends on r7. External ram is used for buffering.
;;;
putHost
	move	a,x:x_sPutVal
	move	a,x:(r7)+
	move	y:y_sCurSamp,b
	move	#>1,a			;
	add	a,b			;
	move	b,y:y_sCurSamp		;curSamp++
	move	y:y_sDmaSize,a
	cmp	b,a
	jgt	_exit			;if (curSamp < bufSize) exit
	move	#>XRAMLO,r7
_beginBuf
	jclr	#m_htde,x:m_hsr,_beginBuf
	movep	#DM_R_REQ,x:m_htx	;    send "DSP_dm_R_REQ" to host
_ackBegin
	jclr	#m_hf1,x:m_hsr,_ackBegin	;    wait for HF1 to go high
	move	y:y_sDmaSize,b
	do	b,_prodDMA
_send
	jclr	#m_htde,x:m_hsr,_send
	movep	x:(r7)+,x:m_htx			;    send values
_prodDMA
	btst	#DMA_DONE,x:x_sFlags
	jcs	_endDMA
	jclr	#m_htde,x:m_hsr,_prodDMA
	movep	#0,x:m_htx		;send zeros until noticed
	jmp	_prodDMA
_endDMA
	bclr	#DMA_DONE,x:x_sFlags	;be sure we know we are through
_ackEnd
	jset	#m_hf1,x:m_hsr,_ackEnd	;wait for HF1 to go low
	move	#>XRAMLO,r7
	clr	b
	move	b,y:y_sCurSamp		;curSamp = 0
_exit
	move	x:x_sPutVal,a
	btst	#m_hf0,x:m_hsr		;
	rts				;

;;; getHost - returns the next sample from the input stream in register a1.
;;; Waits until there is a sample available.
;;;
getHost
	jset	#m_hf0,x:m_hsr,_exit	;
	jclr	#m_hrdf,x:m_hsr,getHost
	movep	x:m_hrx,a
_exit
	btst	#m_hf0,x:m_hsr		;
	rts

;;; reset - called first then when booting the DSP.  Not usually called
;;; by the user.
;;; 
reset
	movec   #6,omr			;data rom enabled, mode 2
	bset    #0,x:m_pbc		;host port
	bset	#3,x:m_pcddr		;   pc3 is an output with value
	bclr	#3,x:m_pcd		;   zero to enable the external ram
	movep   #>$000000,x:m_bcr	;no wait states on the external sram
        movep   #>$00B400,x:m_ipr  	;intr levels: SSI=2, SCI=1, HOST=0
_reset
	jclr	#m_hrdf,x:m_hsr,_reset
	movep	x:m_hrx,y:y_sDmaSize	;get dma buffer size from host
	move	#>XRAMLO,r7
	clr	a
	move	a,x:x_sFlags		;clear flags
	move	a,y:y_sCurSamp		;curSamp = 0;
	bset    #m_hcie,x:m_hcr		;host command interrupts
	move	#0,sr			;enable interrupts
	jmp	main			;call user's code.

