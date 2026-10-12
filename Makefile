NAME	:= ircserv
CXX		:= c++
SRCDIR	:= src
OBJDIR	:= obj

INCS :=	-I $(SRCDIR)/

CXXFLAGS :=	-std=c++98 \
			-O2 \
			-Wall \
			-Werror \
			-Wextra \
			-Wshadow \
			-MMD \
			-MP

LDFLAGS :=

LDLIBS :=

ifeq ($(DEBUG), 1)
	CXXFLAGS += -DDEBUG
	CXXFLAGS += -g3 -O0
	LDFLAGS += -g3 -O0
endif

ifeq ($(LSAN), 1)
	CXXFLAGS += -fsanitize=leak
	LDFLAGS += -fsanitize=leak
endif

ifeq ($(ASAN), 1)
	CXXFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

SRC :=	ircserv.cpp \
		ircs_connection.cpp \
		ircs_epoll.cpp \
		ircs_logger.cpp \
		ircs_message.cpp
		# ircs_command.cpp \

OBJ	:= $(SRC:.cpp=.o)
OBJ	:= $(addprefix $(OBJDIR)/, $(OBJ))
DEP := $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

-include $(DEP)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re:
	@$(MAKE) fclean
	@$(MAKE) all

# ==========================================
# TEST CONFIGURATION
# ==========================================

TEST_NAME	:= test_runner
TEST_DIR	:= tests
TEST_OBJDIR	:= $(OBJDIR)/tests

TEST_CXXFLAGS	:= $(CXXFLAGS)
TEST_INCS		:= $(INCS) -isystem $(TEST_DIR)/

# Exclude main() in production code
TEST_APP_SRC := $(filter-out ircserv.cpp, $(SRC))
TEST_APP_OBJ := $(TEST_APP_SRC:.cpp=.o)
TEST_APP_OBJ := $(addprefix $(TEST_OBJDIR)/, $(TEST_APP_OBJ))

# Add Test Files Here!
# example: TEST_SRC := test_file.cpp
# ===========================================

TEST_SRC := test_main.cpp \
			test_ircs_message.cpp \
			test_ircs_client_init.cpp
			# test_ircs_command.cpp

# ===========================================
TEST_OBJ := $(TEST_SRC:.cpp=.o)
TEST_OBJ := $(addprefix $(TEST_OBJDIR)/, $(TEST_OBJ))

TEST_DEP := $(TEST_APP_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
-include $(TEST_DEP)

$(TEST_OBJDIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) $(INCS) -c $< -o $@

$(TEST_OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) $(INCS) -c $< -o $@

$(TEST_NAME): $(CATCH_OBJ) $(TEST_APP_OBJ) $(TEST_OBJ)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

.PHONY: all clean fclean re test
test: $(TEST_NAME)
