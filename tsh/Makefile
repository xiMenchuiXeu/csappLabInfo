CC = g++
CFLAGS   =

EXECUTABLE = tsh_change
DEBUG_EXECUTABLE = tsh_change_debug
SOURCES    = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
INCLUDE    = 
release: CFLAGS += -O3 -DNDEBUG 
release: $(EXECUTABLE)      

debug: CFLAGS += -g3 -DDEBUG 
debug: $(DEBUG_EXECUTABLE)

all: release debug 
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS)  -o $(EXECUTABLE) 

$(DEBUG_EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(DEBUG_EXECUTABLE)

.PHONY : clean 
clean : 
	-rm -f $(OBJECTS) $(EXECUTABLE) $(DEBUG_EXECUTABLE)

base.o: base.cpp base.h
get_num.o: get_num.cpp get_num.h
job_change.o: job_change.cpp job_change.h base.h get_num.h sio.h
signalHandle.o: signalHandler_change.cpp signalHandler_change.h base.h job_change.h get_num.h sio.h
