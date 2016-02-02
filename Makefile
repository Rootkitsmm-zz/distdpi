
PROGRAM = distdpi

INCLUDEDIRS = \
	-Iinclude/ \
	-L../distdpibk2/distdpi/lib/ \
	-L../navl_4.3.0.36_linux-x86-64-release/flowaware/

LIBS =-lpthread 

#STD_LIBS =-lboost_serialization

CXXSOURCES = \
             ./PacketHandler.cpp \
             ./FlowTable.cpp \
             ./DPIEngine.cpp \
			 ./navl_externals_posix.c \
             ./Main.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files
CXXFLAGS = $(INCLUDEDIRS)
CXX = g++ -fpermissive -g -ggdb -std=c++0x -DNETX_SDK_BUILD -Wall -DVMX86_SERVER

LDFLAGS = -lnetx -lnavl $(LIBDIRS) $(LIBS) $(STD_LIBS) $(DYNAMIC_LDFLAGS_LIBS)

all: $(PROGRAM)

$(PROGRAM): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

clean:
	$(RM) $(CXXOBJECTS) $(PROGRAM)
