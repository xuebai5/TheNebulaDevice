#ifndef N_FILETIME_H
#define N_FILETIME_H
//------------------------------------------------------------------------------
/**
    @class nFileTime

    Encapsulate a file time stamp. See nFile class for details.
    
    (C) 2003 RadonLabs GmbH
*/
#ifdef __WIN32__
#include "kernel/_nfiletime_win32.h"

typedef _nFileTimeWin32 nFileTime;

#elif defined(__LINUX__)
#include "kernel/_nfiletime_posix.h"

typedef _nFileTimePosix nFileTime;

#else
#error "nFileTime not implemented for this platform"
#endif

//------------------------------------------------------------------------------
#endif
