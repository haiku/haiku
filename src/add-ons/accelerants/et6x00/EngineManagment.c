/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"


/*****************************************************************************/
static engine_token et6000EngineToken = {1, B_2D_ACCELERATION, NULL};
/*****************************************************************************/
uint32 ACCELERANT_ENGINE_COUNT(void) {
    return 1;
}
/*****************************************************************************/
status_t ACQUIRE_ENGINE(uint32 capabilities,
                        uint32 max_wait,
                        sync_token *st,
                        engine_token **et)
{
    /* acquire the shared benaphore */
    AQUIRE_BEN(si->engine.lock)
    /* sync if required */
    if (st) SYNC_TO_TOKEN(st);

    /* return an engine token */
    *et = &et6000EngineToken;
    return B_OK;
}
/*****************************************************************************/
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
/*****************************************************************************/
void WAIT_ENGINE_IDLE(void) {
    et6000aclWaitIdle();
    /* note our current possition */
    si->engine.lastIdle = si->engine.count;
}
/*****************************************************************************/
status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st) {
    st->engine_id = et->engine_id;
    st->counter = si->engine.count;
    return B_OK;
}
/*****************************************************************************/
status_t SYNC_TO_TOKEN(sync_token *st) {
    WAIT_ENGINE_IDLE();
    si->engine.lastIdle = st->counter;
    return B_OK;
}
/*****************************************************************************/
