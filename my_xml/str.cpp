#include "str.h"

const _tchar *skip_space(const _tchar *start,const _tchar *end, const _tchar *s)
{
	const _tchar *tem = start;
	int len = 0;

	assert(start);
	assert(end);
	assert(s);

	while ((tem < end) && (*tem != '/0'))
	{
		len = _tcscspn(tem,s);
		if (len == 0)
			tem++;
		else
			return tem;
	}
	return NULL;
}