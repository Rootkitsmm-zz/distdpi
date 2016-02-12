
PROGRAM = distdpi

INCLUDEDIRS = \
	-Iinclude/ \
	-L../distdpibk2/distdpi/lib/ \
	-L../navl_4.3.1.29_linux-x86-64-release/lib/

LIBS =-lpthread 

#STD_LIBS =-lboost_serialization

CXXSOURCES = \
			 ./DistDpi.cpp \
             ./PacketHandler.cpp \
             ./FlowTable.cpp \
             ./DPIEngine.cpp \
			 ./DataPathUpdate.cpp \
             ./SignalHandler.cpp \
			 ./navl_externals_posix.c \
			 ./netx_sample_service.c \
             ./Main.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files
CXXFLAGS = $(INCLUDEDIRS)
CXX = g++ -fpermissive -g -ggdb -std=c++0x -DNETX_SDK_BUILD -Wall -DVMX86_SERVER

LDFLAGS = -lnetx -lnavl $(LIBDIRS) $(LIBS) $(STD_LIBS) $(DYNAMIC_LDFLAGS_LIBS)

all: $(PROGRAM)

$(PROGRAM): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

clean:
	$(RM) *.o $(PROGRAM)
