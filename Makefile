CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -O3 -g
#CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++17 -Og -g -fsanitize=address
NCURSES = `pkg-config --cflags --libs ncurses`
NCURSESW = `pkg-config --cflags --libs ncursesw`
PORTAUDIO = `pkg-config --cflags --libs portaudiocpp`
SNDFILE = `pkg-config --cflags --libs sndfile`
JSONCPP = `pkg-config --cflags --libs jsoncpp`
BINARY = agbplay
LIBS = $(NCURSESW) $(PORTAUDIO) $(SNDFILE) $(JSONCPP)
# Use this macro if you have linker errors with ncursesw
LIBS = $(NCURSES) $(PORTAUDIO) $(SNDFILE) $(JSONCPP)

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_FILES = $(wildcard src/*.cpp)
OBJ_FILES = $(addprefix obj/,$(notdir $(SRC_FILES:.cpp=.o)))

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

$(BINARY): $(OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@$(CXX) -o $@ $(CXXFLAGS) $^ $(LIBS)

obj/%.o: src/%.cpp src/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMPORT)
