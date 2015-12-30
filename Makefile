CXX = clang++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -D DEBUG -O2
BINARY = agbplay
LIBS = ../portaudio/libportaudio_static.a -lm -lncursesw -lboost_system -lboost_thread
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
	@echo "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)"
	@rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	@echo "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)"
	@$(CXX) -o $@ $(CXXFLAGS) $(LIBS) $^

%.o: %.cpp
	@echo "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

