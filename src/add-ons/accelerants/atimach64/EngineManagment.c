#include "GlobalData.h"
#include "generic.h"


static engine_token ati_engine_token = { 1, B_2D_ACCELERATION, NULL };

uint32 ACCELERANT_ENGINE_COUNT(void) {
	return 1;
}

status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et) {
	/* acquire the shared benaphore */
	AQUIRE_BEN(si->engine.lock)
	/* sync if required */
	if (st) SYNC_TO_TOKEN(st);

	/* return an engine token */
	*et = &ati_engine_token;
	return B_OK;
}

status_t RELEASE_ENGINE(engine_token *et, sync_token *st) {
	/* update the sync token, if any */
	if (st) {
		st->engine_id = et->engine_id;
		st->counter = si->engine.count;
	}

	/* release the shared benaphore */
	RELEASE_BEN(si->engine.lock)
	return B_OK;
}

void WAIT_ENGINE_IDLE(void) {

        // Wait until engine finishes
        WaitIdleEmpty(); 
	// note our current possition
	si->engine.last_idle = si->engine.count;
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st) {
	st->engine_id = et->engine_id;
	st->counter = si->engine.count;
	return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st) {

	uint64 fifo_diff;
	uint64 fifo_limit;
	
	/* a quick out */
	if (st->counter < si->engine.last_idle) return B_OK;

	/* the full monty */
	fifo_limit = si->engine.fifo_limit;
	/*	fifo_mask = si->fifo_mask;*/
	do {
		/* calculate the age of the sync token */
		fifo_diff = (vuint64)(si->engine.count) - st->counter;
		/* add in the number of free slots in the fifo */
		fifo_diff += (uint64)((inw(GUI_STAT) >> 16) & 0x003f);
		/*
		   The astute observer will notice that the free slot counter
		   doesn't have enough bits to represent the full FIFO depth.
		   This means that for "recent" operations, we end up waiting
		   on engine idle :-(
		*/
#if 1
		/* add one if the engine is idle (for when st->counter == si->engine.count) */
		if (!(inw(GUI_STAT) & 0x01)) fifo_diff++;
#endif
		/* anything more than fifo_limit fifo slots ago is guaranteed done */
		/* if the engine is idle, bail out */
	} while ((fifo_diff <= fifo_limit) && (inw(GUI_STAT) & 0x01));

	/*	WAIT_ENGINE_IDLE(); */

	si->engine.last_idle = st->counter;
	return B_OK;
}

