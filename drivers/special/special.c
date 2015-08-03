/*++

Copyright (c) 2012 Minoca Corp. All Rights Reserved

Module Name:

    special.c

Abstract:

    This module implements the special file driver.

Author:

    Evan Green 23-Sep-2013

Environment:

    Kernel

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <minoca/driver.h>
#include <minoca/intrface/random.h>
#include <minoca/crypto.h>

//
// ---------------------------------------------------------------- Definitions
//

#define SPECIAL_DEVICE_ALLOCATION_TAG 0x76447053 // 'vDpS'

#define SPECIAL_DEVICE_NULL_NAME "null"
#define SPECIAL_DEVICE_ZERO_NAME "zero"
#define SPECIAL_DEVICE_FULL_NAME "full"
#define SPECIAL_DEVICE_RANDOM_NAME "random"
#define SPECIAL_DEVICE_URANDOM_NAME "urandom"

#define SPECIAL_URANDOM_BUFFER_SIZE 2048

//
// ------------------------------------------------------ Data Type Definitions
//

typedef enum _SPECIAL_DEVICE_TYPE {
    SpecialDeviceInvalid,
    SpecialDeviceNull,
    SpecialDeviceZero,
    SpecialDeviceFull,
    SpecialDevicePseudoRandom
} SPECIAL_DEVICE_TYPE, *PSPECIAL_DEVICE_TYPE;

/*++

Structure Description:

    This structure defines the context for a special device.

Members:

    Type - Stores the type of device this is representing.

    CreationTime - Stores the system time when the device was created.

    ReferenceCount - Stores the number of references held on the device.

    FortunaContext - Stores a pointer to the fortuna context.

    Lock - Stores the lock protecting the Fortuna context.

    Interface - Stores a pointer to the pseudo-random source interface.

    InterfaceRegistered - Stores a pointer indicating whether or not the
        interface has been registered.

--*/

typedef struct _SPECIAL_DEVICE {
    SPECIAL_DEVICE_TYPE Type;
    SYSTEM_TIME CreationTime;
    volatile ULONG ReferenceCount;
    PFORTUNA_CONTEXT FortunaContext;
    KSPIN_LOCK Lock;
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface;
    BOOL InterfaceRegistered;
} SPECIAL_DEVICE, *PSPECIAL_DEVICE;

//
// ----------------------------------------------- Internal Function Prototypes
//

KSTATUS
SpecialAddDevice (
    PVOID Driver,
    PSTR DeviceId,
    PSTR ClassId,
    PSTR CompatibleIds,
    PVOID DeviceToken
    );

VOID
SpecialDispatchStateChange (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
SpecialDispatchOpen (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
SpecialDispatchClose (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
SpecialDispatchIo (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
SpecialDispatchSystemControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
SpecialDispatchUserControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

KSTATUS
SpecialFillZeroes (
    PIRP Irp
    );

KSTATUS
SpecialPseudoRandomStartDevice (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    );

KSTATUS
SpecialPseudoRandomRemoveDevice (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    );

KSTATUS
SpecialPerformPseudoRandomIo (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    );

VOID
SpecialPseudoRandomAddEntropy (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface,
    PVOID Data,
    UINTN Length
    );

VOID
SpecialPseudoRandomAddTimePointEntropy (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface
    );

VOID
SpecialPseudoRandomGetBytes (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface,
    PVOID Data,
    UINTN Length
    );

VOID
SpecialDeviceAddReference (
    PSPECIAL_DEVICE Device
    );

VOID
SpecialDeviceReleaseReference (
    PSPECIAL_DEVICE Device
    );

VOID
SpecialDestroyDevice (
    PSPECIAL_DEVICE Device
    );

//
// -------------------------------------------------------------------- Globals
//

PDRIVER SpecialDriver = NULL;

INTERFACE_PSEUDO_RANDOM_SOURCE SpecialPseudoRandomInterfaceTemplate = {
    NULL,
    SpecialPseudoRandomAddEntropy,
    SpecialPseudoRandomAddTimePointEntropy,
    SpecialPseudoRandomGetBytes
};

UUID SpecialPseudoRandomInterfaceUuid = UUID_PSEUDO_RANDOM_SOURCE_INTERFACE;

//
// ------------------------------------------------------------------ Functions
//

KSTATUS
DriverEntry (
    PDRIVER Driver
    )

/*++

Routine Description:

    This routine is the entry point for the null driver. It registers its other
    dispatch functions, and performs driver-wide initialization.

Arguments:

    Driver - Supplies a pointer to the driver object.

Return Value:

    STATUS_SUCCESS on success.

    Failure code on error.

--*/

{

    DRIVER_FUNCTION_TABLE FunctionTable;
    KSTATUS Status;

    SpecialDriver = Driver;
    RtlZeroMemory(&FunctionTable, sizeof(DRIVER_FUNCTION_TABLE));
    FunctionTable.Version = DRIVER_FUNCTION_TABLE_VERSION;
    FunctionTable.AddDevice = SpecialAddDevice;
    FunctionTable.DispatchStateChange = SpecialDispatchStateChange;
    FunctionTable.DispatchOpen = SpecialDispatchOpen;
    FunctionTable.DispatchClose = SpecialDispatchClose;
    FunctionTable.DispatchIo = SpecialDispatchIo;
    FunctionTable.DispatchSystemControl = SpecialDispatchSystemControl;
    FunctionTable.DispatchUserControl = SpecialDispatchUserControl;
    Status = IoRegisterDriverFunctions(Driver, &FunctionTable);
    return Status;
}

KSTATUS
SpecialAddDevice (
    PVOID Driver,
    PSTR DeviceId,
    PSTR ClassId,
    PSTR CompatibleIds,
    PVOID DeviceToken
    )

/*++

Routine Description:

    This routine is called when a device is detected for which the null device
    acts as the function driver. The driver will attach itself to the stack.

Arguments:

    Driver - Supplies a pointer to the driver being called.

    DeviceId - Supplies a pointer to a string with the device ID.

    ClassId - Supplies a pointer to a string containing the device's class ID.

    CompatibleIds - Supplies a pointer to a string containing device IDs
        that would be compatible with this device.

    DeviceToken - Supplies an opaque token that the driver can use to identify
        the device in the system. This token should be used when attaching to
        the stack.

Return Value:

    STATUS_SUCCESS on success.

    Failure code if the driver was unsuccessful in attaching itself.

--*/

{

    UINTN AllocationSize;
    PSPECIAL_DEVICE Context;
    SPECIAL_DEVICE_TYPE DeviceType;
    KSTATUS Status;

    Context = NULL;
    if (IoAreDeviceIdsEqual(DeviceId, SPECIAL_DEVICE_NULL_NAME) != FALSE) {
        DeviceType = SpecialDeviceNull;

    } else if (IoAreDeviceIdsEqual(DeviceId, SPECIAL_DEVICE_ZERO_NAME) !=
                                                                       FALSE) {

        DeviceType = SpecialDeviceZero;

    } else if (IoAreDeviceIdsEqual(DeviceId, SPECIAL_DEVICE_FULL_NAME) !=
                                                                       FALSE) {

        DeviceType = SpecialDeviceFull;

    } else if (IoAreDeviceIdsEqual(DeviceId, SPECIAL_DEVICE_URANDOM_NAME) !=
                                                                       FALSE) {

        DeviceType = SpecialDevicePseudoRandom;

    //
    // Random and urandom are the same. Convincing arguments have been made
    // that trying to estimate the amount of entropy in a source (and therefore
    // block random until there is enough) is perilous.
    //

    } else if (IoAreDeviceIdsEqual(DeviceId, SPECIAL_DEVICE_RANDOM_NAME) !=
                                                                       FALSE) {

        DeviceType = SpecialDevicePseudoRandom;

    } else {
        RtlDebugPrint("Special device %s not recognized.\n", DeviceId);
        Status = STATUS_NOT_SUPPORTED;
        goto AddDeviceEnd;
    }

    //
    // The urandom special device must be created non-paged as entropy can be
    // added from dispatch level.
    //

    if (DeviceType == SpecialDevicePseudoRandom) {
        AllocationSize = sizeof(SPECIAL_DEVICE) + sizeof(FORTUNA_CONTEXT) +
                         sizeof(INTERFACE_PSEUDO_RANDOM_SOURCE);

        Context = MmAllocateNonPagedPool(AllocationSize,
                                         SPECIAL_DEVICE_ALLOCATION_TAG);

        if (Context == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto AddDeviceEnd;
        }

        RtlZeroMemory(Context, AllocationSize);
        Context->FortunaContext = (PFORTUNA_CONTEXT)(Context + 1);
        CyFortunaInitialize(Context->FortunaContext,
                            HlQueryTimeCounter,
                            HlQueryTimeCounterFrequency());

        KeInitializeSpinLock(&(Context->Lock));
        Context->Interface =
                (PINTERFACE_PSEUDO_RANDOM_SOURCE)(Context->FortunaContext + 1);

        RtlCopyMemory(Context->Interface,
                      &SpecialPseudoRandomInterfaceTemplate,
                      sizeof(INTERFACE_PSEUDO_RANDOM_SOURCE));

        Context->Interface->DeviceToken = Context;

    //
    // Create a regular special device.
    //

    } else {
        AllocationSize = sizeof(SPECIAL_DEVICE);
        Context = MmAllocatePagedPool(AllocationSize,
                                      SPECIAL_DEVICE_ALLOCATION_TAG);

        if (Context == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto AddDeviceEnd;
        }

        RtlZeroMemory(Context, AllocationSize);
    }

    Context->Type = DeviceType;
    Context->ReferenceCount = 1;
    KeGetSystemTime(&(Context->CreationTime));
    Status = IoAttachDriverToDevice(Driver, DeviceToken, Context);
    if (!KSUCCESS(Status)) {
        goto AddDeviceEnd;
    }

AddDeviceEnd:
    if (!KSUCCESS(Status)) {
        if (Context != NULL) {
            SpecialDeviceReleaseReference(Context);
        }
    }

    return Status;
}

VOID
SpecialDispatchStateChange (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles State Change IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;
    KSTATUS Status;

    ASSERT(Irp->MajorCode == IrpMajorStateChange);

    Device = (PSPECIAL_DEVICE)DeviceContext;
    switch (Irp->MinorCode) {
    case IrpMinorQueryResources:
        if (Irp->Direction == IrpUp) {
            IoCompleteIrp(SpecialDriver, Irp, STATUS_SUCCESS);
        }

        break;

    case IrpMinorStartDevice:
        if (Irp->Direction == IrpUp) {
            if (Device->Type == SpecialDevicePseudoRandom) {
                Status = SpecialPseudoRandomStartDevice(Device, Irp);

            } else {
                Status = STATUS_SUCCESS;
            }

            IoCompleteIrp(SpecialDriver, Irp, Status);
        }

        break;

    case IrpMinorQueryChildren:
        IoCompleteIrp(SpecialDriver, Irp, STATUS_SUCCESS);
        break;

    case IrpMinorRemoveDevice:
        if (Irp->Direction == IrpUp) {
            if (Device->Type == SpecialDevicePseudoRandom) {
                Status = SpecialPseudoRandomRemoveDevice(Device, Irp);

            } else {
                Status = STATUS_SUCCESS;
            }

            if (KSUCCESS(Status)) {
                SpecialDeviceReleaseReference(Device);
            }

            IoCompleteIrp(SpecialDriver, Irp, Status);
        }

        break;

    //
    // For all other IRPs, do nothing.
    //

    default:
        break;
    }

    return;
}

VOID
SpecialDispatchOpen (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles Open IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;

    Device = (PSPECIAL_DEVICE)DeviceContext;
    SpecialDeviceAddReference(Device);

    ASSERT(Irp->U.Open.IoState != NULL);

    //
    // The data sink devices are always ready for I/O.
    //

    IoSetIoObjectState(Irp->U.Open.IoState,
                       POLL_EVENT_IN | POLL_EVENT_OUT,
                       TRUE);

    IoCompleteIrp(SpecialDriver, Irp, STATUS_SUCCESS);
    return;
}

VOID
SpecialDispatchClose (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles Close IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;

    Device = (PSPECIAL_DEVICE)DeviceContext;
    SpecialDeviceReleaseReference(Device);
    IoCompleteIrp(SpecialDriver, Irp, STATUS_SUCCESS);
    return;
}

VOID
SpecialDispatchIo (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles I/O IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;
    KSTATUS Status;

    ASSERT(Irp->MajorCode == IrpMajorIo);
    ASSERT(Irp->Direction == IrpDown);

    Device = (PSPECIAL_DEVICE)DeviceContext;
    switch (Device->Type) {

    //
    // The null device accepts and discards all input, and produces no output.
    //

    case SpecialDeviceNull:
        if (Irp->MinorCode == IrpMinorIoRead) {
            Irp->U.ReadWrite.IoBytesCompleted = 0;

        } else {

            ASSERT(Irp->MinorCode == IrpMinorIoWrite);

            Irp->U.ReadWrite.IoBytesCompleted = Irp->U.ReadWrite.IoSizeInBytes;
        }

        Status = STATUS_SUCCESS;
        break;

    //
    // The zero device accepts and discards all input, and produces a
    // continuous stream of zero bytes.
    //

    case SpecialDeviceZero:
        if (Irp->MinorCode == IrpMinorIoRead) {
            Status = SpecialFillZeroes(Irp);

        } else {

            ASSERT(Irp->MinorCode == IrpMinorIoWrite);

            Irp->U.ReadWrite.IoBytesCompleted = Irp->U.ReadWrite.IoSizeInBytes;
            Status = STATUS_SUCCESS;
        }

        break;

    //
    // The full device produces a continuous stream of zero bytes when read,
    // and returns "disk full" when written to.
    //

    case SpecialDeviceFull:
        if (Irp->MinorCode == IrpMinorIoRead) {
            Status = SpecialFillZeroes(Irp);

        } else {

            ASSERT(Irp->MinorCode == IrpMinorIoWrite);

            Status = STATUS_VOLUME_FULL;
        }

        break;

    //
    // The urandom device produces psuedo-random numbers when read, and adds
    // entropy when written to.
    //

    case SpecialDevicePseudoRandom:
        Status = SpecialPerformPseudoRandomIo(Device, Irp);
        break;

    default:

        ASSERT(FALSE);

        Status = STATUS_FILE_CORRUPT;
    }

    IoCompleteIrp(SpecialDriver, Irp, Status);
    return;
}

VOID
SpecialDispatchSystemControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles System Control IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PVOID Context;
    PSPECIAL_DEVICE Device;
    PSYSTEM_CONTROL_LOOKUP Lookup;
    PFILE_PROPERTIES Properties;
    KSTATUS Status;

    Device = (PSPECIAL_DEVICE)DeviceContext;
    Context = Irp->U.SystemControl.SystemContext;
    switch (Irp->MinorCode) {
    case IrpMinorSystemControlLookup:
        Lookup = (PSYSTEM_CONTROL_LOOKUP)Context;
        Status = STATUS_PATH_NOT_FOUND;
        if (Lookup->Root != FALSE) {

            //
            // Enable opening of the root as a single file.
            //

            Properties = &(Lookup->Properties);
            Properties->FileId = 0;
            Properties->Type = IoObjectCharacterDevice;
            Properties->HardLinkCount = 1;
            Properties->BlockSize = 1;
            Properties->BlockCount = 0;
            Properties->StatusChangeTime = Device->CreationTime;
            Properties->ModifiedTime = Properties->StatusChangeTime;
            Properties->AccessTime = Properties->StatusChangeTime;
            Properties->Permissions = FILE_PERMISSION_ALL;
            WRITE_INT64_SYNC(&(Properties->FileSize), 0);
            Status = STATUS_SUCCESS;
        }

        IoCompleteIrp(SpecialDriver, Irp, Status);
        break;

    //
    // Succeed for the basics.
    //

    case IrpMinorSystemControlWriteFileProperties:
    case IrpMinorSystemControlTruncate:
        Status = STATUS_SUCCESS;
        IoCompleteIrp(SpecialDriver, Irp, Status);
        break;

    //
    // Ignore everything unrecognized.
    //

    default:

        ASSERT(FALSE);

        break;
    }

    return;
}

VOID
SpecialDispatchUserControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles User Control IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    return;
}

KSTATUS
SpecialFillZeroes (
    PIRP Irp
    )

/*++

Routine Description:

    This routine fills a read buffer with zeroes.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

Return Value:

    Status code.

--*/

{

    KSTATUS Status;

    ASSERT(Irp->MinorCode == IrpMinorIoRead);
    ASSERT(Irp->U.ReadWrite.IoBuffer != NULL);

    Status = MmZeroIoBuffer(Irp->U.ReadWrite.IoBuffer,
                            0,
                            Irp->U.ReadWrite.IoSizeInBytes);

    if (!KSUCCESS(Status)) {
        return Status;
    }

    Irp->U.ReadWrite.IoBytesCompleted = Irp->U.ReadWrite.IoSizeInBytes;
    return STATUS_SUCCESS;
}

KSTATUS
SpecialPseudoRandomStartDevice (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    )

/*++

Routine Description:

    This routine starts a urandom device.

Arguments:

    Device - Supplies a pointer to the special device context.

    Irp - Supplies a pointer to the request.

Return Value:

    Status code.

--*/

{

    KSTATUS Status;

    ASSERT(Device->Type == SpecialDevicePseudoRandom);

    if (Device->InterfaceRegistered != FALSE) {
        return STATUS_SUCCESS;
    }

    Status = IoCreateInterface(&SpecialPseudoRandomInterfaceUuid,
                               Irp->Device,
                               Device->Interface,
                               sizeof(INTERFACE_PSEUDO_RANDOM_SOURCE));

    if (KSUCCESS(Status)) {
        Device->InterfaceRegistered = TRUE;
    }

    //
    // Seed the generator with at least this somewhat random point in time.
    //

    SpecialPseudoRandomAddTimePointEntropy(Device->Interface);
    return Status;
}

KSTATUS
SpecialPseudoRandomRemoveDevice (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    )

/*++

Routine Description:

    This routine stops a urandom device.

Arguments:

    Device - Supplies a pointer to the special device context.

    Irp - Supplies a pointer to the request.

Return Value:

    Status code.

--*/

{

    KSTATUS Status;

    ASSERT(Device->Type == SpecialDevicePseudoRandom);

    if (Device->InterfaceRegistered == FALSE) {
        return STATUS_SUCCESS;
    }

    Status = IoDestroyInterface(&SpecialPseudoRandomInterfaceUuid,
                                Irp->Device,
                                Device->Interface);

    if (KSUCCESS(Status)) {
        Device->InterfaceRegistered = FALSE;
    }

    return Status;
}

KSTATUS
SpecialPerformPseudoRandomIo (
    PSPECIAL_DEVICE Device,
    PIRP Irp
    )

/*++

Routine Description:

    This routine fills a buffer with random data, or adds entropy to the pools.

Arguments:

    Device - Supplies a pointer to the special device context.

    Irp - Supplies a pointer to the I/O request packet.

Return Value:

    Status code.

--*/

{

    PVOID Buffer;
    UINTN BytesRemaining;
    PFORTUNA_CONTEXT Fortuna;
    PIO_BUFFER IoBuffer;
    UINTN IoBufferOffset;
    RUNLEVEL OldRunLevel;
    UINTN Size;
    KSTATUS Status;

    ASSERT(Irp->U.ReadWrite.IoBuffer != NULL);

    IoBuffer = Irp->U.ReadWrite.IoBuffer;
    IoBufferOffset = 0;
    BytesRemaining = Irp->U.ReadWrite.IoSizeInBytes;

    //
    // Allocate a non-paged buffer because acquiring the lock raises to
    // dispatch level, since entropy can be added at dispatch.
    //

    Buffer = MmAllocateNonPagedPool(SPECIAL_URANDOM_BUFFER_SIZE,
                                    SPECIAL_DEVICE_ALLOCATION_TAG);

    if (Buffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto PerformPseudoRandomIoEnd;
    }

    Fortuna = Device->FortunaContext;
    while (BytesRemaining != 0) {
        Size = SPECIAL_URANDOM_BUFFER_SIZE;
        if (Size > BytesRemaining) {
            Size = BytesRemaining;
        }

        if (Irp->MinorCode == IrpMinorIoWrite) {
            Status = MmCopyIoBufferData(IoBuffer,
                                        Buffer,
                                        IoBufferOffset,
                                        Size,
                                        FALSE);

            if (!KSUCCESS(Status)) {
                goto PerformPseudoRandomIoEnd;
            }

            OldRunLevel = KeRaiseRunLevel(RunLevelDispatch);
            KeAcquireSpinLock(&(Device->Lock));
            CyFortunaAddEntropy(Fortuna, Buffer, Size);
            KeReleaseSpinLock(&(Device->Lock));
            KeLowerRunLevel(OldRunLevel);

        } else {
            OldRunLevel = KeRaiseRunLevel(RunLevelDispatch);
            KeAcquireSpinLock(&(Device->Lock));
            CyFortunaGetRandomBytes(Fortuna, Buffer, Size);
            KeReleaseSpinLock(&(Device->Lock));
            KeLowerRunLevel(OldRunLevel);
            Status = MmCopyIoBufferData(IoBuffer,
                                        Buffer,
                                        IoBufferOffset,
                                        Size,
                                        TRUE);

            if (!KSUCCESS(Status)) {
                goto PerformPseudoRandomIoEnd;
            }
        }

        BytesRemaining -= Size;
        IoBufferOffset += Size;
    }

    Status = STATUS_SUCCESS;

PerformPseudoRandomIoEnd:
    if (Buffer != NULL) {
        MmFreeNonPagedPool(Buffer);
    }

    Irp->U.ReadWrite.IoBytesCompleted = Irp->U.ReadWrite.IoSizeInBytes -
                                        BytesRemaining;

    return Status;
}

VOID
SpecialPseudoRandomAddEntropy (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface,
    PVOID Data,
    UINTN Length
    )

/*++

Routine Description:

    This routine adds entropy to a pseudo-random device. This function can be
    called at or below dispatch level.

Arguments:

    Interface - Supplies a pointer to the interface instance.

    Data - Supplies a pointer to the entropy data to add. This data must be
        non-paged.

    Length - Supplies the number of bytes in the data.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;
    RUNLEVEL OldRunLevel;

    Device = Interface->DeviceToken;

    ASSERT(Device->Type == SpecialDevicePseudoRandom);

    OldRunLevel = KeRaiseRunLevel(RunLevelDispatch);
    KeAcquireSpinLock(&(Device->Lock));
    CyFortunaAddEntropy(Device->FortunaContext, Data, Length);
    KeReleaseSpinLock(&(Device->Lock));
    KeLowerRunLevel(OldRunLevel);
    return;
}

VOID
SpecialPseudoRandomAddTimePointEntropy (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface
    )

/*++

Routine Description:

    This routine adds entropy to a pseudo-random device based on the fact that
    this current moment in time is a random one. Said differently, it adds
    entropy based on the current timestamp, with the assumption that this
    function is called by a source that generates such events randomly. This
    function can be called at or below dispatch level.

Arguments:

    Interface - Supplies a pointer to the interface instance.

Return Value:

    None.

--*/

{

    ULONGLONG Counter;
    PSPECIAL_DEVICE Device;
    RUNLEVEL OldRunLevel;

    Device = Interface->DeviceToken;

    ASSERT(Device->Type == SpecialDevicePseudoRandom);

    OldRunLevel = KeRaiseRunLevel(RunLevelDispatch);
    Counter = HlQueryProcessorCounter();
    KeAcquireSpinLock(&(Device->Lock));
    CyFortunaAddEntropy(Device->FortunaContext, &Counter, sizeof(ULONGLONG));
    KeReleaseSpinLock(&(Device->Lock));
    KeLowerRunLevel(OldRunLevel);
    return;
}

VOID
SpecialPseudoRandomGetBytes (
    PINTERFACE_PSEUDO_RANDOM_SOURCE Interface,
    PVOID Data,
    UINTN Length
    )

/*++

Routine Description:

    This routine gets random data from a pseudo-random number generator. This
    function can be called at or below dispatch level.

Arguments:

    Interface - Supplies a pointer to the interface instance.

    Data - Supplies a pointer where the random data will be returned. This
        buffer must be non-paged.

    Length - Supplies the number of bytes of random data to return.

Return Value:

    None.

--*/

{

    PSPECIAL_DEVICE Device;
    RUNLEVEL OldRunLevel;

    Device = Interface->DeviceToken;

    ASSERT(Device->Type == SpecialDevicePseudoRandom);

    OldRunLevel = KeRaiseRunLevel(RunLevelDispatch);
    KeAcquireSpinLock(&(Device->Lock));
    CyFortunaGetRandomBytes(Device->FortunaContext, Data, Length);
    KeReleaseSpinLock(&(Device->Lock));
    KeLowerRunLevel(OldRunLevel);
    return;
}

VOID
SpecialDeviceAddReference (
    PSPECIAL_DEVICE Device
    )

/*++

Routine Description:

    This routine adds a reference on a special device.

Arguments:

    Device - Supplies a pointer to a special device.

Return Value:

    None.

--*/

{

    ULONG OldReferenceCount;

    OldReferenceCount = RtlAtomicAdd32(&(Device->ReferenceCount), 1);

    ASSERT(OldReferenceCount < 0x10000000);

    return;
}

VOID
SpecialDeviceReleaseReference (
    PSPECIAL_DEVICE Device
    )

/*++

Routine Description:

    This routine releases a reference on a special device.

Arguments:

    Device - Supplies a pointer to a special device.

Return Value:

    None.

--*/

{

    ULONG OldReferenceCount;

    OldReferenceCount = RtlAtomicAdd32(&(Device->ReferenceCount), (ULONG)-1);

    ASSERT((OldReferenceCount != 0) && (OldReferenceCount < 0x10000000));

    if (OldReferenceCount == 1) {
        SpecialDestroyDevice(Device);
    }

    return;
}

VOID
SpecialDestroyDevice (
    PSPECIAL_DEVICE Device
    )

/*++

Routine Description:

    This routine destroys a special device.

Arguments:

    Device - Supplies a pointer to a special device.

Return Value:

    None.

--*/

{

    ASSERT(Device->InterfaceRegistered == FALSE);

    if (Device->Type == SpecialDevicePseudoRandom) {
        MmFreeNonPagedPool(Device);

    } else {
        MmFreePagedPool(Device);
    }

    return;
}
