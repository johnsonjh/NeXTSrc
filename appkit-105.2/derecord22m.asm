;;; derecord.asm
;;; DSP code to listen to serial port and send data in DMA mode.
;;; Author: Robert D. Poor, NeXT Technical Support
;;; Copyright 1989 NeXT, Inc.  Next, Inc. is furnishing this software
;;; for example purposes only and assumes no liability for its use.
;;;
;;; Edit history (most recent edits first)
;;;
;;; 04-Apr-90 Mike Minnick: cheap 22K mono version - returns every 4th sample
;;; 11-Sep-89 Rob Poor: Converted to use dspstream.asm rather than
;;;	dspsound.asm, also eliminating need for portdefs.asm
;;; 07-Sep-89 Rob Poor: Created (based on code by Lee Boynton)
;;;
;;; End of edit history

	include	"dspstream.asm"

DMASIZE		equ	1024
BUFSIZE		equ	DMASIZE*4

main
	move	#>XRAMLO,r5		;
	move	r5,x:1			;
	move	#>XRAMLO+BUFSIZE,r6	;use external RAM for double buffers
	move	r6,x:0			;
	move	#>BUFSIZE-1,m5		;
	move	#>BUFSIZE-1,m6		;
	move	#4,n6			;send every 4th sample
	move	#1,n5			;
	move	#>$008000,y1		;scale factor for 16 bit output
	movep   #>$0001F7,x:m_pcc	;both serial ports (SC0 not available)
	movep	#$4100,x:m_cra		; Set up serial port
	movep	#$2a00,x:m_crb		; 	in network mode
	;; most properly, this would be the place to enable interrupts...
	move	#>BUFSIZE,a
	do	a,prime2		;prime the first buffer
prime					;
	jclr	#m_rdf,x:m_sr,prime 	;
	movep	x:m_rx,x:(r5)+		;
prime2					;
					;
loop					;while (1) {
	move	x:1,r6			;
	move	x:0,r5			;
	move	r6,x:0			;    swap buffers
	move	r5,x:1			;
beginBuf				;
	jclr	#m_rdf,x:m_sr,_beg2	;
	movep	x:m_rx,x:(r5)+	;
_beg2					;
	jclr	#m_htde,x:m_hsr,beginBuf	;    start DMA
	movep	#DM_R_REQ,x:m_htx		;
_ackBegin				;
	jclr	#m_rdf,x:m_sr,_ack2	;
	movep	x:m_rx,x:(r5)+	;
_ack2					;
	jclr	#m_hf1,x:m_hsr,_ackBegin	;
	
	move	#>DMASIZE,a		;
	do	a,senddone		;    send the output buffer
send					;
	jclr	#m_rdf,x:m_sr,_foo	;
	movep	x:m_rx,x:(r5)+	;
_foo					;
	jclr	#m_htde,x:m_hsr,send	;
	move	x:(r6)+n6,x1		;send every 4th sample
	mpy	x1,y1,a			;
	move	a,x:m_htx			;    
senddone				;
					;
_prodDMA				;    handshake the DMA
	btst	#DMA_DONE,x:x_sFlags	;
	jcs	endDMA			;
	jclr	#m_rdf,x:m_sr,_foo	;
	movep	x:m_rx,x:(r5)+	;
_foo					;
	jclr	#m_htde,x:m_hsr,_prodDMA	;
	movep	#0,x:m_htx		;
	jmp	_prodDMA		;
endDMA					;
	bclr	#DMA_DONE,x:x_sFlags	;
_ackEnd					;
	jclr	#m_rdf,x:m_sr,_foo	;
	movep	x:m_rx,x:(r5)+	;
_foo					;
	jset	#m_hf1,x:m_hsr,_ackEnd	;
					;
finin					;    finish filling the input buffer
	move	x:1,b			;
	move	r5,a			;
	cmp	a,b			;
	jeq	loop_end		;
_foo					;
	jclr	#m_rdf,x:m_sr,_foo	;
	movep	x:m_rx,x:(r5)+	;
	jmp	finin			;}
loop_end				;
	jset	#m_hf0,x:m_hsr,stop	;
	jmp	loop			;




