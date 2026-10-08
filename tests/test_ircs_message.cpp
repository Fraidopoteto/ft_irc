#include "catch.hpp"
#include "ircs_message.h"

TEST_CASE("ParseMessage correct error detection", "") {
    ircs::msg::Message msg;
    int ret;

    SECTION("empty") {
        ret = ircs::msg::ParseMessage("", msg);
        REQUIRE(ret == -1);
    }

    SECTION("prefix only") {
        ret = ircs::msg::ParseMessage(":prefix", msg);
        REQUIRE(ret == -1);
    }

    SECTION("test prefix") {
        SECTION("correct prefix 1") {
            ret = ircs::msg::ParseMessage(":nickname!user@host CMD", msg);
            REQUIRE(ret == 0);
        }
        SECTION("correct prefix 2") {
            ret = ircs::msg::ParseMessage(":nickname!@ CMD", msg);
            REQUIRE(ret == 0);
        }
        SECTION("! at beginning") {
            ret = ircs::msg::ParseMessage(":!nickname CMD", msg);
            REQUIRE(ret == -1);
        }
        SECTION("multiple !") {
            ret = ircs::msg::ParseMessage(":nickname!! CMD", msg);
            REQUIRE(ret == -1);
        }
        SECTION("@ at beginning") {
            ret = ircs::msg::ParseMessage(":@nickname CMD", msg);
            REQUIRE(ret == -1);
        }
        SECTION("multiple @") {
            ret = ircs::msg::ParseMessage(":nick@na@me CMD", msg);
            REQUIRE(ret == -1);
        }
        SECTION("invalid characters") {
            SECTION("CR") {
                ret = ircs::msg::ParseMessage(":nick\rname CMD", msg);
                REQUIRE(ret == -1);
            }
            SECTION("LF") {
                ret = ircs::msg::ParseMessage(":nick\nname CMD", msg);
                REQUIRE(ret == -1);
            }
            SECTION("NUL") {
                std::string s(":nick\0name CMD", 14);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
        }
    }

    SECTION("test command") {
        SECTION("invalid characters") {
            SECTION("numbers") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CM1D para", msg);
                REQUIRE(ret == -1);
            }
            SECTION("CR") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CM\rD para", msg);
                REQUIRE(ret == -1);
            }
            SECTION("LF") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CM\nD para", msg);
                REQUIRE(ret == -1);
            }
            SECTION("NUL") {
                std::string s(":nickname!user@host CM\0D para", 29);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
        }
    }

    SECTION("test parameters") {
        SECTION("invalid characters") {
            SECTION("CR") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CMD pa\rra", msg);
                REQUIRE(ret == -1);
            }
            SECTION("LF") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CMD pa\nra", msg);
                REQUIRE(ret == -1);
            }
            SECTION("NUL") {
                std::string s(":nickname!user@host CMD pa\0ra", 29);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
            SECTION(":") {
                ret = ircs::msg::ParseMessage(":nickname!user@host CMD para:", msg);
                REQUIRE(ret == -1);
            }
        }
}

    SECTION("test trailing") {
        SECTION("valid") {
            ret = ircs::msg::ParseMessage(":nickname!user@host CMD :this is  \t :valid", msg);
            REQUIRE(ret == 0);
        }
        SECTION("empty trailing") {
            ret = ircs::msg::ParseMessage(":nickname!user@host CMD :", msg);
            REQUIRE(ret == 0);
        }
        SECTION("invalid characters") {
            SECTION("NUL") {
                std::string s(":nickname!user@host CMD :trai\0ling", 34);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
            SECTION("CR") {
                std::string s(":nickname!user@host CMD :trai\rling", 34);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
            SECTION("LF") {
                std::string s(":nickname!user@host CMD :trai\nling", 34);
                ret = ircs::msg::ParseMessage(s, msg);
                REQUIRE(ret == -1);
            }
        }
    }
}

TEST_CASE("ParseMessage correct Message initialization", "[meta]")
{
    // This variable is freshly initialized at the start of EVERY section.
    ircs::msg::Message msg;
    int ret;

    SECTION("empty") {
        ret = ircs::msg::ParseMessage("", msg);
        REQUIRE(ret == -1);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command.empty());
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }
    SECTION("trailing spaces") {
        ret = ircs::msg::ParseMessage(":prefix CMD ", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix only") {
        ret = ircs::msg::ParseMessage(":prefix", msg);
        REQUIRE(ret == -1);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command.empty());
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Command only") {
        ret = ircs::msg::ParseMessage("NICK", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Command with one parameter") {
        ret = ircs::msg::ParseMessage("NICK Guest67", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix.empty());
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0] == "Guest67");
        REQUIRE(msg.parameters[1].empty()); // Verifies no extra parameters were parsed
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, and one parameter") {
        ret = ircs::msg::ParseMessage(":prefix NICK Guest67", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "NICK");
        REQUIRE(msg.parameters[0] == "Guest67");
        REQUIRE(msg.parameters[1].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, and multiple parameters") {
        ret = ircs::msg::ParseMessage(":prefix CMD para1 para2", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing.empty());
    }

    SECTION("Prefix, command, parameters, and trailing text") {
        ret = ircs::msg::ParseMessage(":prefix CMD para1 para2 :trailing", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("Trailing text with carriage returns and spaces") {
        ret = ircs::msg::ParseMessage(":prefix CMD para1 para2 :trailing with   spaces", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "para1");
        REQUIRE(msg.parameters[1] == "para2");
        REQUIRE(msg.parameters[2].empty());
        REQUIRE(msg.trailing == "trailing with   spaces");
    }

    SECTION("Prefix, command and trailing") {
        ret = ircs::msg::ParseMessage(":prefix CMD :trailing", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("prefix, command, all params and trailing with colon") {
        ret = ircs::msg::ParseMessage(":prefix CMD 1 2 3 4 5 6 7 8 9 10 11 12 13 14 :trailing", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "1");
        REQUIRE(msg.parameters[13] == "14");
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("prefix, command, all params and trailing without colon") {
        ret = ircs::msg::ParseMessage(":prefix CMD 1 2 3 4 5 6 7 8 9 10 11 12 13 14 trailing", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0] == "1");
        REQUIRE(msg.parameters[13] == "14");
        REQUIRE(msg.trailing == "trailing");
    }

    SECTION("prefix, command, empty trailing") {
        ret = ircs::msg::ParseMessage(":prefix CMD :", msg);
        REQUIRE(ret == 0);

        REQUIRE(msg.prefix == "prefix");
        REQUIRE(msg.command == "CMD");
        REQUIRE(msg.parameters[0].empty());
        REQUIRE(msg.trailing.empty());
    }
}

TEST_CASE("MessageBufferNext") {
    ircs::msg::MessageBuffer buffer = ircs::msg::MessageBuffer();
    std::string line;
    ircs::msg::ProcessStatus status;

    SECTION("one line") {
        buffer.data = "NICK nickname\r\n";
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 15);
        CHECK(buffer.overflow == false);
    }

    SECTION("multiple lines") {
        buffer.data = "PASS 6969\r\nNICK nickname\r\nUSER user\r\n";

        // First line
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 11);
        CHECK(buffer.overflow == false);

        // Second line
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 26);
        CHECK(buffer.overflow == false);

        // Third line
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 37);
        CHECK(buffer.overflow == false);
    }

    SECTION("incomplete line") {
        buffer.data = "NICK nickna";
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kNeedMoreData);

        CHECK(buffer.read_pos == 0);
        CHECK(buffer.overflow == false);
    }

    SECTION("one + incomplete line") {
        buffer.data = "NICK nickname\r\nUSER us";

        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 15);
        CHECK(buffer.overflow == false);

        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kNeedMoreData);

        CHECK(buffer.read_pos == 15);
        CHECK(buffer.overflow == false);
    }

    SECTION("big message 512") {
        buffer.data = std::string(510, 'A') + "\r\n";
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 512);
        CHECK(buffer.overflow == false);
    }

    SECTION("overflow with CRLF") {
        buffer.data = std::string(511, 'A') + "\r\n";
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kMessageTooLong);

        CHECK(buffer.read_pos == 0);
        CHECK(buffer.overflow == true);
    }

    SECTION("overflow without CRLF") {
        buffer.data = std::string(511, 'A');
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kMessageTooLong);

        CHECK(buffer.read_pos == 0);
        CHECK(buffer.overflow == true);
    }

    SECTION("one complete + overflow") {
        buffer.data = "NICK nickname\r\n" + std::string(511, 'A') + "\r\n";
        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kOk);

        CHECK(buffer.read_pos == 15);
        CHECK(buffer.overflow == false);

        status = ircs::msg::MessageBufferNext(buffer, line);
        REQUIRE(status == ircs::msg::kMessageTooLong);

        CHECK(buffer.read_pos == 15);
        CHECK(buffer.overflow == true);
    }
}
