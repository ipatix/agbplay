CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -D DEBUG -g -O3
BINARY = agbplay

SYS = $(shell $(CXX) -dumpmachine)

ifneq (, $(findstring linux, $(SYS)))
	# clang doesn't seem to compile correctly on windows but on linux it works
	CXX = clang++
	LIBS = ../portaudio/lib/.libs/libportaudio.a -lm -lncursesw -lboost_system -lboost_thread -pthread -lasound
else ifneq (, $(findstring cygwin, $(SYS))$(findstring windows, $(SYS)))
	LIBS = ../portaudio/lib/.libs/libportaudio.dll.a -lm -lncursesw -lboost_system -lboost_thread -pthread
else
	@echo "Unsupported Platform: $(SYS)"
endif

IMPORT = -I ../portaudio/include

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_FILES = $(wildcard *.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)

.PHONY: all
all: $(BINARY)

.PHONY: clean
clean:
	@printf "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)\n"
	@rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -o $@ $(CXXFLAGS) $^ $(LIBS) -lstdc++

%.o: %.cpp
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

