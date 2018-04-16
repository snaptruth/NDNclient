/*
 * bntime.h
 *
 *  Created on: 2017年8月23日
 */

#ifndef INCLUDE_BNOSUTILS_H_
#define INCLUDE_BNOSUTILS_H_

#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// Add gettimeofday function for windows system
#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp);
#endif

bool ExecuteCmd(const char * pcCommand);

#endif /* INCLUDE_BNOSUTILS_H_ */
