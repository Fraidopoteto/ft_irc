#include "../src/ircs_connection.h"
#include "catch.hpp"

using namespace ircs;

TEST_CASE("Nickname Validation", "[nickname]")
{
    // valid names
    REQUIRE(ValidateNickname("Jonas", -1) == true);
    REQUIRE(ValidateNickname("Jonas123", -1) == true);
    REQUIRE(ValidateNickname("J-nas", -1) == true);

    // invalid names
    REQUIRE(ValidateNickname("Jonas Schmunk", -1) == false);
    REQUIRE(ValidateNickname("Jonas,", -1) == false);
    REQUIRE(ValidateNickname("Jonas*", -1) == false);
    REQUIRE(ValidateNickname("Jonas?", -1) == false);
    REQUIRE(ValidateNickname("Jonas!", -1) == false);
    REQUIRE(ValidateNickname("Jonas@", -1) == false);

    // invalid names: first char
    REQUIRE(ValidateNickname("$Jonas", -1) == false);
    REQUIRE(ValidateNickname(":Jonas", -1) == false);
    REQUIRE(ValidateNickname("#Jonas", -1) == false);
    REQUIRE(ValidateNickname("&Jonas", -1) == false);
    REQUIRE(ValidateNickname("@Jonas", -1) == false);

    // invalid names: empty name
    REQUIRE(ValidateNickname("", -1) == false);

    // invalid names: too long
    REQUIRE(ValidateNickname("a_very_very_very_very_long_nickname", -1) == false);
}

TEST_CASE("Username Validation", "[username]")
{
    // valid names
    REQUIRE(ValidateUsername("jschmunk") == true);
    REQUIRE(ValidateUsername("user.name") == true);

    // invalid names
    REQUIRE(ValidateUsername("~jschmunk") == false);
    REQUIRE(ValidateUsername("") == false);
    REQUIRE(ValidateUsername("a_very_long_username") == false);
    REQUIRE(ValidateUsername("Jonas Schmunk") == false);
    REQUIRE(ValidateUsername("user@name") == false);
    REQUIRE(ValidateUsername("user:name") == false);
}

TEST_CASE("Password Validation", "[password]")
{
    // valid password
    REQUIRE(ValidatePassword("6767", NULL) == true);
    REQUIRE(ValidatePassword("6767", "6767") == true);

    // invalid password
    REQUIRE(ValidatePassword("6767", "6969") == false);
}
