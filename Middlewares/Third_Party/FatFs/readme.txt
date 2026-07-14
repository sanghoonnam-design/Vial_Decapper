
#include "ff.h"
#include <string.h>

FATFS SDFatFs;  /* File system object for SD card logical drive */
FIL MyFile;     /* File object */

void SampleFunction(void)
{
	static uint8_t wtext[100]; 								
	static uint8_t rtext[100];  
  	uint32_t byteswritten, bytesread;  
  	
  	sprintf(wtext,"FatFS New Version R0.16\r\n");
  	
	f_mount(&SDFatFs, (TCHAR const*)"0:/", 0);
	f_open(&MyFile, "yjkang.TXT", FA_CREATE_ALWAYS | FA_WRITE);
	f_write(&MyFile, wtext, strlen(wtext), (void *)&byteswritten);
	f_close(&MyFile);
	
	f_open(&MyFile, "yjkang.TXT", FA_READ);
	f_read(&MyFile, rtext, byteswritten, (UINT*)&bytesread);
	f_close(&MyFile);
}
