CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -D NDEBUG -O3 -g
#CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -Og -g
BINARY = agbplay
LIBS = -lncursesw -lboost_system -lboost_filesystem -lsndfile -lportaudio

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_FILES = $(wildcard src/*.cpp)
OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_FILES:.cpp=.o)))

.PHONY: all
all: $(BINARY)

.PHONY: clean
clean:
	@printf "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)\n"
	@rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -o $@ $(CXXFLAGS) $^ $(LIBS) -lstdc++

obj/%.o: src/%.cpp src/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

