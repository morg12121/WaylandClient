#OPTIONS=-Wall -Werror -std=c99 -pedantic
OPTIONS=-Wall -Werror
DISABLEDWARNINGS=-Wno-unused-variable -Wno-unused-function
OPTIMIZATIONS=-O0
CODEFLAGS=
LIBRARIES=-lrt
#CODEFLAGS=-DFLAG

WAYLAND_FLAGS = $(shell pkg-config wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell pkg-config wayland-protocols --variable=pkgdatadir)
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)
XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
XDG_SHELL_FILES=xdg-shell-client-protocol.h xdg-shell-protocol.c

FILE_MAIN=main.c
FILE_OUTPUT=WaylandClient

build:
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) xdg-shell-client-protocol.h
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) xdg-shell-protocol.c
	gcc $(FILE_MAIN) -o $(FILE_OUTPUT) $(OPTIONS) $(DISABLEDWARNINGS) $(OPTIMIZATION) $(CODEFLAGS) $(LIBRARIES) $(WAYLAND_FLAGS)
	chmod +x $(FILE_OUTPUT)

run:
	./$(FILE_OUTPUT)

