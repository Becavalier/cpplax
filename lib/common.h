#ifndef	_MACRO_H
#define	_MACRO_H

// #define DEBUG_STRESS_GC

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
#define DEBUG_LOG_GC

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#define DEFAULT_IDX 0
#define UINT8_COUNT (UINT8_MAX + 1)
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define GC_HEAP_GROW_FACTOR 2

constexpr char INITIALIZER_NAME[] = "init";

#endif
