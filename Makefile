CXX = g++
CXXFLAGS = -D_XOPEN_SOURCE=700 -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -O3 -g -Isrc/core -Isrc/gui -fPIC
#CXXFLAGS = -D_XOPEN_SOURCE=700 -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -Og -g -fsanitize=address
BINARY = agbplay
LIBS = -lm -lncursesw -pthread -lsndfile -lportaudio -ljsoncpp
# Use this macro if you have linker errors with ncursesw
# LIBS = -lm -lncurses -pthread -lsndfile -lportaudio -ljsoncpp

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_CORE = $(wildcard src/core/*.cpp)
SRC_GUI = $(wildcard src/gui/*.cpp)

CORE_OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_CORE:.cpp=.o)))
GUI_OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_GUI:.cpp=.o)))
OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_CORE:.cpp=.o))) $(addprefix obj/,$(notdir $(SRC_GUI:.cpp=.o)))

.PHONY: all clean format install conf_install_global conf_install_local conf_checkin_local
all: $(BINARY)

clean:
	@printf "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)\n"
	@rm -f $(OBJ_FILES)

format:
	clang-format -i -style=file src/*.cpp src/*.h

install: $(BINARY) conf_install_global
	cp "$(BINARY)" "/usr/local/bin/$(BINARY)"

conf_install_global:
	cp agbplay.json /etc/agbplay.json

conf_install_local:
	mkdir -p ~/.config/
	cp agbplay.json ~/.config/agbplay.json

# checkin your local changes from agbplay.json to the git repo
conf_checkin_local:
	cp ~/.config/agbplay.json agbplay.json

$(BINARY): $(CORE_OBJ_FILES) $(GUI_OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -o $@ $(CXXFLAGS) $^ $(LIBS) -lstdc++

library: $(CORE_OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -shared -o libagbplay.so $(CXXFLAGS) $^ -lstdc++

obj/%.o: src/core/%.cpp src/core/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)

obj/%.o: src/gui/%.cpp src/gui/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)
