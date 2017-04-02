CXXFLAGS =	-O3 -g -Wall -fmessage-length=0

OBJS =		PPU700.o

LIBS =

TARGET =	PPU700

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
