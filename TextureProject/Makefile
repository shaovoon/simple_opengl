CC=emcc
SOURCES:=$(wildcard *.cpp)
LDFLAGS=-O2 --llvm-opts 2
OUTPUT=opengl.html

all: $(SOURCES) $(OUTPUT)

$(OUTPUT): $(SOURCES) 
	$(CC) $(SOURCES) -g --bind -s FULL_ES2=1 -s USE_SDL=2 -s WASM=0 --memory-init-file 0 -s USE_SDL_IMAGE=2 -D __EMSCRIPTEN__ -std=c++11 -I /mnt/d/App/SDL_App/ext/glm-0.9.8.4/ $(LDFLAGS) -o $(OUTPUT)

clean:
	rm $(OUTPUT)
