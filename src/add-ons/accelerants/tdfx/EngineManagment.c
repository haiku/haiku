/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.

   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/

#include "GlobalData.h"
#include "generic.h"

static engine_token voodoo_engine_token = { 1, B_2D_ACCELERATION, NULL };

uint32 ACCELERANT_ENGINE_COUNT(void)
{
    return 1;
}

status_t
ACQUIRE_ENGINE (
    uint32 capabilities, uint32 max_wait,
    sync_token *st, engine_token **et)
{
    /* acquire the shared benaphore */
    AQUIRE_BEN(si->engine.lock)
    /* sync if required */
    if (st) SYNC_TO_TOKEN(st);

    /* return an engine token */
    *et = &voodoo_engine_token;
    return B_OK;
}

status_t RELEASE_ENGINE (engine_token *et, sync_token *st)
{
    /* update the sync token, if any */
    if (st)
    {
        st->engine_id = et->engine_id;
        st->counter = si->engine.count;
    }

    /* release the shared benaphore */
    RELEASE_BEN(si->engine.lock)
    return B_OK;
}

void WAIT_ENGINE_IDLE(void)
{
    // note our current possition
    si->engine.last_idle = si->engine.count;
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st)
{
    st->engine_id = et->engine_id;
    st->counter = si->engine.count;
    return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st)
{
    si->engine.last_idle = st->counter;
    return B_OK;
}

