CC=emcc
SOURCES:=$(wildcard *.cpp)
LDFLAGS=-O2 --llvm-opts 2
OUTPUT=simple.html

all: $(SOURCES) $(OUTPUT)

$(OUTPUT): $(SOURCES) 
	$(CC) $(SOURCES) --bind -s WASM=0 --memory-init-file 0 -s USE_WEBGL2=0 -s USE_SDL=2 -std=c++11 $(LDFLAGS) -o $(OUTPUT)

clean:
	rm $(OUTPUT)
