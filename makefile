CC = g++

CXXFLAGS = -g -Wall -std=c++0x -Ishared/include -Icontent-server/include -Imirror-initiator/include -Imirror-server/include -pthread

OUTP_MI = mirror-init
OUTP_MS = mirror-serv
OUTP_CS = content-serv

SRC_SHRD = shared/source/cmd_parser.cpp\
						shared/source/help_func.cpp\
						shared/source/my_string.cpp\
						shared/source/my_vector.cpp
						
SRC_CS = content-server/source/content_server.cpp\
					content-server/source/cs-main.cpp\
					content-server/source/hash_table.cpp
					
SRC_MS = mirror-server/source/mirror_manager.cpp\
					mirror-server/source/mirror_server.cpp\
					mirror-server/source/ms-main.cpp\
					mirror-server/source/queue.cpp\
					mirror-server/source/worker.cpp
					
SRC_MI = mirror-initiator/source/mi-main.cpp\
					mirror-initiator/source/mirror_init.cpp

OBJS_CS = ${SRC_CS:.cpp=.o} ${SRC_SHRD:.cpp=.o}
OBJS_MS = ${SRC_MS:.cpp=.o} ${SRC_SHRD:.cpp=.o}
OBJS_MI = ${SRC_MI:.cpp=.o} ${SRC_SHRD:.cpp=.o}

OBJS_ALL = ${OBJS_CS} ${OBJS_MS} ${OBJS_MI}

all : ms mi cs

ms : ${OBJS_MS}
	${CC} ${CXXFLAGS} -o ${OUTP_MS} ${OBJS_MS}

mi : ${OBJS_MI}
	${CC} ${CXXFLAGS} -o ${OUTP_MI} ${OBJS_MI}

cs : ${OBJS_CS}
	${CC} ${CXXFLAGS} -o ${OUTP_CS} ${OBJS_CS}
	
clean : 
	@rm */*/*.o
	
clean_all :
	@rm */*/*.o
	@rm ${OUTP_CS} ${OUTP_MI} ${OUTP_MS}
	

# Mirror Server
mirror_manager.o : mirror_manager.cpp constants.h help_func.h mirror_manager.h
mirror_server.o : mirror_server.cpp mirror_server.h constants.h worker.h
ms-main.o : ms-main.cpp cmd_parser.h constants.h mirror_server.h
queue.o : queue.cpp queue.h constants.h my_string.h
worker.o : worker.cpp worker.h help_func.h constants.h

# Mirror Initiator
mi-main.o : mi-main.cpp cmd_parser.h constants.h mirror_init.h
mirror_init.o : mirror_init.cpp mirror_init.h help_func.h constants.h

# Content Server
cs-main.o : cs-main.cpp cmd_parser.h constants.h content_server.h
content_server.o : content_server.cpp constants.h content_server.h help_func.h
hash_table.o : hash_table.cpp hash_table.h my_string.h

# Shared
cmd_parser.o : cmd_parser.cpp cmd_parser.h constants.h
help_func.o : help_func.cpp help_func.h my_string.h constants.h
my_string.o : my_string.cpp my_string.h my_vector.h
my_vector.o : my_vector.cpp my_vector.h my_string.h
