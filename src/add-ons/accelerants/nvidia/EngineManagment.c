/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	modification to call G400 functions and mess-ups - Mark Watson
*/

#define MODULE_BIT 0x10000000

#include "acc_std.h"


static engine_token nv_engine_token = { 1, B_2D_ACCELERATION, NULL };

uint32 ACCELERANT_ENGINE_COUNT(void) {
	return 1;
}

status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et) {
	/* acquire the shared benaphore */
	AQUIRE_BEN(si->engine.lock)
	/* sync if required */
	if (st) SYNC_TO_TOKEN(st);

	/* return an engine token */
	*et = &nv_engine_token;
	return B_OK;
}

status_t RELEASE_ENGINE(engine_token *et, sync_token *st) {
	/* update the sync token, if any */
	if (st) {
		GET_SYNC_TOKEN(et,st);
	}

	/* release the shared benaphore */
	RELEASE_BEN(si->engine.lock)
	return B_OK;
}

void WAIT_ENGINE_IDLE(void) {
	uint32 count;
	/*wait for the engine to be totally idle*/
	count = si->engine.count;
	nv_acc_wait_idle();

	si->engine.last_idle = count;
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st) {
	si->engine.count+=4;
	st->engine_id = et->engine_id;
	st->counter = si->engine.count;
	return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st) {
	/* a quick out */
	if (st->counter <= si->engine.last_idle) return B_OK;

	/* another quick out! */
	if ((st->counter >0xFFFFFFF) && (si->engine.last_idle <0xFFFF)) return B_OK; /*for when counter wraps*/

	/* If not we have to wait :-(*/
	WAIT_ENGINE_IDLE();

	return B_OK;
}
