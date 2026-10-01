#include "../src/ircs_connection.h"
#include "catch.hpp"

using namespace ircs;

TEST_CASE("Nickname Validation", "[nickname]")
{
    EventLoopContext context;

    Client existing_client;
    existing_client.fd = 42;
    existing_client.nick_name = "Thomas";

    context.clients[42] = &existing_client;

    // valid names
    REQUIRE(is_ValidNickname("Jonas", -1, context) == true);
    REQUIRE(is_ValidNickname("Jonas123", -1, context) == true);
    REQUIRE(is_ValidNickname("J-nas", -1, context) == true);

    // invalid names
    REQUIRE(is_ValidNickname("Jonas Schmunk", -1, context) == false);
    REQUIRE(is_ValidNickname("Jonas,", -1, context) == false);
    REQUIRE(is_ValidNickname("Jonas*", -1, context) == false);
    REQUIRE(is_ValidNickname("Jonas?", -1, context) == false);
    REQUIRE(is_ValidNickname("Jonas!", -1, context) == false);
    REQUIRE(is_ValidNickname("Jonas@", -1, context) == false);

    // invalid names: first char
    REQUIRE(is_ValidNickname("$Jonas", -1, context) == false);
    REQUIRE(is_ValidNickname(":Jonas", -1, context) == false);
    REQUIRE(is_ValidNickname("#Jonas", -1, context) == false);
    REQUIRE(is_ValidNickname("&Jonas", -1, context) == false);
    REQUIRE(is_ValidNickname("@Jonas", -1, context) == false);

    // invalid names: empty name
    REQUIRE(is_ValidNickname("", -1, context) == false);

    // invalid names: too long
    REQUIRE(is_ValidNickname("a_very_very_very_very_long_nickname", -1, context) == false);

    // invalid names: duplicate
    REQUIRE(is_ValidNickname("Thomas", -1, context) == false);
}

TEST_CASE("Username Validation", "[username]")
{
    // valid names
    REQUIRE(is_ValidUsername("jschmunk") == true);
    REQUIRE(is_ValidUsername("user.name") == true);

    // invalid names
    REQUIRE(is_ValidUsername("~jschmunk") == false);
    REQUIRE(is_ValidUsername("") == false);
    REQUIRE(is_ValidUsername("a_very_long_username") == false);
    REQUIRE(is_ValidUsername("Jonas Schmunk") == false);
    REQUIRE(is_ValidUsername("user@name") == false);
    REQUIRE(is_ValidUsername("user:name") == false);
}

TEST_CASE("Password Validation", "[password]")
{
    // valid password
    REQUIRE(is_ValidPassword("6767", NULL) == true);
    REQUIRE(is_ValidPassword("6767", "6767") == true);

    // invalid password
    REQUIRE(is_ValidPassword("6767", "6969") == false);
}

TEST_CASE("Client Validation", "[client]")
{
    EventLoopContext context;
    context.password = const_cast<char *>("6767");

    Client existing_client;
    existing_client.fd = 42;
    existing_client.nick_name = "Thomas";
    context.clients[42] = &existing_client;

    // valid client
    Client valid_client;
    valid_client.fd = 1;
    valid_client.state = CONNECTED;
    valid_client.pass_accepted = false;
    valid_client.nick_accepted = false;
    valid_client.name_accepted = false;
    context.clients[1] = &valid_client;

    msg::Message msg;
    msg.command = "PASS";
    msg.parameters[0] = "6767";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "USER";
    msg.parameters[0] = "Jonas";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "NICK";
    msg.parameters[0] = "jojo";

    REQUIRE(is_ValidClient(1, msg, context) == true);

    // invalid pass
    Client invalid_pass;
    invalid_pass.fd = 2;
    invalid_pass.state = CONNECTED;
    invalid_pass.pass_accepted = false;
    invalid_pass.nick_accepted = false;
    invalid_pass.name_accepted = false;
    context.clients[2] = &invalid_pass;

    msg.command = "PASS";
    msg.parameters[0] = "6969";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "USER";
    msg.parameters[0] = "Password";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "NICK";
    msg.parameters[0] = "pass";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    // invalid user_name
    Client invalid_user;
    invalid_user.fd = 3;
    invalid_user.state = CONNECTED;
    invalid_user.pass_accepted = false;
    invalid_user.nick_accepted = false;
    invalid_user.name_accepted = false;
    context.clients[3] = &invalid_user;

    msg.command = "PASS";
    msg.parameters[0] = "6767";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "USER";
    msg.parameters[0] = "~jschmunk";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "NICK";
    msg.parameters[0] = "pass";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    // invalid nick_name
    Client invalid_nick;
    invalid_nick.fd = 4;
    invalid_nick.state = CONNECTED;
    invalid_nick.pass_accepted = false;
    invalid_nick.nick_accepted = false;
    invalid_nick.name_accepted = false;
    context.clients[4] = &invalid_nick;

    msg.command = "PASS";
    msg.parameters[0] = "6767";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "USER";
    msg.parameters[0] = "Nick";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "NICK";
    msg.parameters[0] = "invalid nick";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    // duplicate nick_name
    Client dup_nick;
    dup_nick.fd = 5;
    dup_nick.state = CONNECTED;
    dup_nick.pass_accepted = false;
    dup_nick.nick_accepted = false;
    dup_nick.name_accepted = false;
    context.clients[5] = &dup_nick;

    msg.command = "PASS";
    msg.parameters[0] = "6767";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "USER";
    msg.parameters[0] = "Thomas";

    REQUIRE(is_ValidClient(1, msg, context) == false);

    msg.command = "NICK";
    msg.parameters[0] = "Thomas";

    REQUIRE(is_ValidClient(1, msg, context) == false);
}
