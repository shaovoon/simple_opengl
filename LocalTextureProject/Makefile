CC=emcc
SOURCES:=$(wildcard *.cpp)
LDFLAGS=-O2 --llvm-opts 2
OUTPUT=localtexture.js

all: $(SOURCES) $(OUTPUT)

$(OUTPUT): $(SOURCES) 
	$(CC) $(SOURCES) -g --bind -s FULL_ES2=1 -s USE_SDL=2 -s WASM=0 --memory-init-file 0 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s EXPORTED_FUNCTIONS="['_main', '_load_2nd_texture_from_memory']" -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -std=c++11 -I /mnt/d/App/SDL_App/ext/glm-0.9.8.4/ $(LDFLAGS) -o $(OUTPUT)

clean:
	rm $(OUTPUT)
