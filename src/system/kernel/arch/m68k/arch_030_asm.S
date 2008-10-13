
#include <asm_defs.h>

.text


	/* that one can be inlined */
FUNCTION(flush_insn_pipeline_030):
	nop
	rts
FUNCTION_END(flush_insn_pipeline_030)

	/* flush all ATC entries */
FUNCTION(flush_atc_all_030):
	pflusha
	rts
FUNCTION_END(flush_atc_all_030)

	/* flush ATC entries for given address */
FUNCTION(flush_atc_addr_030):
	move.l	(4,%a7),%a0
	pflush	#0,#0,(%a0)
	rts
FUNCTION_END(flush_atc_addr_030)



