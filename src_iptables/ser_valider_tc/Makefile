CC = gcc
LD = gcc
CCFLAGS = 
LDFLAGS = 
DEBUGFLAGS = -O0 -g3 -Wall
#DEBUGFLAGS = 
LOCALINCLUDES = 
LOCALLIBS = 
LIBDIRS = 
DEP = Makefile.dep

TARGET = valider_tc
SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

.c.o:
	$(CC) $(DEBUGFLAGS) $(LOCALINCLUDES) $(CCFLAGS) -c $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(DEBUGFLAGS)  $(LDFLAGS) $(LOCALLIBS) $(LIBDIRS) $(LIB) -o $(TARGET) $(OBJS)

include $(DEP)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_LIB) *.c~ *.h~

dependencies:
	$(CC) -MM $(CCFLAGS) $(SRC) > $(DEP)

