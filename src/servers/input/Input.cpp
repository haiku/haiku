/***********************************************************************
 * AUTHOR: nobody <baron>
 *   FILE: Input.cpp
 *   DATE: Sun Dec 16 15:57:43 2001
 *  DESCR: 
 ***********************************************************************/
#include "Input.h"

/*
 *  Method: BInputDevice::~BInputDevice()
 *   Descr: 
 */
BInputDevice::~BInputDevice()
{
}


/*
 *  Method: BInputDevice::Name()
 *   Descr: 
 */
const char *
BInputDevice::Name() const
{
    return NULL;
}


/*
 *  Method: BInputDevice::Type()
 *   Descr: 
 */
input_device_type
BInputDevice::Type() const
{
    input_device_type dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::IsRunning()
 *   Descr: 
 */
bool
BInputDevice::IsRunning() const
{
    bool dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Start()
 *   Descr: 
 */
status_t
BInputDevice::Start()
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Stop()
 *   Descr: 
 */
status_t
BInputDevice::Stop()
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Control()
 *   Descr: 
 */
status_t
BInputDevice::Control(uint32 code,
                      BMessage *message)
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Start()
 *   Descr: 
 */
status_t
BInputDevice::Start(input_device_type type)
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Stop()
 *   Descr: 
 */
status_t
BInputDevice::Stop(input_device_type type)
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::Control()
 *   Descr: 
 */
status_t
BInputDevice::Control(input_device_type type,
                      uint32 code,
                      BMessage *message)
{
    status_t dummy;

    return dummy;
}


/*
 *  Method: BInputDevice::BInputDevice()
 *   Descr: 
 */
BInputDevice::BInputDevice()
{
}


/*
 *  Method: BInputDevice::set_name_and_type()
 *   Descr: 
 */
void
BInputDevice::set_name_and_type(const char *name,
                                input_device_type type)
{
}


