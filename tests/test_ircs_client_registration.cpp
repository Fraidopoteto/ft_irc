#include "../src/ircs_connection.h"
#include "catch.hpp"

using namespace ircs;

TEST_CASE("Nickname Validation", "[nickname]")
{
    REQUIRE(ValidateNickname("Jonas", -1) == true);
    REQUIRE(ValidateNickname("Jonas123", -1) == true);
    REQUIRE(ValidateNickname("J-nas") == true);

    REQUIRE(ValidateNickname("Jonas!", -1) == false);         // Ausrufezeichen verboten
    REQUIRE(ValidateNickname("$Jonas", -1) == false);         // Dollar am Anfang verboten
    REQUIRE(ValidateNickname("", -1) == false);               // Leer verboten
    REQUIRE(ValidateNickname("Jonas Schmunk", -1) == false);  // Leerzeichen verboten
}

TEST_CASE("Username Validation", "[username]")
{
    REQUIRE(ValidateUsername("jschmunk") == true);
    REQUIRE(ValidateUsername("user.name") == true);

    REQUIRE(ValidateUsername("~jschmunk") == false);             // Darf nicht mit ~ starten
    REQUIRE(ValidateUsername("user@name") == false);             // @ verboten
    REQUIRE(ValidateUsername("a_very_long_username") == false);  // Über 10 Zeichen (RFC)
}