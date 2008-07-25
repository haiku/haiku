
#define FUNCTION(x) .global x; .type x,@function; x

.text


	/* that one can be inlined */
FUNCTION(flush_insn_pipeline_030):
	nop
	rts

	/* flush all ATC entries */
FUNCTION(flush_atc_all_030):
	pflusha
	rts

	/* flush ATC entries for given address */
FUNCTION(flush_atc_addr_030):
	move.l	(4,%a7),%a0
	pflush	#0,#0,(%a0)
	rts



