/**
 * WinPR: Windows Portable Runtime
 * Collections
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Modified on 2017-10-25 by Roland Haas to by indepenendent of WinPR
 */

#define BOOL int
#define BYTE unsigned char
#define WINPR_API

#define CopyMemory memcpy

#define FALSE 0
#define TRUE 1

#ifndef WINPR_COLLECTIONS_H
#define WINPR_COLLECTIONS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BipBuffer */

struct _wBipBlock
{
	size_t index;
	size_t size;
};
typedef struct _wBipBlock wBipBlock;

struct _wBipBuffer
{
	size_t size;
	BYTE* buffer;
	wBipBlock blockA;
	wBipBlock blockB;
	wBipBlock readR;
	wBipBlock writeR;
};
typedef struct _wBipBuffer wBipBuffer;

WINPR_API BOOL BipBuffer_Grow(wBipBuffer* bb, size_t size);
WINPR_API void BipBuffer_Clear(wBipBuffer* bb);

WINPR_API size_t BipBuffer_UsedSize(wBipBuffer* bb);
WINPR_API size_t BipBuffer_BufferSize(wBipBuffer* bb);

WINPR_API BYTE* BipBuffer_WriteReserve(wBipBuffer* bb, size_t size);
WINPR_API BYTE* BipBuffer_WriteTryReserve(wBipBuffer* bb, size_t size, size_t* reserved);
WINPR_API void BipBuffer_WriteCommit(wBipBuffer* bb, size_t size);

WINPR_API BYTE* BipBuffer_ReadReserve(wBipBuffer* bb, size_t size);
WINPR_API BYTE* BipBuffer_ReadTryReserve(wBipBuffer* bb, size_t size, size_t* reserved);
WINPR_API void BipBuffer_ReadCommit(wBipBuffer* bb, size_t size);

WINPR_API int BipBuffer_Read(wBipBuffer* bb, BYTE* data, size_t size);
WINPR_API int BipBuffer_Write(wBipBuffer* bb, BYTE* data, size_t size);

WINPR_API wBipBuffer* BipBuffer_New(size_t size);
WINPR_API void BipBuffer_Free(wBipBuffer* bb);

#ifdef __cplusplus
}
#endif

#endif /* WINPR_COLLECTIONS_H */
