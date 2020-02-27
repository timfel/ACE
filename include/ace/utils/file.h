/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_FILE_H_
#define _ACE_UTILS_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <ace/types.h>

typedef enum _tFileSeekMode {
	FILE_SEEK_CURRENT,
	FILE_SEEK_SET,
	FILE_SEEK_END,
	FILE_SEEK_COUNT,
} tFileSeekMode;

typedef struct _tFile {
	UBYTE isMem;
	union {
		struct {
			const UBYTE *pData;
			UWORD uwPos;
			UWORD uwSize;
		} asMem;
		struct {
			FILE *pHandle;
		} asFile;
	};
} tFile;

tFile *fileOpen(const char *szPath, const char *szMode);

tFile *fileOpenFromMem(const UBYTE *pData, UWORD uwSize);

void fileClose(tFile *pFile);

ULONG fileRead(tFile *pFile, void *pDest, ULONG ulSize);

ULONG fileWrite(tFile *pFile, void *pSrc, ULONG ulSize);

UBYTE fileSeek(tFile *pFile, LONG lPos, tFileSeekMode eMode);

ULONG fileGetPos(tFile *pFile);

UBYTE fileIsEof(tFile *pFile);

LONG fileVaPrintf(tFile *pFile, const char *szFmt, va_list vaArgs);

LONG filePrintf(tFile *pFile, const char *szFmt, ...);

LONG fileVaScanf(tFile *pFile, const char *szFmt, va_list vaArgs);

LONG fileScanf(tFile *pFile,const char *szFmt, ...);

void fileFlush(tFile *pFile);

/**
 * @brief Returns file size of file, in bytes.
 *
 * @param szPath Path to file.
 * @return LONG On fail -1, otherwise file size in bytes.
 */
LONG fileGetSize(const char *szPath);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_FILE_H_
