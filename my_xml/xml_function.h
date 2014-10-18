#ifndef _XML_FUNCTION_H
#define _XML_FUNCTION_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "array.h"
#include "str.h"


#ifdef UNICODE
	#define _tcsncpy wcsncpy 
	#define _tcsncmp wcsncmp
	#define _tchar wchar_t
	#define _tfopen _wfopen
	#define _tstat _wstat
	#define _tstrlen wcslen
	//#define L L
#else
	#define _tcscpy strncpy
	#define _tcsncmp strncmp
	#define _tchar char
	#define _tfopen fopen
	#define _tstat stat
	#define _tstrlen strlen
	#define L  
#endif


extern struct node *parse_xml(const wchar_t *file_name);
extern struct node *search_child(struct node *parent, const wchar_t *name);
extern struct node *search_brother(struct node *brother, const wchar_t *name);

#endif