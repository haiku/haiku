/******************************************************************************
 *
 * Module Name: evxfevnt - External Interfaces, ACPI event disable/enable
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2025, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************
 *
 * Alternatively, you may choose to be licensed under the terms of the
 * following license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, you may choose to be licensed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 *****************************************************************************/

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxfevnt")


#if (!ACPI_REDUCED_HARDWARE) /* Entire module */
/*******************************************************************************
 *
 * FUNCTION:    AcpiEnable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfers the system into ACPI mode.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnable (
    void)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiEnable);


    /* ACPI tables must be present */

    if (AcpiGbl_FadtIndex == ACPI_INVALID_TABLE_INDEX)
    {
        return_ACPI_STATUS (AE_NO_ACPI_TABLES);
    }

    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Check current mode */

    if (AcpiHwGetMode() == ACPI_SYS_MODE_ACPI)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "System is already in ACPI mode\n"));
    }
    else
    {
        /* Transition to ACPI mode */

        Status = AcpiHwSetMode (ACPI_SYS_MODE_ACPI);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO, "Could not transition to ACPI mode"));
            return_ACPI_STATUS (Status);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "Transition to ACPI mode successful\n"));
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfers the system into LEGACY (non-ACPI) mode.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisable (
    void)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiDisable);


    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    if (AcpiHwGetMode() == ACPI_SYS_MODE_LEGACY)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "System is already in legacy (non-ACPI) mode\n"));
    }
    else
    {
        /* Transition to LEGACY mode */

        Status = AcpiHwSetMode (ACPI_SYS_MODE_LEGACY);

        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not exit ACPI mode to legacy mode"));
            return_ACPI_STATUS (Status);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "ACPI mode disabled\n"));
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnableEvent
 *
 * PARAMETERS:  Event           - The fixed eventto be enabled
 *              Flags           - Reserved
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableEvent (
    UINT32                  Event,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE (AcpiEnableEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Enable the requested fixed event (by writing a one to the enable
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId,
        ACPI_ENABLE_EVENT);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Make sure that the hardware responded */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &Value);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Value != 1)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not enable %s event", AcpiUtGetEventName (Event)));
        return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnableEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisableEvent
 *
 * PARAMETERS:  Event           - The fixed event to be disabled
 *              Flags           - Reserved
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisableEvent (
    UINT32                  Event,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE (AcpiDisableEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Disable the requested fixed event (by writing a zero to the enable
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId,
        ACPI_DISABLE_EVENT);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &Value);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Value != 0)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not disable %s events", AcpiUtGetEventName (Event)));
        return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisableEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiClearEvent
 *
 * PARAMETERS:  Event           - The fixed event to be cleared
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiClearEvent (
    UINT32                  Event)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiClearEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Clear the requested fixed event (By writing a one to the status
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].StatusRegisterId,
        ACPI_CLEAR_STATUS);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiClearEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetEventStatus
 *
 * PARAMETERS:  Event           - The fixed event
 *              EventStatus     - Where the current status of the event will
 *                                be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtains and returns the current status of the event
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetEventStatus (
    UINT32                  Event,
    ACPI_EVENT_STATUS       *EventStatus)
{
    ACPI_STATUS             Status;
    ACPI_EVENT_STATUS       LocalEventStatus = 0;
    UINT32                  InByte;


    ACPI_FUNCTION_TRACE (AcpiGetEventStatus);


    if (!EventStatus)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Fixed event currently can be dispatched? */

    if (AcpiGbl_FixedEventHandlers[Event].Handler)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_HAS_HANDLER;
    }

    /* Fixed event currently enabled? */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &InByte);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (InByte)
    {
        LocalEventStatus |=
            (ACPI_EVENT_FLAG_ENABLED | ACPI_EVENT_FLAG_ENABLE_SET);
    }

    /* Fixed event currently active? */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].StatusRegisterId, &InByte);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (InByte)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_STATUS_SET;
    }

    (*EventStatus) = LocalEventStatus;
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetEventStatus)

#endif /* !ACPI_REDUCED_HARDWARE */
