INCLUDES 	 = -I./include/live555/usageEnvironment/ -I./include/live555/groupsock/ \
				-I./include/live555/liveMedia/ -I./include/live555/basicUsageEnvironment \
				-I./include/imp_sys
LIVE555_LIBS =  ./lib/livelib/libliveMedia.a ./lib/livelib/libgroupsock.a \
				./lib/livelib/libBasicUsageEnvironment.a ./lib/livelib/libUsageEnvironment.a
SDK_LIB_DIR	=  ./lib/imp_sys/uclibc
IMP_LIBS	= $(SDK_LIB_DIR)/libimp.a $(SDK_LIB_DIR)/libalog.a
LIBS	=  $(LIVE555_LIBS) $(IMP_LIBS)

CROSS_COMPILE?= mips-linux-uclibc-gnu-

COMPILE_OPTS =      $(INCLUDES) -I. -O2 -Wall -march=mips32r2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DLOCALE_NOT_USED -g
C 			 =         c
C_COMPILER   =	$(CROSS_COMPILE)gcc
C_FLAGS 	 =       $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP 		 =           cpp
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS =   $(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ 		 =           o
LINK 		 =  $(CROSS_COMPILE)g++ -o
#LINK_OPTS    =  -ldl  -lm -lpthread -ldl -g
LINK_OPTS    =  -lpthread -lm -lrt -ldl
CONSOLE_LINK_OPTS = $(LINK_OPTS)
LINK_OBJ	 = imp-common.o capture_and_encoding.o on_demand_rtsp_server.o 

APP = t20-rtspd

commit_tag=$(shell git rev-parse --short HEAD)
.PHONY:all
all : version $(APP)
version :
	@if  ! grep "$(commit_tag)" version.h >/dev/null ; then                   \
        echo "update version.h"       ;    \
        sed 's/COMMIT_TAG/"$(commit_tag)"/g' version.tpl.h > version.h     ;  \
    fi

$(APP):
.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<
$(APP): $(LINK_OBJ)
	$(LINK)$@  $(LINK_OBJ)  $(LIBS) $(CONSOLE_LINK_OPTS)
	cp $(APP) ~/tftproot/

clean:
	-rm -rf *.$(OBJ) $(APP) core *.core *~ include/*~ version.h

