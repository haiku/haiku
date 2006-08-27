DEBUG = yes
#DEBUG = no

TARGET = 	HaikuMediaPlayer
SOURCES =	Controller.cpp \
			ControllerView.cpp \
			DrawingTidbits.cpp \
			SoundOutput.cpp \
			TransportButton.cpp \
			TransportControlGroup.cpp \
			MainApp.cpp \
			MainWin.cpp \
			Playlist.cpp \
			SeekSlider.cpp \
			VideoNode.cpp \
			VideoView.cpp \
			VolumeSlider.cpp

CPPFLAGS = -Wall -Wno-multichar -Wmissing-prototypes -Wpointer-arith -Wcast-align -Wsign-compare

LDFLAGS = -lmedia -lbe -ltracker

ARCHIVES =

CC=g++

OBJECTS = $(patsubst %.cpp, obj/%.o, $(SOURCES))


ifeq ($(DEBUG), yes)
	CPPFLAGS	+= -g -O0 -DDEBUG=1
	LDFLAGS += -g
else
# Application crashed when O1 or higher
	CPPFLAGS	+= -O0 
endif

CPPFLAGS += 

all: $(TARGET)

obj:
	@[ -d obj ] || mkdir obj > /dev/null 2>&1

#obj/%.o: %.cpp obj
obj/%.o: %.cpp 
	$(CC) -c $< $(INCLUDES) $(CPPFLAGS)  -o $@

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(ARCHIVES)
	chmod 0777 $@

run: $(TARGET)
	./$(TARGET) &

clean:
	rm -f ./$(TARGET)
	rm -f ./obj/*.o

