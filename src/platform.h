#ifndef PLATFORM_H
#define PLATFORM_H

typedef enum {
    OS_WINDOWS,
    OS_LINUX,
    OS_UNKNOWN
} OSType;

OSType detect_os(void);

#endif
