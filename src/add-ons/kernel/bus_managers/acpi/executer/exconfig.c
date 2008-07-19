/******************************************************************************
 *
 * Module Name: exconfig - Namespace reconfiguration (Load/Unload opcodes)
 *              $Revision: 1.113 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2008, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code.  No other license or right
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
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
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
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY

 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
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
 *****************************************************************************/

#define __EXCONFIG_C__

#include "acpi.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "actables.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exconfig")

/* Local prototypes */

static ACPI_STATUS
AcpiExAddTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *ParentNode,
    ACPI_OPERAND_OBJECT     **DdbHandle);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAddTable
 *
 * PARAMETERS:  Table               - Pointer to raw table
 *              ParentNode          - Where to load the table (scope)
 *              DdbHandle           - Where to return the table handle.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Common function to Install and Load an ACPI table with a
 *              returned table handle.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiExAddTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *ParentNode,
    ACPI_OPERAND_OBJECT     **DdbHandle)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE (ExAddTable);


    /* Create an object to be the table handle */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_REFERENCE);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Init the table handle */

    ObjDesc->Reference.Opcode = AML_LOAD_OP;
    *DdbHandle = ObjDesc;

    /* Install the new table into the local data structures */

    ObjDesc->Reference.Object = ACPI_TO_POINTER (TableIndex);

    /* Add the table to the namespace */

    Status = AcpiNsLoadTable (TableIndex, ParentNode);
    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ObjDesc);
        *DdbHandle = NULL;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExLoadTableOp
 *
 * PARAMETERS:  WalkState           - Current state with operands
 *              ReturnDesc          - Where to store the return object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load an ACPI table from the RSDT/XSDT
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExLoadTableOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     **ReturnDesc)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_NAMESPACE_NODE     *ParentNode;
    ACPI_NAMESPACE_NODE     *StartNode;
    ACPI_NAMESPACE_NODE     *ParameterNode = NULL;
    ACPI_OPERAND_OBJECT     *DdbHandle;
    ACPI_TABLE_HEADER       *Table;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (ExLoadTableOp);


    /* Validate lengths for the SignatureString, OEMIDString, OEMTableID */

    if ((Operand[0]->String.Length > ACPI_NAME_SIZE) ||
        (Operand[1]->String.Length > ACPI_OEM_ID_SIZE) ||
        (Operand[2]->String.Length > ACPI_OEM_TABLE_ID_SIZE))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Find the ACPI table in the RSDT/XSDT */

    Status = AcpiTbFindTable (Operand[0]->String.Pointer,
                              Operand[1]->String.Pointer,
                              Operand[2]->String.Pointer, &TableIndex);
    if (ACPI_FAILURE (Status))
    {
        if (Status != AE_NOT_FOUND)
        {
            return_ACPI_STATUS (Status);
        }

        /* Table not found, return an Integer=0 and AE_OK */

        DdbHandle = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!DdbHandle)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        DdbHandle->Integer.Value = 0;
        *ReturnDesc = DdbHandle;

        return_ACPI_STATUS (AE_OK);
    }

    /* Default nodes */

    StartNode = WalkState->ScopeInfo->Scope.Node;
    ParentNode = AcpiGbl_RootNode;

    /* RootPath (optional parameter) */

    if (Operand[3]->String.Length > 0)
    {
        /*
         * Find the node referenced by the RootPathString.  This is the
         * location within the namespace where the table will be loaded.
         */
        Status = AcpiNsGetNode (StartNode, Operand[3]->String.Pointer,
                    ACPI_NS_SEARCH_PARENT, &ParentNode);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* ParameterPath (optional parameter) */

    if (Operand[4]->String.Length > 0)
    {
        if ((Operand[4]->String.Pointer[0] != '\\') &&
            (Operand[4]->String.Pointer[0] != '^'))
        {
            /*
             * Path is not absolute, so it will be relative to the node
             * referenced by the RootPathString (or the NS root if omitted)
             */
            StartNode = ParentNode;
        }

        /* Find the node referenced by the ParameterPathString */

        Status = AcpiNsGetNode (StartNode, Operand[4]->String.Pointer,
                    ACPI_NS_SEARCH_PARENT, &ParameterNode);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Load the table into the namespace */

    Status = AcpiExAddTable (TableIndex, ParentNode, &DdbHandle);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Parameter Data (optional) */

    if (ParameterNode)
    {
        /* Store the parameter data into the optional parameter object */

        Status = AcpiExStore (Operand[5],
                    ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, ParameterNode),
                    WalkState);
        if (ACPI_FAILURE (Status))
        {
            (void) AcpiExUnloadTable (DdbHandle);
            return_ACPI_STATUS (Status);
        }
    }

    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_SUCCESS (Status))
    {
        ACPI_INFO ((AE_INFO,
            "Dynamic OEM Table Load - [%.4s] OemId [%.6s] OemTableId [%.8s]",
            Table->Signature, Table->OemId, Table->OemTableId));
    }

    /* Invoke table handler if present */

    if (AcpiGbl_TableHandler)
    {
        (void) AcpiGbl_TableHandler (ACPI_TABLE_EVENT_LOAD, Table,
                    AcpiGbl_TableHandlerContext);
    }

    *ReturnDesc = DdbHandle;
    return_ACPI_STATUS  (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExLoadOp
 *
 * PARAMETERS:  ObjDesc         - Region or Buffer/Field where the table will be
 *                                obtained
 *              Target          - Where a handle to the table will be stored
 *              WalkState       - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load an ACPI table from a field or operation region
 *
 * NOTE: Region Fields (Field, BankField, IndexFields) are resolved to buffer
 *       objects before this code is reached.
 *
 *       If source is an operation region, it must refer to SystemMemory, as
 *       per the ACPI specification.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExLoadOp (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *Target,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *DdbHandle;
    ACPI_TABLE_HEADER       *Table;
    ACPI_TABLE_DESC         TableDesc;
    UINT32                  TableIndex;
    ACPI_STATUS             Status;
    UINT32                  Length;


    ACPI_FUNCTION_TRACE (ExLoadOp);


    ACPI_MEMSET (&TableDesc, 0, sizeof (ACPI_TABLE_DESC));

    /* Source Object can be either an OpRegion or a Buffer/Field */

    switch (ACPI_GET_OBJECT_TYPE (ObjDesc))
    {
    case ACPI_TYPE_REGION:

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Load table from Region %p\n", ObjDesc));

        /* Region must be SystemMemory (from ACPI spec) */

        if (ObjDesc->Region.SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY)
        {
            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        /*
         * If the Region Address and Length have not been previously evaluated,
         * evaluate them now and save the results.
         */
        if (!(ObjDesc->Common.Flags & AOPOBJ_DATA_VALID))
        {
            Status = AcpiDsGetRegionArguments (ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }

        /*
         * Map the table header and get the actual table length. The region
         * length is not guaranteed to be the same as the table length.
         */
        Table = AcpiOsMapMemory (ObjDesc->Region.Address,
                    sizeof (ACPI_TABLE_HEADER));
        if (!Table)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        Length = Table->Length;
        AcpiOsUnmapMemory (Table, sizeof (ACPI_TABLE_HEADER));

        /* Must have at least an ACPI table header */

        if (Length < sizeof (ACPI_TABLE_HEADER))
        {
            return_ACPI_STATUS (AE_INVALID_TABLE_LENGTH);
        }

        /*
         * The memory region is not guaranteed to remain stable and we must
         * copy the table to a local buffer. For example, the memory region
         * is corrupted after suspend on some machines. Dynamically loaded
         * tables are usually small, so this overhead is minimal.
         */

        /* Allocate a buffer for the table */

        TableDesc.Pointer = ACPI_ALLOCATE (Length);
        if (!TableDesc.Pointer)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Map the entire table and copy it */

        Table = AcpiOsMapMemory (ObjDesc->Region.Address, Length);
        if (!Table)
        {
            ACPI_FREE (TableDesc.Pointer);
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        ACPI_MEMCPY (TableDesc.Pointer, Table, Length);
        AcpiOsUnmapMemory (Table, Length);

        TableDesc.Address = ObjDesc->Region.Address;
        break;


    case ACPI_TYPE_BUFFER: /* Buffer or resolved RegionField */

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Load table from Buffer or Field %p\n", ObjDesc));

        /* Must have at least an ACPI table header */

        if (ObjDesc->Buffer.Length < sizeof (ACPI_TABLE_HEADER))
        {
            return_ACPI_STATUS (AE_INVALID_TABLE_LENGTH);
        }

        /* Get the actual table length from the table header */

        Table = ACPI_CAST_PTR (ACPI_TABLE_HEADER, ObjDesc->Buffer.Pointer);
        Length = Table->Length;

        /* Table cannot extend beyond the buffer */

        if (Length > ObjDesc->Buffer.Length)
        {
            return_ACPI_STATUS (AE_AML_BUFFER_LIMIT);
        }
        if (Length < sizeof (ACPI_TABLE_HEADER))
        {
            return_ACPI_STATUS (AE_INVALID_TABLE_LENGTH);
        }

        /*
         * Copy the table from the buffer because the buffer could be modified
         * or even deleted in the future
         */
        TableDesc.Pointer = ACPI_ALLOCATE (Length);
        if (!TableDesc.Pointer)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        ACPI_MEMCPY (TableDesc.Pointer, Table, Length);
        TableDesc.Address = ACPI_TO_INTEGER (TableDesc.Pointer);
        break;


    default:
        return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
    }

    /* Validate table checksum (will not get validated in TbAddTable) */

    Status = AcpiTbVerifyChecksum (TableDesc.Pointer, Length);
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (TableDesc.Pointer);
        return_ACPI_STATUS (Status);
    }

    /* Complete the table descriptor */

    TableDesc.Length = Length;
    TableDesc.Flags = ACPI_TABLE_ORIGIN_ALLOCATED;

    /* Install the new table into the local data structures */

    Status = AcpiTbAddTable (&TableDesc, &TableIndex);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /*
     * Add the table to the namespace.
     *
     * Note: Load the table objects relative to the root of the namespace.
     * This appears to go against the ACPI specification, but we do it for
     * compatibility with other ACPI implementations.
     */
    Status = AcpiExAddTable (TableIndex, AcpiGbl_RootNode, &DdbHandle);
    if (ACPI_FAILURE (Status))
    {
        /* On error, TablePtr was deallocated above */

        return_ACPI_STATUS (Status);
    }

    /* Store the DdbHandle into the Target operand */

    Status = AcpiExStore (DdbHandle, Target, WalkState);
    if (ACPI_FAILURE (Status))
    {
        (void) AcpiExUnloadTable (DdbHandle);

        /* TablePtr was deallocated above */

        AcpiUtRemoveReference (DdbHandle);
        return_ACPI_STATUS (Status);
    }

    /* Invoke table handler if present */

    if (AcpiGbl_TableHandler)
    {
        (void) AcpiGbl_TableHandler (ACPI_TABLE_EVENT_LOAD, TableDesc.Pointer,
                    AcpiGbl_TableHandlerContext);
    }

Cleanup:
    if (ACPI_FAILURE (Status))
    {
        /* Delete allocated table buffer */

        AcpiTbDeleteTable (&TableDesc);
    }
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExUnloadTable
 *
 * PARAMETERS:  DdbHandle           - Handle to a previously loaded table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Unload an ACPI table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExUnloadTable (
    ACPI_OPERAND_OBJECT     *DdbHandle)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     *TableDesc = DdbHandle;
    UINT32                  TableIndex;
    ACPI_TABLE_HEADER       *Table;


    ACPI_FUNCTION_TRACE (ExUnloadTable);


    /*
     * Validate the handle
     * Although the handle is partially validated in AcpiExReconfiguration(),
     * when it calls AcpiExResolveOperands(), the handle is more completely
     * validated here.
     */
    if ((!DdbHandle) ||
        (ACPI_GET_DESCRIPTOR_TYPE (DdbHandle) != ACPI_DESC_TYPE_OPERAND) ||
        (ACPI_GET_OBJECT_TYPE (DdbHandle) != ACPI_TYPE_LOCAL_REFERENCE))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Get the table index from the DdbHandle (ACPI_SIZE for 64-bit case) */

    TableIndex = (UINT32) (ACPI_SIZE) TableDesc->Reference.Object;

    /* Invoke table handler if present */

    if (AcpiGbl_TableHandler)
    {
        Status = AcpiGetTableByIndex (TableIndex, &Table);
        if (ACPI_SUCCESS (Status))
        {
            (void) AcpiGbl_TableHandler (ACPI_TABLE_EVENT_UNLOAD, Table,
                        AcpiGbl_TableHandlerContext);
        }
    }

    /*
     * Delete the entire namespace under this table Node
     * (Offset contains the TableId)
     */
    AcpiTbDeleteNamespaceByOwner (TableIndex);
    (void) AcpiTbReleaseOwnerId (TableIndex);

    AcpiTbSetTableLoadedFlag (TableIndex, FALSE);

    /* Table unloaded, remove a reference to the DdbHandle object */

    AcpiUtRemoveReference (DdbHandle);
    return_ACPI_STATUS (AE_OK);
}

