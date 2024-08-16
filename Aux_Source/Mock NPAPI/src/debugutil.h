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
#ifndef _DEBUGUTIL_H_
#define _DEBUGUTIL_H_

#define DBG_SERDES DBG_VERBOSE
#define DBG_HS     DBG_VERBOSE

#define DBG_OUT stderr
//#define DBG_OUT stdout


//extern int dbg_minlevel;
#define DBG_MINLEVEL DBG_VERBOSE


void np_SetErrorMsg(char const* const fmt, ...);

#define debug_print(DBGLEVEL, fmt, ...) \
        do { if (DBGLEVEL==DBG_ERROR) np_SetErrorMsg(fmt, __VA_ARGS__);if (DBGLEVEL>=DBG_MINLEVEL) fprintf(DBG_OUT, fmt"\r\n", __VA_ARGS__); } while (0)

#define debug_trace(DBGLEVEL, fmt, ...) \
        do { if (DBGLEVEL>=DBG_MINLEVEL) fprintf(DBG_OUT, "%s:%d:%s(): " fmt"\r\n", __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)


#endif // _DEBUGUTIL_H_