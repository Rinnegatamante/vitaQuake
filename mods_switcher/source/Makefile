TARGET		:= vitaQuake_Mod_Selector
APP_NAME	:= vitaQuake Mod Selector
TITLE		:= QUAK00002
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

LIBS = -lc -lvita2d -lSceCommonDialog_stub -lSceKernel_stub \
	-lpng -lz -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lm \
	-lScePgf_stub -ljpeg -lScePower_stub \

COMMON_OBJS =	main.o
	
CPPSOURCES	:= .

CFILES	:= $(COMMON_OBJS)
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O3
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself $< ..\build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(APP_NAME)" param.sfo
	cp -f param.sfo ../build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip ../$(TARGET).vpk -r ..\build\sce_sys\* ..\build\eboot.bin 
	#-------------------------------------------------------------------
	
%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)

run: $(TARGET).velf
	@sh run_homebrew_unity.sh $(TARGET).velf