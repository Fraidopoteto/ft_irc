#ifndef IRCS_MESSAGE_H_
#define IRCS_MESSAGE_H_

#include <sys/types.h>

#include <string>

namespace ircs {

struct Client;
struct EventLoopContext;

namespace msg {

const int kMsgBufSize = 512;
const int kMaxParameters = 15;
const char kCrLf[] = "\r\n";

const char *const kPrivMsg = "PRIVMSG";
const char *const kUser = "USER";
const char *const kNick = "NICK";
const char *const kPass = "PASS";

// IRC Error Codes
// Error replies (400 to 599)
enum kErrorCodes {
    // Target & Messaging Errors
    kErrNoSuchNick = 401,
    kErrNoSuchServer = 402,
    kErrNoSuchChannel = 403,
    kErrCannotSendToChan = 404,
    kErrTooManyChannels = 405,
    kErrWasNoSuchNick = 406,
    kErrTooManyTargets = 407,
    kErrNoSuchService = 408,
    kErrNoOrigin = 409,
    kErrNoRecipient = 411,
    kErrNoTextToSend = 412,
    kErrNoTopLevel = 413,
    kErrWildTopLevel = 414,
    kErrBadMask = 415,

    // Command & System Errors
    kErrUnknownCommand = 421,
    kErrNoMotd = 422,
    kErrNoAdminInfo = 423,
    kErrFileError = 424,

    // Nickname & Registration Errors
    kErrNoNicknameGiven = 431,
    kErrErroneousNickname = 432,
    kErrNicknameInUse = 433,
    kErrNickCollision = 436,
    kErrUnavailResource = 437,

    // Membership & Session Errors
    kErrUserNotInChannel = 441,
    kErrNotOnChannel = 442,
    kErrUserOnChannel = 443,
    kErrNoLogin = 444,
    kErrSummonDisabled = 445,
    kErrUsersDisabled = 446,
    kErrNotRegistered = 451,

    // Parameter & Auth Errors
    kErrNeedMoreParams = 461,
    kErrAlreadyRegistered = 462,
    kErrNoPermForHost = 463,
    kErrPasswdMismatch = 464,
    kErrYoureBannedCreep = 465,
    kErrYouWillBeBanned = 466,
    kErrKeySet = 467,

    // Channel Mode & Access Errors
    kErrChannelIsFull = 471,
    kErrUnknownMode = 472,
    kErrInviteOnlyChan = 473,
    kErrBannedFromChan = 474,
    kErrBadChannelKey = 475,
    kErrBadChanMask = 476,
    kErrNoChanModes = 477,
    kErrBanListFull = 478,

    // Privilege & Operator Errors
    kErrNoPrivileges = 481,
    kErrChanOpRivsNeeded = 482,
    kErrCantKillServer = 483,
    kErrRestricted = 484,
    kErrUniqOpPrivsNeeded = 485,
    kErrNoOperHost = 491,

    // User Mode Errors
    kErrUModeUnknownFlag = 501,
    kErrUsersDontMatch = 502
};

// enum Command { kPrivMsg,
//     kUser,
//     kNick,
//     kPass,
//     kJoin,
//     kPart,
//     kKick,
// };

struct Message {
    std::string prefix;
    std::string command;
    std::string parameters[kMaxParameters - 1];
    std::string trailing;  // The last parameter
};

int ParseMessage(const std::string &message, Message &msg);
int ProcessTcpStream(EventLoopContext &context, char *buf, Client *client, ssize_t buflen);

}  // namespace msg
}  // namespace ircs

#endif  // IRCS_MESSAGE_H_
