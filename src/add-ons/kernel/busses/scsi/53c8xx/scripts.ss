;
; BeOS 53c8xx SCRIPTS
;
ARCH     810

; Interrupt codes used to signal driver from SCRIPTS
;
ABSOLUTE status_ready           = 0x10  ; idle loop interrupted by driver
ABSOLUTE status_reselected      = 0x11  ; select or idle interrupted by reselection
ABSOLUTE status_timeout         = 0x12  ; select timed out
ABSOLUTE status_selected        = 0x13  ; select succeeded
ABSOLUTE status_complete        = 0x14  ; transaction completed
ABSOLUTE status_disconnect      = 0x15  ; device disconnected in the middle
ABSOLUTE status_badstatus       = 0x16  ; snafu in the status phase
ABSOLUTE status_overrun         = 0x17  ; data overrun occurred
ABSOLUTE status_underrun        = 0x18  ; data underrun occurred
ABSOLUTE status_badphase        = 0x19  ; weird phase transition occurred
ABSOLUTE status_badmsg          = 0x1a  ; bad msg received
ABSOLUTE status_badextmsg       = 0x1b  ; bad extended msg received
ABSOLUTE status_selftest        = 0x1c  ; used by selftest stub
ABSOLUTE status_iocomplete      = 0x1d  ; used by driver
ABSOLUTE status_syncin          = 0x1e  ; inbound sync msg
ABSOLUTE status_widein          = 0x1f  ; inbound wdtr msg
ABSOLUTE status_ignore_residue  = 0x20  ; ignore wide residue bytes

; DSA-offset data table
;
ABSOLUTE ctxt_device      = 0x00             ; targ id, sync params, etc
ABSOLUTE ctxt_sendmsg     = 0x08             ; outgoing (ID) msg(1) pointer
ABSOLUTE ctxt_recvmsg     = 0x10             ; incoming msg(1) pointer
ABSOLUTE ctxt_extdmsg     = 0x18             ; extdmsg(1) pointer
ABSOLUTE ctxt_syncmsg     = 0x20             ; sync(2) pointer
ABSOLUTE ctxt_status      = 0x28             ; status(1) pointer
ABSOLUTE ctxt_command     = 0x30             ; command(6,10,12) pointer
ABSOLUTE ctxt_widemsg     = 0x38             ; wide(2) pointer

ENTRY start
ENTRY idle
ENTRY switch
ENTRY switch_resel
ENTRY phase_dataerr
ENTRY test

ENTRY do_datain
ENTRY do_dataout

; ------------------------------------------------------------------------------------
; Idle loop -- when no new requests are pending wait for a reselect
; or interrupt from the driver
;
idle:
	WAIT RESELECT interrupted
	INT status_reselected
	
interrupted:	
	MOVE CTEST2 TO SFBR                      ; read sigp to clear it
	INT status_ready		

	
; ------------------------------------------------------------------------------------
; Driver must load DSA and jump here to attempt to select a target, beginning a new
; transaction.  Interrupt will indicate success, timeout, or reselection
;
start:
	SELECT ATN FROM ctxt_device, resel       ; try to select the dev for the req
	JUMP selected, WHEN MSG_OUT              ; force wait for timeout
	INT status_timeout
	
selected:
	INT status_selected		

resel:
	INT status_reselected


; ------------------------------------------------------------------------------------
; If we're entering the main dispatcher after a reselection, we must insure that 
; the registers for SYNC/WIDE transfers are properly loaded.  This is the solution
; suggested in 9-18 of the Symbios programming guide.
;
switch_resel:
	SELECT FROM ctxt_device, switch          ; force the load of SXFER/SCNTL3, etc
	
; ------------------------------------------------------------------------------------
; Main activity entry -- driver must set DSA and patch do_datain and do_dataout
; before starting this dispatch function.
;	
switch:
	JUMP phase_msgin, WHEN MSG_IN
	JUMP phase_msgout, IF MSG_OUT	
	JUMP phase_command, IF CMD
do_datain:	                                 ; Patched by driver 
	JUMP phase_dataerr, IF DATA_IN
do_dataout:                                  ; Patched by driver
	JUMP phase_dataerr, IF DATA_OUT
	JUMP phase_status, IF STATUS
	INT status_badphase

phase_msgin:
	MOVE FROM ctxt_recvmsg, WHEN MSG_IN
	JUMP phase_msgin_ext, IF 0x01
	JUMP disc, IF 0x04
	JUMP ignore, IF 0x23       ; /* wide ignore residue */
	CLEAR ACK
	JUMP switch, IF 0x02       ; ignore save data pointers
	JUMP switch, IF 0x07       ; ignore message reject :)
	JUMP switch, IF 0x03       ; ignore restore data pointers
	JUMP switch, IF 0x80       ; ignore ident after reselect
	JUMP switch, IF 0x81       ; ignore ident after reselect
	JUMP switch, IF 0x82       ; ignore ident after reselect
	JUMP switch, IF 0x83       ; ignore ident after reselect
	JUMP switch, IF 0x84       ; ignore ident after reselect
	JUMP switch, IF 0x85       ; ignore ident after reselect
	JUMP switch, IF 0x86       ; ignore ident after reselect
	JUMP switch, IF 0x87       ; ignore ident after reselect
	JUMP switch, IF 0xC0       ; ignore ident after reselect
	JUMP switch, IF 0xC1       ; ignore ident after reselect
	JUMP switch, IF 0xC2       ; ignore ident after reselect
	JUMP switch, IF 0xC3       ; ignore ident after reselect
	JUMP switch, IF 0xC4       ; ignore ident after reselect
	JUMP switch, IF 0xC5       ; ignore ident after reselect
	JUMP switch, IF 0xC6       ; ignore ident after reselect
	JUMP switch, IF 0xC7       ; ignore ident after reselect
	INT status_badmsg

ignore:
	CLEAR ACK
	MOVE FROM ctxt_extdmsg, WHEN MSG_IN  ; read status byte
	CLEAR ACK
	JUMP switch
;	INT status_ignore_residue
	
phase_msgin_ext:
	CLEAR ACK
	MOVE FROM ctxt_extdmsg, WHEN MSG_IN  ; read extended message length
	JUMP phase_msgin_sync, IF 0x03
	JUMP phase_msgin_wide, IF 0x02
	INT status_badextmsg

phase_msgin_wide:
	CLEAR ACK
	MOVE FROM ctxt_widemsg, WHEN MSG_IN
	CLEAR ACK
	INT status_widein
	
phase_msgin_sync:
	CLEAR ACK
	MOVE FROM ctxt_syncmsg, WHEN MSG_IN
	CLEAR ACK
	INT status_syncin
	
phase_msgout:
	MOVE FROM ctxt_sendmsg, WHEN MSG_OUT
	JUMP switch
	
phase_command:
	MOVE FROM ctxt_command, WHEN CMD
	JUMP switch
	
phase_dataerr:
	INT status_overrun
	
phase_status:
	MOVE FROM ctxt_status, WHEN STATUS
	INT status_badstatus, WHEN NOT MSG_IN
	MOVE FROM ctxt_recvmsg, WHEN MSG_IN
	INT status_badmsg, IF NOT 0x00 ; not disconnect?!
	MOVE SCNTL2 & 0x7f TO SCNTL2
	CLEAR ACK
	WAIT DISCONNECT
	INT status_complete

disc:
	MOVE SCNTL2 & 0x7f to SCNTL2              ; expect disconnect
	CLEAR ACK
	WAIT DISCONNECT
	INT status_disconnect

; ------------------------------------------------------------------------------------
; Self-test snippet
;
test:
	INT status_selftest


	