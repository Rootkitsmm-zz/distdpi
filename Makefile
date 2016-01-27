
PROGRAM = distdpi

INCLUDEDIRS = \
	-Iinclude/ \
	-Llib/

LIBS =-lpthread 

#STD_LIBS =-lboost_serialization

CXXSOURCES = \
             ./PacketHandler.cpp \
             ./Main.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files
CXXFLAGS = $(INCLUDEDIRS)
CXX = g++ -fpermissive -g -std=c++0x -DNETX_SDK_BUILD -Wall -DVMX86_SERVER

LDFLAGS = -lnetx $(LIBDIRS) $(LIBS) $(STD_LIBS) $(DYNAMIC_LDFLAGS_LIBS)

all: $(PROGRAM)

$(PROGRAM): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

clean:
	$(RM) $(CXXOBJECTS) $(PROGRAM)
