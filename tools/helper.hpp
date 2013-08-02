#ifdef __cplusplus
extern "C" {
#endif


void LogError(int rc,
            const char *functionName,
            const char *format,
            ...);


void LogWarn(
            const char *functionName,
            const char *format,
            ...);

void LogInfo(
            const char *functionName,
            const char *format,
            ...);

#ifdef __cplusplus
}
#endif

#include "timer.hpp"

