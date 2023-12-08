# Directories
TARGET  := bin
BUILD   := obj
SOURCES := src
# Find source files
export CFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export FULLCFILES   := $(addprefix $(SOURCES)/,$(CFILES))
export CPPFILES     := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export FULLCPPFILES := $(addprefix $(SOURCES)/,$(CPPFILES))
export OFILESC      := $(CFILES:.c=.o)
export OFILESCPP    := $(CPPFILES:.cpp=.o)
export OFILES       := $(OFILESC) $(OFILESCPP)
export FULLOFILES   := $(addprefix $(BUILD)/,$(OFILES))
# Flags and libraries
export CFLAGS       := -O3 -fopenmp
export CXXFLAGS     :=
export LINKLIBS     := -lavcodec -lavformat -lavutil -lswscale -lswresample
# Targets
all: $(BUILD) $(TARGET) $(TARGET)/videoanalogiser

$(BUILD):
	mkdir -p $(BUILD)
	make -f $(CURDIR)/Makefile

$(TARGET):
	mkdir -p $(TARGET)
	make -f $(CURDIR)/Makefile

$(TARGET)/videoanalogiser   : $(FULLOFILES)
	g++ $(CFLAGS) $(CXXFLAGS) -o $@ $^ $(LINKLIBS)

$(FULLOFILES)  : $(FULLCFILES) $(FULLCPPFILES)

$(BUILD)/%.o : $(SOURCES)/%.c
	gcc -c $(CFLAGS) -o $@ $<

$(BUILD)/%.o : $(SOURCES)/%.cpp
	g++ -c $(CFLAGS) $(CXXFLAGS) -o $@ $<
