/*
 * This file is part of the Neuropixels System Developer Distribution (https://github.com/imec-Neuropixels/NP_SYSTEM ).
* Copyright (c) 2020 IMEC vzw, Kapeldreef 75, 3001 Leuven, Belgium.
*
 * This program is distributed under a free license agreement: BY USING OR REDISTRIBUTING THE LICENSED TECHNOLOGY,
* LICENSEE IS AGREEING TO THE TERMS AND CONDITIONS OF THIS LICENSE AGREEMENT.
* IF LICENSEE DOES NOT AGREE WITH THE TERMS AND CONDITIONS OF THE LICENSE AGREEMENT,
* LICENSEE MAY NOT USE OR REDISTRIBUTE THE LICENSED TECHNOLOGY.
*
* You, the licensee, should have received a copy of the Neuropixels System Developer License
 * along with this program. If not, see < https://github.com/imec-Neuropixels/NP_SYSTEM/blob/master/LICENSE/20200608_Neuropixels_System_Developer_License.pdf >.
*/
#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include "NeuropixAPI.h"
#include "debugutil.h"


#define VERSION(maj,min,build) ((maj&0xFF)<<24|(min&0xFF)<<16|build&0xFFFF)
#define VERSION_MAJ(ver) ((ver>>24) & 0xFF)
#define VERSION_MIN(ver) ((ver>>16) & 0xFF)
#define VERSION_BUILD(ver) (ver & 0xFFFF)

#pragma pack(push, 1)
typedef struct {
	union {
		uint32_t value;
		struct {
			uint16_t build;
			uint8_t min;
			uint8_t maj;
		};
	};
}version_t;
#pragma pack(pop)



#define LOGRETURN(something) \
do { \
  NP_ErrorCode errorcode = something; \
  debug_trace(DBG_VERBOSE, "return [%s] (code %i)", np_GetErrorMessage(errorcode), errorcode); \
  log_write(__FUNCTION__, errorcode); \
  log_disable(); \
  return errorcode; \
} while (0)

#define TRY(something) \
do { \
    NP_ErrorCode code = something; \
    if (code != SUCCESS) \
      LOGRETURN(code); \
  } while (0)

#define SETFLAGS(lhs,flags) lhs |= (flags)
#define CLEARFLAGS(lhs,flags) lhs &= ~(flags)


#endif // _UTIL_H_
