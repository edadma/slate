#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include <stdio.h>

// Enable double precision floating point support
#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_PRECISION 1e-12
#define UNITY_DOUBLE_TYPE double

// Enable 64-bit support
#define UNITY_SUPPORT_64

// Custom output for better readability
#define UNITY_OUTPUT_CHAR(a) putchar(a)
#define UNITY_OUTPUT_FLUSH() fflush(stdout)
#define UNITY_OUTPUT_START() 
#define UNITY_OUTPUT_COMPLETE()

#endif // UNITY_CONFIG_H