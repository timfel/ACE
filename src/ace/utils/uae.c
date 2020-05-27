#include <ace/utils/uae.h>
#include <string.h>
#include <ace/types.h>

static volatile ULONG * const s_pUaeArg = (ULONG*)0xBFFF00;
static volatile ULONG * const s_pUaeFmt = (ULONG*)0xBFFF04;

void uaeVaPrintf(const char *szFmt, va_list vaArgs) {
	UBYTE isInFmtSeq = 0;
	UBYTE isExpectFlags = 0;
	UBYTE isExpectPrecDot = 0;
	UBYTE isExpectWidth = 0;
	UBYTE isExpectPrecWidth = 0;
	UBYTE isExpectAsterisk = 0;
	UBYTE isExpectLength = 0;

	char szBfr[2] = "a";

	while(*szFmt) {
		// if(isInFmtSeq) {
		// 	if(strchr("diuoxXfFeEgGaAcspn", *szFmt)) {
		// 		// specifier - end of fmt sequence
		// 		// Grab an arg and output it
		// 		// *s_pUaeArg = va_arg(vaArgs, ULONG);
		// 		isInFmtSeq = 0;
		// 	}
		// 	else if(*szFmt == '%') {
		// 		// Do nothing - escaped % sign
		// 		isInFmtSeq = 0;
		// 	}
		// 	else if(strchr("hljztL", *szFmt)) {
		// 		if(!isExpectLength || isExpectPrecWidth || isExpectWidth) {
		// 			// ERROR
		// 			return;
		// 		}
		// 		isExpectPrecWidth = 0;
		// 		isExpectWidth = 0;
		// 		isExpectPrecDot = 0;
		// 		isExpectFlags = 0;
		// 	}
		// 	else if(strchr("-+ #0", *szFmt)) {
		// 		if(!isExpectFlags) {
		// 			// ERROR
		// 			return;
		// 		}
		// 		isExpectFlags = 0;
		// 	}
		// 	else if(*szFmt == '*') {
		// 		if(!isExpectAsterisk) {
		// 			// ERROR
		// 			return;
		// 		}
		// 		// No more values in that width - grab an arg and output it
		// 		// *s_pUaeArg = va_arg(vaArgs, ULONG);
		// 		isExpectWidth = 0;
		// 		isExpectPrecWidth = 0;
		// 		isExpectAsterisk = 0;
		// 	}
		// 	else if(strchr("0123456789", *szFmt)) {
		// 		if(!isExpectWidth || !isExpectPrecWidth) {
		// 			// ERROR
		// 			return;
		// 		}
		// 		isExpectAsterisk = 0;
		// 		isExpectFlags = 0;
		// 	}
		// 	else if(*szFmt == '.') {
		// 		if(!isExpectPrecDot) {
		// 			// ERROR
		// 			return;
		// 		}
		// 		isExpectWidth = 0;
		// 		isExpectPrecDot = 0;
		// 		isExpectFlags = 0;
		// 		isExpectPrecWidth = 1;
		// 		isExpectAsterisk = 1;
		// 	}
		// }
		// else if(*szFmt == '%') {
		// 	isInFmtSeq = 1;
		// 	isExpectFlags = 1;
		// 	isExpectPrecDot = 1;
		// 	isExpectWidth = 1;
		// 	isExpectLength = 1;
		// }

		// szBfr[0] = 'b';
		*s_pUaeFmt = (ULONG)((UBYTE*)szBfr);
		++szFmt;
	}
}

void uaePrintf(const char *szFmt, ...)
{
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	uaeVaPrintf(szFmt, vaArgs);
	va_end(vaArgs);
}
