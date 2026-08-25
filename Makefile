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
		ircs_logger.cpp

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
	rm -f $(NAME)

re:
	@$(MAKE) fclean
	@$(MAKE) all

# ==========================================
# TEST CONFIGURATION
# ==========================================

TEST_NAME	:= test_runner
TEST_DIR	:= tests
TEST_OBJDIR	:= $(OBJDIR)/tests

# Swap C++98 for C++14 for testing compilation
TEST_CXXFLAGS := $(filter-out -std=c++98, $(CXXFLAGS)) -std=c++14

# 1. Production objects compiled in C++14 (Excluded core/webserv.cpp for main)
TEST_APP_SRC := $(filter-out core/webserv.cpp, $(SRC))
TEST_APP_OBJ := $(TEST_APP_SRC:.cpp=.o)
TEST_APP_OBJ := $(addprefix $(TEST_OBJDIR)/, $(TEST_APP_OBJ))

# 2. Test objects (Add new test files here)
# TEST_SRC := test_webs_connection.cpp
TEST_OBJ := $(TEST_SRC:.cpp=.o)
TEST_OBJ := $(addprefix $(TEST_OBJDIR)/, $(TEST_OBJ))

# The heavy Catch2 framework object
CATCH_OBJ := $(TEST_OBJDIR)/catch_amalgamated.o

# Include dependencies for test files
TEST_DEP := $(TEST_APP_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
-include $(TEST_DEP)

# 3. Compile Catch2 framework separately
$(CATCH_OBJ): $(TEST_DIR)/catch_amalgamated.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) $(INCS) -c $< -o $@

# 4. Compile the test files
$(TEST_OBJDIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) $(INCS) -c $< -o $@

# 5. Compile the C++14 versions of the production code
$(TEST_OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) $(INCS) -c $< -o $@

# 6. Link the test binary
$(TEST_NAME): $(CATCH_OBJ) $(TEST_APP_OBJ) $(TEST_OBJ)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

.PHONY: all clean fclean re test
test: $(TEST_NAME)
	./$(TEST_NAME)
