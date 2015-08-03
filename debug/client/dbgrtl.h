/*++

Copyright (c) 2013 Minoca Corp. All Rights Reserved

Module Name:

    dbgrtl.h

Abstract:

    This header contains definitions for the runtime library used in the
    debugger client.

Author:

    Evan Green 19-Feb-2013

--*/

//
// ------------------------------------------------------------------- Includes
//

//
// Define the RTL library functions as exports since they're statically linked
// in here.
//

#define RTL_API DLLEXPORT

#include <minoca/types.h>
#include <minoca/status.h>
#include <minoca/rtl.h>

//
// ---------------------------------------------------------------- Definitions
//

//
// ------------------------------------------------------ Data Type Definitions
//

//
// -------------------------------------------------------------------- Globals
//

//
// -------------------------------------------------------- Function Prototypes
//