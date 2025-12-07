#include "platform.h"

OSType detect_os(void) {
#ifdef _WIN32
    return OS_WINDOWS;
#elif __linux__
    return OS_LINUX;
#else
    return OS_UNKNOWN;
#endif
}
