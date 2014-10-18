#ifndef STR_H_
#define STR_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef UNICODE

#define _tcscpy wcscpy 
#define _tcscmp wcscmp
#define _tchar wchar_t
#define _tfopen _wfopen
#define _tstat _wstat
#define _tstrlen wcslen
#define _tcscspn wcscspn

#else

#define _tcscpy strcpy
#define _tcscmp strcmp
#define _tchar char
#define _tfopen fopen
#define _tstat stat
#define _tstrlen strlen
#define _tcscspn strcspn
#endif


extern const _tchar *skip_space(const _tchar *start, const _tchar *end, const _tchar *s);


#endif