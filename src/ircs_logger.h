#ifndef IRCS_LOGGER_H_
#define IRCS_LOGGER_H_

// Extended background colors (256 colors)
#define IRCS_LOG_EXTENDED_BG(x) "\x1b[48;5;" #x "m"
// Extended text colors (256 colors)
#define IRCS_LOG_EXTENDED_TEXT(x) "\x1b[38;5;" #x "m"


namespace ircs {
namespace log {

// Basic text colors
const char kBlackText[] = "\x1b[30m";
const char kRedText[] = "\x1b[31m";
const char kGreenText[] = "\x1b[32m";
const char kYellowText[] = "\x1b[33m";
const char kBlueText[] = "\x1b[34m";
const char kMagentaText[] = "\x1b[35m";
const char kCyanText[] = "\x1b[36m";
const char kWhiteText[] = "\x1b[37m";

// Basic background colors
const char kBlackBg[] = "\x1b[40m";
const char kRedBg[] = "\x1b[41m";
const char kGreenBg[] = "\x1b[42m";
const char kYellowBg[] = "\x1b[43m";
const char kBlueBg[] = "\x1b[44m";
const char kMagentaBg[] = "\x1b[45m";
const char kCyanBg[] = "\x1b[46m";
const char kWhiteBg[] = "\x1b[47m";

const char kResetAttr[] = "\x1b[0m";
const char kBoldStart[] = "\x1b[1m";
const char kBoldEnd[] = "\x1b[22m";

enum Level {
    kError,
    kInfo,
    kDebug,
    kWarning,
};

int Logger(Level level, const char *function_name, const char *fmt, ...);

}  // namespace log
}  // namespace ircs

#endif  // IRCS_LOGGER_H_
