/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#ifndef LIBTOPOLOGY_WINDOWS_H
#define LIBTOPOLOGY_WINDOWS_H

#include <inttypes.h>

#define DECLARE_HANDLE(n) typedef struct n##__ {int i;} *n
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
typedef int WINBOOL, BOOL;
typedef int64_t LONGLONG;
typedef uint64_t DWORDLONG;
typedef DWORDLONG ULONGLONG, *PULONGLONG;
typedef unsigned char BYTE, UCHAR;
typedef unsigned short WORD, USHORT;
typedef unsigned long ULONG_PTR, DWORD_PTR, DWORD, *PDWORD;
typedef const char *LPCSTR;
typedef int (*FARPROC)();
typedef void *PVOID;

// This is to cope with linux using integers for topo_pid_t and topo_thread_t
//typedef PVOID HANDLE;
typedef int HANDLE;

#ifdef __GNUC__
#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__
#else
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#endif /* __GNUC__ */
#define DUMMYUNIONNAME
#define WINAPI

#define ANYSIZE_ARRAY 1

#define ERROR_INSUFFICIENT_BUFFER 122L

WINAPI HINSTANCE LoadLibrary(LPCSTR);
WINAPI FARPROC GetProcAddress(HINSTANCE, LPCSTR);
WINAPI DWORD GetLastError(void);

DWORD_PTR WINAPI SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask);
BOOL WINAPI SetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask);

HANDLE WINAPI GetCurrentThread(void);
HANDLE WINAPI GetCurrentProcess(void);

BOOL GetNumaAvailableMemoryNode(UCHAR Node, PULONGLONG AvailableBytes);


#endif /* LIBTOPOLOGY_WINDOWS_H */
