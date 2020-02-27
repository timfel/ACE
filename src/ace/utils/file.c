/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/utils/file.h>
#include <stdarg.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

LONG fileGetSize(const char *szPath) {
	// One could use std library to seek to end of file and use ftell,
	// but SEEK_END is not guaranteed to work.
	// http://www.cplusplus.com/reference/cstdio/fseek/
	// Also this variant is 14 bytes smaller on Amiga ;)
	systemUse();
	BPTR pLock = Lock((CONST_STRPTR)szPath, ACCESS_READ);
	if(!pLock) {
		systemUnuse();
		return -1;
	}
	struct FileInfoBlock sFileBlock;
	LONG lResult = Examine(pLock, &sFileBlock);
	UnLock(pLock);
	systemUnuse();
	if(lResult == DOSFALSE) {
		return -1;
	}
	return sFileBlock.fib_Size;
}

tFile *fileOpen(const char *szPath, const char *szMode) {
	// TODO check if disk is read protected when szMode has 'a'/'w'/'x'
	systemUse();
	tFile *pFile = 0;
	FILE *pHandle = fopen(szPath, szMode);
	if(pHandle) {
		pFile = memAllocFast(sizeof(*pFile));
		pFile->isMem = 0;
		pFile->asFile.pHandle = pHandle;
	}
	systemUnuse();
	return pFile;
}

tFile *fileOpenFromMem(const UBYTE *pData, UWORD uwSize) {
	tFile *pFile = memAllocFast(sizeof(*pFile));
	pFile->isMem = 1;
	pFile->asMem.pData = pData;
	pFile->asMem.uwPos = 0;
	pFile->asMem.uwSize = uwSize;
	return pFile;
}

void fileClose(tFile *pFile) {
	systemUse();
	if(!pFile->isMem) {
		fclose(pFile->asFile.pHandle);
	}
	memFree(pFile, sizeof(*pFile));
	systemUnuse();
}

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize) {
#ifdef ACE_DEBUG
	if(!ulSize) {
		logWrite("ERR: File read size = 0!\n");
	}
#endif
	ULONG ulBytesRead;
	if(pFile->isMem) {
		ulBytesRead = MIN(pFile->asMem.uwSize - pFile->asMem.uwPos, (LONG)ulSize);
		memcpy(pDest, &pFile->asMem.pData[pFile->asMem.uwPos], ulBytesRead);
		pFile->asMem.uwPos += ulBytesRead;
	}
	else {
		systemUse();
		ulBytesRead = fread(pDest, ulSize, 1, pFile->asFile.pHandle);
		systemUnuse();
	}
	return ulBytesRead;
}

ULONG fileWrite(tFile *pFile, void *pSrc, ULONG ulSize) {
	ULONG ulBytesWritten;
	if(pFile->isMem) {
		logWrite("ERR: Unimplemented fileWrite for mem!\r\n");
		ulBytesWritten = 0;
	}
	else {
		systemUse();
		ulBytesWritten = fwrite(pSrc, ulSize, 1, pFile->asFile.pHandle);
		fflush(pFile->asFile.pHandle);
		systemUnuse();
	}
	return ulBytesWritten;
}

UBYTE fileSeek(tFile *pFile, LONG lPos, tFileSeekMode eMode) {
	UBYTE isOk = 0;
	if(pFile->isMem) {
		LONG lNewPos;
		if(eMode == FILE_SEEK_CURRENT) {
			lNewPos = pFile->asMem.uwPos + lPos;
		}
		else if(eMode == FILE_SEEK_END) {
			lNewPos = pFile->asMem.uwSize + lPos;
		}
		else { // FILE_SEEK_SET
			lNewPos = lPos;
		}
		if(0 <= lNewPos && lNewPos <= pFile->asMem.uwSize) {
			pFile->asMem.uwPos = lNewPos;
			isOk = 1;
		}
	}
	else {
		static const int pModes[FILE_SEEK_COUNT] = {
			[FILE_SEEK_CURRENT] = SEEK_CUR, [FILE_SEEK_SET] = SEEK_SET,
			[FILE_SEEK_END] = SEEK_END
		};
		systemUse();
		isOk = !fseek(pFile->asFile.pHandle, lPos, pModes[eMode]);
		systemUnuse();
	}
	return isOk;
}

ULONG fileGetPos(tFile *pFile) {
	ULONG ulPos;
	if(pFile->isMem) {
		ulPos = pFile->asMem.uwPos;
	}
	else {
		systemUse();
		ulPos = ftell(pFile->asFile.pHandle);
		systemUnuse();
	}
	return ulPos;
}

UBYTE fileIsEof(tFile *pFile) {
	UBYTE isEof;
	if(pFile->isMem) {
		isEof = (pFile->asMem.uwPos == pFile->asMem.uwSize);
	}
	else {
		systemUse();
		isEof = feof(pFile->asFile.pHandle);
		systemUnuse();
	}
	return isEof;
}

LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	LONG lBytesWritten;
	if(pFile->isMem) {
		logWrite("ERR: unimplemented fileVaPrintf for mem!\n");
		lBytesWritten = 0;
	}
	else {
		systemUse();
		lBytesWritten = vfprintf(pFile->asFile.pHandle, szFmt, vaArgs);
		fflush(pFile->asFile.pHandle);
		systemUnuse();
	}
	return lBytesWritten;
}

LONG filePrintf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lBytesWritten = fileVaPrintf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lBytesWritten;
}

LONG fileVaScanf(tFile *pFile, const char *szFmt, va_list vaArgs) {
	LONG lArgsRead;
	if(pFile->isMem) {
		logWrite("ERR: unimplemented fileVaScanf for mem!\n");
		lArgsRead = 0;
	}
	else {
		systemUse();
		lArgsRead = vfscanf(pFile->asFile.pHandle, szFmt, vaArgs);
		systemUnuse();
	}
	return lArgsRead;
}

LONG fileScanf(tFile *pFile, const char *szFmt, ...) {
	va_list vaArgs;
	va_start(vaArgs, szFmt);
	LONG lArgsRead = fileVaScanf(pFile, szFmt, vaArgs);
	va_end(vaArgs);
	return lArgsRead;
}

void fileFlush(tFile *pFile) {
	if(!pFile->isMem) {
		systemUse();
		fflush(pFile->asFile.pHandle);
		systemUnuse();
	}
}
