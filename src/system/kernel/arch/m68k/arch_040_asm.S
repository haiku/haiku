
#include <asm_defs.h>

.text
	/* gas doesn't seem to get the -m arg... */
	.cpu	68040

	/* that one can be inlined */
FUNCTION(flush_insn_pipeline_040):
	nop
	rts
FUNCTION_END(flush_insn_pipeline_040)

	/* flush all ATC entries */
FUNCTION(flush_atc_all_040):
	pflusha
	rts
FUNCTION_END(flush_atc_all_040)

	/* flush all user (non-global) ATC entries */
FUNCTION(flush_atc_user_040):
	pflushan
	rts
FUNCTION_END(flush_atc_user_040)

	/* flush ATC entries for given address */
FUNCTION(flush_atc_addr_040):
	move.l	(4,%a7),%a0
	pflush	(%a0)
	rts
FUNCTION_END(flush_atc_addr_040)



