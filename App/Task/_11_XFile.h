#ifndef _XFILE_H_
#define _XFILE_H_

#include "XGlobal.h"

#ifdef __XFILE_C__
#define XFILE_EXT
#else
#define XFILE_EXT extern
#endif

XFILE_EXT void XFile_Init(void);
XFILE_EXT void XFile_Write(const char *fmt, ...);
XFILE_EXT U32 XFile_ReadWriteCount(void);
#endif //@end: _XFILE_H_
