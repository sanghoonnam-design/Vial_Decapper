#ifndef   __A6_DRIVER_TEST_H__
#define   __A6_DRIVER_TEST_H__

#include "project.h"

#ifdef __A6_DRIVER_TEST_C__
	#define A6_DRIVER_TEST_EXT
#else
	#define A6_DRIVER_TEST_EXT extern
#endif


A6_DRIVER_TEST_EXT void A6_Driver_test_Init(void);


#endif
