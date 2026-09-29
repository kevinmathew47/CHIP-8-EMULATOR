CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs)
TARGET = chip8
SOURCES = src/main.cpp src/chip8.cpp src/text.cpp src/disasm.cpp src/debugger.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

WEB_FLAGS = -std=c++17 -O2 -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
            -sEXPORTED_FUNCTIONS=_main,_web_load_rom,_web_hotkey,_web_set_key,_web_set_rewind,_web_get_info \
            -sEXPORTED_RUNTIME_METHODS=ccall,FS
web: $(SOURCES) web/shell.html
	mkdir -p web/build/assets
	cp screenshots/01_bugfix_before_after.png web/build/assets/bugfix.png
	em++ $(WEB_FLAGS) --preload-file roms --preload-file tests/roms@test-roms \
	     --shell-file web/shell.html -o web/build/index.html $(SOURCES)
	mkdir -p docs && cp -r web/build/. docs/ && touch docs/.nojekyll

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean test web
TEST_CORE = src/chip8.cpp src/disasm.cpp
TEST_FLAGS = -std=c++17 -Wall -Wextra -O2

tests/headless_test: tests/headless_test.cpp $(TEST_CORE)
	$(CXX) $(TEST_FLAGS) -o $@ tests/headless_test.cpp src/chip8.cpp

tests/savestate_test: tests/savestate_test.cpp $(TEST_CORE)
	$(CXX) $(TEST_FLAGS) -o $@ tests/savestate_test.cpp src/chip8.cpp

tests/disasm_test: tests/disasm_test.cpp src/disasm.cpp
	$(CXX) $(TEST_FLAGS) -o $@ tests/disasm_test.cpp src/disasm.cpp

tests/keypad_test: tests/keypad_test.cpp $(TEST_CORE)
	$(CXX) $(TEST_FLAGS) -o $@ tests/keypad_test.cpp src/chip8.cpp

test: tests/headless_test tests/savestate_test tests/disasm_test tests/keypad_test
	@set -e; \
	check(){ ./tests/headless_test tests/roms/$$2.ch8 $$3 $$4 $$5 2>/dev/null | grep -v "^Loaded ROM" > tests/out_$$1.txt; \
	         if [ "$$(cat tests/out_$$1.txt)" = "$$(cat tests/expected/$$1.txt)" ]; then echo "PASS: $$1"; \
	         else echo "FAIL: $$1 (see tests/out_$$1.txt)"; exit 1; fi; rm -f tests/out_$$1.txt; }; \
	check 2-ibm-logo                2-ibm-logo  100  0 chip8;  \
	check 3-corax+                  3-corax+    500  0 chip8;  \
	check 4-flags                   4-flags     500  0 chip8;  \
	check 5-quirks                  5-quirks    3000 1 cosmac; \
	check 5-quirks-schip            5-quirks    3000 2 schip;  \
	check 5-quirks-xochip           5-quirks    3000 3 xochip; \
	check 8-scrolling-schip-lores   8-scrolling 600  1 schip;  \
	check 8-scrolling-schip-hires   8-scrolling 600  3 schip;  \
	check 8-scrolling-xochip-lores  8-scrolling 600  4 xochip; \
	check 8-scrolling-xochip-hires  8-scrolling 600  5 xochip; \
	./tests/savestate_test tests/roms/3-corax+.ch8 | tail -n 1; \
	./tests/disasm_test; \
	./tests/keypad_test
