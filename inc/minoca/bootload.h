/*++

Copyright (c) 2012 Minoca Corp. All Rights Reserved

Module Name:

    bootload.h

Abstract:

    This header contains definitions for the boot loader shared between the
    loader and the kernel, as well as system initialization functions.

Author:

    Evan Green 30-Jul-2012

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <minoca/kdebug.h>
#include <minoca/sysres.h>

//
// ---------------------------------------------------------------- Definitions
//

#define BOOT_INITIALIZATION_BLOCK_VERSION 1

#define KERNEL_INITIALIZATION_BLOCK_VERSION 2

//
// Define the initial size of the memory allocation to hand to the hardware
// module support.
//

#define HARDWARE_MODULE_INITIAL_ALLOCATION_SIZE 0x4000
#define HARDWARE_MODULE_INITIAL_DEVICE_ALLOCATION_SIZE 0x4000

//
// ------------------------------------------------------ Data Type Definitions
//

/*++

Structure Description:

    This structure stores a region of reserved memory that may or may not
    already be marked in the firmware memory map. The boot manager uses these
    descriptors to stake out its own memory in the loader on legacy PC/AT
    systems.

Members:

    Address - Stores the base address of the reserved region.

    Size - Stores the size of the reserved region in bytes.

    Flags - Stores flags describing the region.

--*/

typedef struct _BOOT_RESERVED_REGION {
    ULONGLONG Address;
    ULONGLONG Size;
    ULONGLONG Flags;
} BOOT_RESERVED_REGION, *PBOOT_RESERVED_REGION;

/*++

Structure Description:

    This structure stores the information passed between the boot manager and
    OS loader or other boot application. Future versions of this structure must
    be backwards compatible as newer boot managers may pass control over to
    older OS loaders.

Members:

    Version - Stores the version number of the loader initialization block.
        Set to BOOT_INITIALIZATION_BLOCK_VERSION.

    BootConfigurationFile - Stores a pointer to a buffer containing the
        contents of the boot configuration file.

    BootConfigurationFileSize - Stores the size of the boot configuration file
        buffer in bytes.

    BootEntryId - Stores the identifier of the selected boot entry.

    BootEntryFlags - Stores the flags associated with this boot entry. See
        BOOT_ENTRY_FLAG_* definitions.

    ReservedRegions - Stores a pointer to an array of reserved regions of
        memeory that may or may not be in the firmware memory map.

    ReservedRegionCount - Stores the number of reserved region structures in
        the array.

    StackTop - Stores a pointer to the top of the stack.

    StackSize - Stores the size of the boot stack region, in bytes.

    EfiImageHandle - Stores a pointer to the EFI image handle used to launch
        the boot application that launched this boot application. Note the
        type here is an EFI_HANDLE *, not an EFI_HANDLE.

    EfiSystemTable - Stores a pointer to the EFI system table as passed to the
        original EFI boot application. The type here is an EFI_SYSTEM_TABLE *.

    ApplicationBaseAddress - Stores the loaded base address of the boot
        application.

    PartitionOffset - Stores the offset in blocks from the beginning of the
        disk to the OS partition if the firmware doesn't support partitions
        natively.

    DriveNumber - Stores the drive number of the OS partition for legacy PC/AT
        systems.

    ApplicationName - Stores a pointer to a string containing the file name of
        the application being launched.

    ApplicationLowestAddress - Stores the lowest address of the boot
        application image.

    ApplicationSize - Stores the size of the loaded boot application image in
        bytes.

    ApplicationArguments - Stores a pointer to a null terminated string
        containing the command-line-esque arguments to the application.

--*/

typedef struct _BOOT_INITIALIZATION_BLOCK {
    ULONG Version;
    PVOID BootConfigurationFile;
    ULONG BootConfigurationFileSize;
    ULONG BootEntryId;
    ULONGLONG BootEntryFlags;
    PBOOT_RESERVED_REGION ReservedRegions;
    ULONG ReservedRegionCount;
    PVOID StackTop;
    UINTN StackSize;
    PVOID EfiImageHandle;
    PVOID EfiSystemTable;
    ULONGLONG PartitionOffset;
    ULONG DriveNumber;
    PSTR ApplicationName;
    PVOID ApplicationBaseAddress;
    PVOID ApplicationLowestAddress;
    UINTN ApplicationSize;
    PSTR ApplicationArguments;
} BOOT_INITIALIZATION_BLOCK, *PBOOT_INITIALIZATION_BLOCK;

typedef
INT
(*PBOOT_APPLICATION_ENTRY) (
    PBOOT_INITIALIZATION_BLOCK Parameters
    );

/*++

Routine Description:

    This routine is the entry point into a boot application.

Arguments:

    Parameters - Supplies a pointer to the initialization information.

Return Value:

    0 or does not return on success.

    Returns a non-zero value on failure.

--*/

/*++

Structure Description:

    This structure stores pointers to all of the static tables provided by the
    firmware. An array of virtual addresses is expected to immediately follow
    this structure.

Members:

    TableCount - Supplies the number of tables in the following array.

--*/

typedef struct _FIRMWARE_TABLE_DIRECTORY {
    ULONG TableCount;
} FIRMWARE_TABLE_DIRECTORY, *PFIRMWARE_TABLE_DIRECTORY;

/*++

Structure Description:

    This structure stores information about a file loaded directly into memory
    by the boot loader.

Members:

    File - Stores a pointer to the buffer containing the file.

    FileSize - Stores the size of the file, in bytes.

--*/

typedef struct _LOADER_FILE {
    PVOID File;
    ULONG FileSize;
} LOADER_FILE, *PLOADER_FILE;

/*++

Structure Description:

    This structure stores information needed by the kernel to initialize. It
    is provided by the loader when the kernel is launched.

Members:

    Version - Stores the version number of the loader block. This is used to
        detect version mismatch between the loader and the kernel.

    Size - Stores the total size of the initialization block structure, in
        bytes. This field can also be used to detect mismatch or corruption
        between the loader and the kernel.

    FirmwareTables - Stores a pointer to the directory of static tables
        provided by the platform firmware.

    MemoryMap - Stores a pointer to the memory map of the machine, including
        any regions defined by the firmware, and regions allocated by the
        loader.

    VirtualMap - Stores a pointer to the virtual memory map created for the
        kernel.

    PageDirectory - Stores a pointer to the top level paging structure.

    PageTables - Stores a pointer to the page tables.

    PageTableStage - Stores a pointer to the initial page table staging area.
        The mapping for this virtual does *not* correspond to any valid memory,
        but a page table has been set up for this VA to prevent infinite loops.

    ImageList - Stores the head of the list of images loaded by the kernel.
        Entries on this list are of type LOADED_IMAGE.

    KernelModule - Stores a pointer to the module information for the kernel
        itself. This data should also be in the loaded modules list.

    LoaderModule - Stores a pointer to the module information for the OS
        loader. This data should also be in the loaded modules list.

    KernelStack - Stores a pointer to the top of the kernel stack. The actual
        stack pointer received by the kernel may be less than this due to
        parameters already pushed on by the loader.

    KernelStackSize - Stores the total size of the kernel stack, in bytes.

    DeviceToDriverFile - Stores the location of the file containing the mapping
        between devices and drivers.

    DeviceMapFile - Stores the location of the file containing a list of
        unenumerable devices that exist on the system.

    SystemResourceListHead - Stores the list of system resources provided to
        the kernel by the loader. All system resources begin with a
        SYSTEM_RESOURCE_HEADER.

    TimeZoneData - Stores a pointer to the initial time zone data.

    TimeZoneDataSize - Stores the size of the time zone data in bytes.

    BootEntry - Stores a pointer to the boot entry that was launched.

    BootTime - Stores the boot time of the system.

    FirmwareType - Stores the system firmware type.

    EfiRuntimeServices - Stores a pointer to the EFI runtime services table.
        This is only valid on EFI based systems.

    CycleCounterFrequency - Stores an estimate of the frequency of the cycle
        counter, used for very early stall services. On some architectures or
        platforms this may be 0.

--*/

typedef struct _KERNEL_INITIALIZATION_BLOCK {
    ULONG Version;
    ULONG Size;
    PFIRMWARE_TABLE_DIRECTORY FirmwareTables;
    PMEMORY_DESCRIPTOR_LIST MemoryMap;
    PMEMORY_DESCRIPTOR_LIST VirtualMap;
    PVOID PageDirectory;
    PVOID PageTables;
    PVOID PageTableStage;
    LIST_ENTRY ImageList;
    PLOADED_MODULE KernelModule;
    PLOADED_MODULE LoaderModule;
    PVOID KernelStack;
    ULONG KernelStackSize;
    LOADER_FILE DeviceToDriverFile;
    LOADER_FILE DeviceMapFile;
    LIST_ENTRY SystemResourceListHead;
    PVOID TimeZoneData;
    ULONG TimeZoneDataSize;
    PVOID BootEntry;
    SYSTEM_TIME BootTime;
    SYSTEM_FIRMWARE_TYPE FirmwareType;
    PVOID EfiRuntimeServices;
    ULONGLONG CycleCounterFrequency;
} KERNEL_INITIALIZATION_BLOCK, *PKERNEL_INITIALIZATION_BLOCK;

/*++

Structure Description:

    This structure stores information needed by an application processor to
    initialize.

    Warning: The offsets of this structure are also used by assembly code, so
             be very careful adding, deleting, or moving members of this
             structure.

Members:

    StackBase - Supplies the base of the stack that the initialization is
        running on.

    StackSize - Supplies the size of the stack that the initialization is
        running on.

    ProcessorNumber - Supplies the number of the processor.

    ProcessorStructures - Supplies the processor structures buffer used for
        early architecture specific initialization.

    SwapPage - Supplies a pointer to the virtual address reservation the
        processor should use for quick dispatch level mappings.

--*/

typedef struct _PROCESSOR_START_BLOCK {
    PVOID StackBase;
    ULONG StackSize;
    ULONG ProcessorNumber;
    PVOID ProcessorStructures;
    PVOID SwapPage;
} PACKED PROCESSOR_START_BLOCK, *PPROCESSOR_START_BLOCK;

//
// -------------------------------------------------------------------- Globals
//

//
// -------------------------------------------------------- Function Prototypes
//

VOID
AcpiInitializePreDebugger (
    PKERNEL_INITIALIZATION_BLOCK Parameters
    );

/*++

Routine Description:

    This routine pre-initializes ACPI to the extent that the debugger requires
    it. This routine is *undebuggable* as it is called before debug services
    are online.

Arguments:

    Parameters - Supplies the kernel parameter block coming from the loader.

Return Value:

    None.

--*/

KSTATUS
AcpiInitialize (
    PKERNEL_INITIALIZATION_BLOCK Parameters
    );

/*++

Routine Description:

    This routine initializes ACPI.

Arguments:

    Parameters - Supplies the kernel parameter block coming from the loader.

Return Value:

    Status code.

--*/

KSTATUS
MmInitialize (
    PKERNEL_INITIALIZATION_BLOCK Parameters,
    PPROCESSOR_START_BLOCK StartBlock,
    ULONG Phase
    );

/*++

Routine Description:

    This routine initializes the kernel Memory Manager.

Arguments:

    Parameters - Supplies a pointer to the initialization block from the loader.

    StartBlock - Supplies a pointer to the processor start block if this is an
        application processor.

    Phase - Supplies the phase of initialization. Valid values are 0 through 4.

Return Value:

    Status code.

--*/

KSTATUS
MmPrepareForProcessorLaunch (
    PPROCESSOR_START_BLOCK StartBlock
    );

/*++

Routine Description:

    This routine initializes a processor start block in preparation for
    launching a new processor.

Arguments:

    StartBlock - Supplies a pointer to the start block that will be passed to
        the new core.

Return Value:

    Status code.

--*/

VOID
MmDestroyProcessorStartBlock (
    PPROCESSOR_START_BLOCK StartBlock
    );

/*++

Routine Description:

    This routine destroys structures initialized by MM in preparation for a
    (now failed) processor launch.

Arguments:

    StartBlock - Supplies a pointer to the start block.

Return Value:

    None.

--*/

KSTATUS
KeInitialize (
    ULONG Phase,
    PKERNEL_INITIALIZATION_BLOCK Parameters
    );

/*++

Routine Description:

    This routine initializes the Kernel Executive subsystem.

Arguments:

    Phase - Supplies the initialization phase. Valid values are 0 through 3.

    Parameters - Supplies a pointer to the kernel initialization block.

Return Value:

    Status code.

--*/

PPROCESSOR_START_BLOCK
KePrepareForProcessorLaunch (
    VOID
    );

/*++

Routine Description:

    This routine prepares the kernel's internal structures for a new processor
    coming online.

Arguments:

    None.

Return Value:

    Returns a pointer to an allocated and filled out processor start block
    structure. At this point the kernel will be ready for this processor to
    come online at any time.

    NULL on failure.

--*/

VOID
KeFreeProcessorStartBlock (
    PPROCESSOR_START_BLOCK StartBlock,
    BOOL FreeResourcesInside
    );

/*++

Routine Description:

    This routine frees a processor start block structure.

Arguments:

    StartBlock - Supplies a pointer to the start block structure to free.

    FreeResourcesInside - Supplies a boolean indicating whether or not to free
        the resources contained inside the start block.

Return Value:

    None.

--*/

KSTATUS
PsInitialize (
    ULONG Phase,
    PKERNEL_INITIALIZATION_BLOCK Parameters,
    PVOID IdleThreadStackBase,
    ULONG IdleThreadStackSize
    );

/*++

Routine Description:

    This routine initializes the process and thread subsystem.

Arguments:

    Phase - Supplies the initialization phase. Valid values are 0 and 1.

    Parameters - Supplies an optional pointer to the kernel initialization
        block. It's only required for processor 0.

    IdleThreadStackBase - Supplies the base of the stack for the one thread
        currently running.

    IdleThreadStackSize - Supplies the size of the stack for the one thread
        currently running.

Return Value:

    STATUS_SUCCESS on success.

    STATUS_INSUFFICIENT_RESOURCES if memory for the kernel process or thread
        could not be allocated.

--*/

KSTATUS
IoInitialize (
    ULONG Phase,
    PKERNEL_INITIALIZATION_BLOCK Parameters
    );

/*++

Routine Description:

    This routine initializes the I/O subsystem.

Arguments:

    Phase - Supplies the initialization phase.

    Parameters - Supplies a pointer to the kernel's initialization information.

Return Value:

    Status code.

--*/

VOID
HlInitializePreDebugger (
    PKERNEL_INITIALIZATION_BLOCK Parameters,
    ULONG Processor,
    PDEBUG_DEVICE_DESCRIPTION *DebugDevice
    );

/*++

Routine Description:

    This routine implements extremely early hardware layer initialization. This
    routine is *undebuggable*, as it is called before the debugger is brought
    online.

Arguments:

    Parameters - Supplies an optional pointer to the kernel initialization
        parameters. This parameter may be NULL.

    Processor - Supplies the processor index of this processor.

    DebugDevice - Supplies a pointer where a pointer to the debug device
        description will be returned on success.

Return Value:

    None.

--*/

KSTATUS
HlInitialize (
    PKERNEL_INITIALIZATION_BLOCK Parameters,
    ULONG Phase
    );

/*++

Routine Description:

    This routine initializes the core system hardware. During phase 0, on
    application processors, this routine enters at low run level and exits at
    dispatch run level.

Arguments:

    Parameters - Supplies a pointer to the kernel's initialization information.

    Phase - Supplies the initialization phase.

Return Value:

    Status code.

--*/
