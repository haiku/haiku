
#define FUNCTION(x) .global x; .type x,@function; x

.text
	/* gas doesn't seem to get the -m arg... */
	.cpu	68040

	/* that one can be inlined */
FUNCTION(flush_insn_pipeline_040):
	nop
	rts

	/* flush all ATC entries */
FUNCTION(flush_atc_all_040):
	pflusha
	rts

	/* flush all user (non-global) ATC entries */
FUNCTION(flush_atc_user_040):
	pflushan
	rts

	/* flush ATC entries for given address */
FUNCTION(flush_atc_addr_040):
	move.l	(4,%a7),%a0
	pflush	(%a0)
	rts



