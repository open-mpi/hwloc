/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_WINDOWS_H
#define LIBTOPOLOGY_WINDOWS_H

#define DECLARE_HANDLE(n) typedef struct n##__ {int i;} *n
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
typedef int WINBOOL, BOOL;
typedef double LONGLONG, DWORDLONG, ULONGLONG;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long ULONG_PTR, DWORD_PTR, DWORD, *PDWORD;
typedef const char *LPCSTR;
typedef int (*FARPROC)();
typedef void *PVOID;
typedef PVOID HANDLE;

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

#endif /* LIBTOPOLOGY_WINDOWS_H */
