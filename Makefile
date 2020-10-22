CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -D NDEBUG -O3 -g
#CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -Og -g -fsanitize=address
BINARY = agbplay
LIBS = -lm -lncursesw -pthread -lboost_system -lboost_filesystem -lsndfile -lportaudio -ljsoncpp
# Use this macro if you have linker errors with ncursesw
# LIBS = -lm -lncurses -pthread -lboost_system -lboost_filesystem -lsndfile -lportaudio -ljsoncpp

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_FILES = $(wildcard src/*.cpp)
OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_FILES:.cpp=.o)))

.PHONY: all clean format
all: $(BINARY)

clean:
	@printf "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)\n"
	@rm -f $(OBJ_FILES)

format:
	clang-format -i -style=file src/*.cpp src/*.h

$(BINARY): $(OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -o $@ $(CXXFLAGS) $^ $(LIBS) -lstdc++

obj/%.o: src/%.cpp src/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

