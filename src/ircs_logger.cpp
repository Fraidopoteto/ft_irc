#include "ircs_logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace ircs {
namespace log {

namespace {

const char *log_level_strings[] = {
    "ERROR",
    "INFO",
    "DEBUG",
    "WARNING",
};

const char *log_level_colors[] = {
    kRedBg,
    kBlueBg,
    kMagentaText,
    kYellowBg,
};

}  // namespace

int Logger(Level level, const char *function_name, const char *fmt, ...)
{
    int rv;
    const char *kDebugInfo = getenv("IRCS_DEBUG");

    if ((kDebugInfo == NULL || strcmp(kDebugInfo, "1") != 0) && level == kDebug)
        return 0;

    std::va_list args;
    va_start(args, fmt);

    std::cerr << log_level_colors[level] << kBoldStart << " " << log_level_strings[level] << " "
              << kResetAttr << " " << kGreenText << function_name << kResetAttr << " : ";
    rv = std::vfprintf(stderr, fmt, args);

    std::cerr << std::endl;  // flush output

    va_end(args);

    return rv;
}

}  // namespace log
}  // namespace ircs
