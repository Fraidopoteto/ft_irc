#ifndef IRCS_COMMAND_H_
#define IRCS_COMMAND_H_

#include <string>

#include "ircs_event_loop.h"

namespace ircs {

const char *const kPass = "PASS";
const char *const kUser = "USER";
const char *const kNick = "NICK";
const char *const kPrivMsg = "PRIVMSG";
const char *const kJoin = "JOIN";
const char *const kOper = "OPER";
const char *const kMode = "MODE";
const char *const kQuit = "QUIT";

// IRC Error Codes
// Error replies (400 to 599)
enum NumericReplies {
    kRplWelcome = 001,
    kRplYourHost = 002,
    kRplCreated = 003,
    kRplMyInfo = 004,

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

struct CommandEntry;

enum CommandStatus { kContinue, kDisconnect };

typedef CommandStatus (*CommandFn)(const CommandEntry *command, EventLoopContext *ctx,
                                   Client *client, const msg::Message &message);

enum CommandFlags {
    kCmdPreRegistrationOnly = 0x1,
    kCmdRequiresRegistration = 0x2,
};

struct CommandEntry {
    std::string name;  // e.g "PASS"
    CommandFn handler;
    int min_params;
    int max_params;
};

}  // namespace ircs

#endif  // IRCS_COMMAND_H_
