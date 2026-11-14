CXX ?= g++
AR ?= ar
PKG_CONFIG ?= pkg-config
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -pedantic -MMD -MP -Icore
BOOST_CXXFLAGS ?=
BUILD := build

ifeq ($(OS),Windows_NT)
EXE := .exe
STATIC_LDFLAGS := -static
else
EXE :=
STATIC_LDFLAGS :=
endif

CORE_SRC := $(wildcard core/*.cpp)
CORE_OBJ := $(CORE_SRC:%.cpp=$(BUILD)/%.o)
CORE_LIB := $(BUILD)/libcore.a

CONSOLE_SRC := $(wildcard console/*.cpp)
CONSOLE_OBJ := $(CONSOLE_SRC:%.cpp=$(BUILD)/%.o)
CONSOLE_BIN := $(BUILD)/crcsim$(EXE)

TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(TEST_SRC:%.cpp=$(BUILD)/%.o)
TEST_BIN := $(BUILD)/crcsim_tests$(EXE)

.PHONY: all console tests test research-tools clean

all: console

console: $(CONSOLE_BIN)

tests: $(TEST_BIN)

test: $(TEST_BIN)
	$(TEST_BIN)

WEIGHTS_BIN := $(BUILD)/weight_enumerator$(EXE)

research-tools: $(CONSOLE_BIN) $(WEIGHTS_BIN)

$(WEIGHTS_BIN): $(BUILD)/research/weight_enumerator.o $(CORE_LIB)
	$(CXX) $(LDFLAGS) $(STATIC_LDFLAGS) -o $@ $^

$(CORE_LIB): $(CORE_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(CONSOLE_BIN): $(CONSOLE_OBJ) $(CORE_LIB)
	$(CXX) $(LDFLAGS) $(STATIC_LDFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_OBJ) $(CORE_LIB)
	$(CXX) $(LDFLAGS) $(STATIC_LDFLAGS) -o $@ $^

$(BUILD)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(BOOST_CXXFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD)

-include $(CORE_OBJ:.o=.d) $(CONSOLE_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
