CXX ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -pedantic -MMD -MP
BUILD := build

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

CONSOLE_SRC := $(wildcard console/*.cpp)
CONSOLE_OBJ := $(CONSOLE_SRC:%.cpp=$(BUILD)/%.o)
CONSOLE_BIN := $(BUILD)/crcsim$(EXE)

.PHONY: all console clean

all: console

console: $(CONSOLE_BIN)

$(CONSOLE_BIN): $(CONSOLE_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD)

-include $(CONSOLE_OBJ:.o=.d)
