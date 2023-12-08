# Directories
TARGET  := bin
BUILD   := obj
SOURCES := src
# Find source files
export CFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export FULLCFILES   := $(addprefix $(SOURCES)/,$(CFILES))
export CPPFILES     := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export FULLCPPFILES := $(addprefix $(SOURCES)/,$(CPPFILES))
export HEADERS      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.h)))
export FULLHEADERS  := $(addprefix $(SOURCES)/,$(HEADERS))
export OFILESC      := $(CFILES:.c=.o)
export OFILESCPP    := $(CPPFILES:.cpp=.o)
export OFILES       := $(OFILESC) $(OFILESCPP)
export FULLOFILES   := $(addprefix $(BUILD)/,$(OFILES))
# Flags and libraries
export CFLAGSBASE       := -fopenmp
export CFLAGSDEBUG      := $(CFLAGSBASE) -Og
export CFLAGSRELEASE    := $(CFLAGSBASE) -O3 -flto
export CFLAGS           := $(CFLAGSBASE)
export CXXFLAGS         :=
export LINKFLAGSBASE    :=
export LINKFLAGSDEBUG   := $(LINKFLAGSBASE)
export LINKFLAGSRELEASE := $(LINKFLAGSBASE) -Wl,-s
export LINKFLAGS        := $(LINKFLAGSBASE)
export LINKLIBS         := -lavcodec -lavformat -lavutil -lswscale -lswresample

# Define some variable overrides
release : CFLAGS = $(CFLAGSRELEASE)
native : CFLAGS = $(CFLAGSRELEASE) -march=native
debug : CFLAGS = $(CFLAGSBASE) -Og

release : LINKFLAGS = $(LINKFLAGSRELEASE)
native : LINKFLAGS = $(LINKFLAGSRELEASE)
debug : LINKFLAGS = $(LINKFLAGSDEBUG)

.PHONY : default native debug clean install-linux

all : default

#Builds for the current host OS, but assumes a basic variant of the current processor architecture
default : $(BUILD) $(TARGET) $(TARGET)/videoanalogiser

#Builds specifically for this machine
native : $(BUILD) $(TARGET) $(TARGET)/videoanalogiser

#Build a debug-friendly version
debug : $(BUILD) $(TARGET) $(TARGET)/videoanalogiser

#Cleans the build directory
clean : $(BUILD) $(TARGET)
	rm -rf $(BUILD)
	rm -rf $(TARGET)
	mkdir -p $(BUILD)
	mkdir -p $(TARGET)

#Copies output executable to /usr/bin/ (with confirmation)
install-linux : $(TARGET)/videoanalogiser
	sudo cp $^ /usr/bin/

$(BUILD) :
	mkdir -p $(BUILD)
	make -f $(CURDIR)/Makefile

$(TARGET) :
	mkdir -p $(TARGET)
	make -f $(CURDIR)/Makefile

$(TARGET)/videoanalogiser : $(FULLOFILES)
	g++ $(CFLAGS) $(CXXFLAGS) $(LINKFLAGS) -o $@ $^ $(LINKLIBS)

$(FULLOFILES) : $(FULLCFILES) $(FULLCPPFILES) $(FULLHEADERS)

$(BUILD)/%.o : $(SOURCES)/%.c
	gcc -c $(CFLAGS) -o $@ $<

$(BUILD)/%.o : $(SOURCES)/%.cpp
	g++ -c $(CFLAGS) $(CXXFLAGS) -o $@ $<
