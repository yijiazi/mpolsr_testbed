INCLUDES = -I/usr/local/include/boost-1_38
METASOURCES = AUTO
bin_PROGRAMS = ser_routing
ser_routing_SOURCES = common.c common.h configfile.cpp hook_local_in.cpp \
	hook_local_out.cpp hook_pre_routing.cpp ippacket.cpp mpolsrpacket.cpp olsreventclient.cpp \
	olsreventclient.h olsreventmessage.h rawsocket.cpp rawsocket.h ser_hooks.h ser_path.c \
	ser_path.h ser_path_def.h ser_path_djk.c ser_path_tc.c ser_routing.cpp \
	tcpconnection.h
ser_routing_LDADD = /usr/local/lib/libboost_date_time-gcc41-mt.so \
	/usr/local/lib/libboost_serialization-gcc41-mt.so /usr/local/lib/libboost_system-gcc41-mt.so \
	/usr/local/lib/libboost_thread-gcc41-mt.so /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a
noinst_HEADERS = configfile.h ippacket.h mpolsrpacket.h
ser_routing_LDFLAGS = -lpthread
