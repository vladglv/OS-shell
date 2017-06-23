# Compiler options
CC=clang++ -g -O0 -fPIC -D_XOPEN_SOURCE=700 -std=c++14 -Wmain -W -Wall -Wthread-safety -pedantic -pedantic-errors
CCS=-fsanitize=address,undefined,leak

# Executable names
EXECUTABLE=shell
EXECUTABLE_VG=shell_vg
EXECUTABLELIST= $(EXECUTABLE) $(EXECUTABLE_VG) 

# Sources
SOURCES=shell.cpp

# Targets
all:
	$(CC) $(CCS) -o $(EXECUTABLE) $(SOURCES)
	$(CC) -o $(EXECUTABLE_VG) $(SOURCES)

clean:
	rm $(EXECUTABLELIST)
