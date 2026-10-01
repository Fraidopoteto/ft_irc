#include "catch.hpp"
#include "ircs_message.h"

TEST_CASE("ParseMessage correctly parses various IRC message formats", "[meta]")
{
    // This variable is freshly initialized at the start of EVERY section.
    ircs::msg::Message msg;

    SECTION("empty") {
        ircs::msg::ParseMessage("", msg);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command.empty());
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }
    SECTION("trailing spaces") {
        ircs::msg::ParseMessage(":prefix CMD ", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix only") {
        ircs::msg::ParseMessage(":prefix", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command.empty());
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Command only") {
        ircs::msg::ParseMessage("NICK", msg);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Command with one parameter") {
        ircs::msg::ParseMessage("NICK Guest67", msg);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0] == "Guest67");
        REQUIRE(msg.parameters[1].empty()); // Verifies no extra parameters were parsed
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, and one parameter") {
        ircs::msg::ParseMessage(":prefix NICK Guest67", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0] == "Guest67");
        REQUIRE(msg.parameters[1].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, and multiple parameters") {
        ircs::msg::ParseMessage(":prefix CMD para1 para2", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, parameters, and trailing text") {
        ircs::msg::ParseMessage(":prefix CMD para1 para2 :trailing", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("Trailing text with carriage returns and spaces") {
        ircs::msg::ParseMessage(":prefix CMD para1 para2 :trailing with  \r spaces", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing == "trailing with  \r spaces");
    }

    SECTION("Prefix, command and trailing") {
        ircs::msg::ParseMessage(":prefix CMD :trailing", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("prefix, command, all params and trailing with colon") {
        ircs::msg::ParseMessage(":prefix CMD 1 2 3 4 5 6 7 8 9 10 11 12 13 14 :trailing", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "1");
        REQUIRE(msg.parameters[13] == "14");
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("prefix, command, all params and trailing without colon") {
        ircs::msg::ParseMessage(":prefix CMD 1 2 3 4 5 6 7 8 9 10 11 12 13 14 trailing", msg);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "1");
        REQUIRE(msg.parameters[13] == "14");
        REQUIRE(msg.trailing == "trailing");
    }
}
