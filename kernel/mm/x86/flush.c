/*++

Copyright (c) 2014 Minoca Corp. All Rights Reserved

Module Name:

    flush.c

Abstract:

    This module implements cache flushing routines for the memory manager.

Author:

    Evan Green 20-Aug-2014

Environment:

    Kernel

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <minoca/kernel.h>

//
// ---------------------------------------------------------------- Definitions
//

//
// ------------------------------------------------------ Data Type Definitions
//

//
// ----------------------------------------------- Internal Function Prototypes
//

//
// -------------------------------------------------------------------- Globals
//

//
// ------------------------------------------------------------------ Functions
//

KERNEL_API
VOID
MmFlushBufferForDataIn (
    PVOID Buffer,
    UINTN SizeInBytes
    )

/*++

Routine Description:

    This routine flushes a buffer in preparation for incoming I/O from a device.

Arguments:

    Buffer - Supplies the virtual address of the buffer to flush. This buffer
        must be cache-line aligned.

    SizeInBytes - Supplies the size of the buffer to flush, in bytes. This
        size must also be cache-line aligned.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

KERNEL_API
VOID
MmFlushBufferForDataOut (
    PVOID Buffer,
    UINTN SizeInBytes
    )

/*++

Routine Description:

    This routine flushes a buffer in preparation for outgoing I/O to a device.

Arguments:

    Buffer - Supplies the virtual address of the buffer to flush. This buffer
        must be cache-line aligned.

    SizeInBytes - Supplies the size of the buffer to flush, in bytes. This
        size must also be cache-line aligned.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

KERNEL_API
VOID
MmFlushBufferForDataIo (
    PVOID Buffer,
    UINTN SizeInBytes
    )

/*++

Routine Description:

    This routine flushes a buffer in preparation for data that is both
    incoming and outgoing (ie the buffer is read from and written to by an
    external device).

Arguments:

    Buffer - Supplies the virtual address of the buffer to flush. This buffer
        must be cache-line aligned.

    SizeInBytes - Supplies the size of the buffer to flush, in bytes. This
        size must also be cache-line aligned.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

KERNEL_API
VOID
MmFlushBuffer (
    PVOID Buffer,
    UINTN SizeInBytes
    )

/*++

Routine Description:

    This routine flushes a buffer to the point of unification.

Arguments:

    Buffer - Supplies the virtual address of the buffer to flush. This buffer
        must be cache-line aligned.

    SizeInBytes - Supplies the size of the buffer to flush, in bytes. This
        size must also be cache-line aligned.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

VOID
MmSysFlushCache (
    ULONG SystemCallNumber,
    PVOID SystemCallParameter,
    PTRAP_FRAME TrapFrame,
    PULONG ResultSize
    )

/*++

Routine Description:

    This routine responds to system calls from user mode requesting to
    invalidate the instruction cache after changing a code region.

Arguments:

    SystemCallNumber - Supplies the system call number that was requested.

    SystemCallParameter - Supplies a pointer to the parameters supplied with
        the system call. This structure will be a stack-local copy of the
        actual parameters passed from user-mode.

    TrapFrame - Supplies a pointer to the trap frame generated by this jump
        from user mode to kernel mode.

    ResultSize - Supplies a pointer where the system call routine returns the
        size of the parameter structure to be copied back to user mode. The
        value returned here must be no larger than the original parameter
        structure size. The default is the original size of the parameters.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

VOID
MmFlushInstructionCache (
    PVOID Address,
    UINTN Size
    )

/*++

Routine Description:

    This routine flushes the given cache region and invalidates the
    instruction cache.

Arguments:

    Address - Supplies the address to flush.

    Size - Supplies the number of bytes in the region to flush.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

VOID
MmFlushDataCache (
    PVOID Address,
    UINTN Size,
    BOOL ValidateAddress
    )

/*++

Routine Description:

    This routine flushes the given date cache region.

Arguments:

    Address - Supplies the address to flush.

    Size - Supplies the number of bytes in the region to flush.

    ValidateAddress - Supplies a boolean indicating whether or not to make sure
        the given address is mapped before flushing.

Return Value:

    None.

--*/

{

    //
    // The x86 is cache coherent with all observers of memory.
    //

    return;
}

//
// --------------------------------------------------------- Internal Functions
//
