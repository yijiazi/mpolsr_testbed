# Script adapted for compilation cpp files from sereadmo plugin
# Author Ricardo T. Macedo
# Date: 16 January 2013

#Compilation from olsr_plugin.c
gcc -Wall -Wextra -Wold-style-definition -Wdeclaration-after-statement -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wsign-compare -Waggregate-return -Wmissing-noreturn -Wmissing-format-attribute -Wno-multichar -Wno-deprecated-declarations -Wendif-labels -Wwrite-strings -Wbad-function-cast -Wpointer-arith -Wcast-qual -Wnested-externs -Winline -Wdisabled-optimization -finline-functions-called-once -fearly-inlining -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/olsrd_plugin.o src/olsrd_plugin.c

#Compilation from ser_data.c
gcc -Wall -Wextra -Wold-style-definition -Wdeclaration-after-statement -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wsign-compare -Waggregate-return -Wmissing-noreturn -Wmissing-format-attribute -Wno-multichar -Wno-deprecated-declarations -Wendif-labels -Wwrite-strings -Wbad-function-cast -Wpointer-arith -Wcast-qual -Wnested-externs -Winline -Wdisabled-optimization -finline-functions-called-once -fearly-inlining -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/ser_data.o src/ser_data.c

#Compilation from ser_print.c
gcc -Wall -Wextra -Wold-style-definition -Wdeclaration-after-statement -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wsign-compare -Waggregate-return -Wmissing-noreturn -Wmissing-format-attribute -Wno-multichar -Wno-deprecated-declarations -Wendif-labels -Wwrite-strings -Wbad-function-cast -Wpointer-arith -Wcast-qual -Wnested-externs -Winline -Wdisabled-optimization -finline-functions-called-once -fearly-inlining -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/ser_print.o src/ser_print.c

#Compilation from ser_tc.c
gcc -Wall -Wextra -Wold-style-definition -Wdeclaration-after-statement -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wsign-compare -Waggregate-return -Wmissing-noreturn -Wmissing-format-attribute -Wno-multichar -Wno-deprecated-declarations -Wendif-labels -Wwrite-strings -Wbad-function-cast -Wpointer-arith -Wcast-qual -Wnested-externs -Winline -Wdisabled-optimization -finline-functions-called-once -fearly-inlining -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/ser_tc.o src/ser_tc.c

#Compilation from eventlistener.cpp
g++ -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/eventlistener.o src/eventlistener.cpp

#Compilation from olsreventserver.cpp
g++ -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/olsreventserver.o src/olsreventserver.cpp

#Compilation from ser_device.cpp
g++ -fomit-frame-pointer -finline-limit=350   -fPIC -Isrc -I../../src -pthread -DOLSR_PLUGIN  -DUSE_FPM -Dlinux -DNDEBUG    -c -o src/ser_device.o src/ser_device.cpp

# Link amog all objects
# gcc -shared -Wl,-soname,olsrd_sereadmo -Wl,--version-script=version-script.txt  -Wl,--warn-common -fPIC -o olsrd_sereadmo.so.0.1 src/olsrd_plugin.o src/ser_data.o src/ser_print.o src/ser_tc.o src/ser_device.o src/olsreventserver.o src/eventlistener.o -lpthread

g++ -shared -Wl,-soname,olsrd_sereadmo -Wl,--version-script=version-script.txt  -Wl,--warn-common -fPIC -o olsrd_sereadmo.so.0.1 src/olsrd_plugin.o src/ser_data.o src/ser_print.o src/ser_tc.o src/ser_device.o src/olsreventserver.o src/eventlistener.o /usr/local/lib/libboost_thread-gcc46-mt.so /usr/local/lib/libboost_serialization-gcc46-mt.so /usr/local/lib/libboost_system-gcc46-mt.so -lpthread
