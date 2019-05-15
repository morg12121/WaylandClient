#OPTIONS=-Wall -Werror -std=c99 -pedantic
OPTIONS=-Wall -Werror
DISABLEDWARNINGS=-Wno-unused-variable -Wno-unused-function
OPTIMIZATIONS=-O0
CODEFLAGS=
LIBRARIES=-lwayland-client -lrt
#LIBRARIES=-lLIBRARY
#CODEFLAGS=-DFLAG

FILE_MAIN=main.c
FILE_OUTPUT=WaylandClient

build:
	gcc $(FILE_MAIN) -o $(FILE_OUTPUT) $(OPTIONS) $(DISABLEDWARNINGS) $(OPTIMIZATION) $(CODEFLAGS) $(LIBRARIES)
	chmod +x $(FILE_OUTPUT)

run:
	./$(FILE_OUTPUT)

