CXX = clang++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -D DEBUG -g -O2
BINARY = agbplay
LIBS = ../portaudio/lib/.libs/libportaudio.a -lm -lncursesw -lboost_system -lboost_thread -pthread -lasound
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
	@$(CXX) -o $@ $(CXXFLAGS) $^ $(LIBS)

%.o: %.cpp
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

