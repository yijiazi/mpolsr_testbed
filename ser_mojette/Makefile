CC = gcc
LD = gcc
CCFLAGS = -fPIC
LDFLAGS = -shared -ldl
#DEBUGFLAGS = -g -Wall
DEBUGFLAGS = 
LOCALINCLUDES = 
LOCALLIBS = 
LIBDIRS = 
DEP = Makefile.dep

TARGET = ser_mojette
TARGET_LIB = $(TARGET).so
SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

.c.o:
	$(CC) $(DEBUGFLAGS) $(LOCALINCLUDES) $(CCFLAGS) -c $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(LOCALLIBS) $(LIBDIRS) $(LIB) -o $(TARGET_LIB) $(OBJS)

include $(DEP)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_LIB) *.c~ *.h~

dependencies:
	$(CC) -MM $(CCFLAGS) $(SRC) > $(DEP)

