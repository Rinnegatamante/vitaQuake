TARGET		:= vitaQuake
TITLE		:= QUAK00001
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include
GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

LIBS = -lvorbisfile -logg -lvorbis -lspeexdsp -lmpg123 \
	-lc -lvita2d -lSceCommonDialog_stub -lSceAudio_stub -lSceKernel_stub \
	-lSceNet_stub -lSceNetCtl_stub -lpng -lz -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub -lm \
	-lSceAppUtil_stub -lScePgf_stub -ljpeg -lSceRtc_stub -lScePower_stub \

COMMON_OBJS =	chase.o \
	cl_demo.o \
	cl_input.o \
	cl_main.o \
	cl_parse.o \
	cl_tent.o \
	cmd.o \
	common.o \
	console.o \
	crc.o \
	cvar.o \
	host.o \
	host_cmd.o \
	keys.o \
	mathlib.o \
	menu.o \
	net_dgrm.o \
	net_loop.o \
	net_main.o \
	net_vcr.o \
	pr_cmds.o \
	pr_edict.o \
	pr_exec.o \
	r_part.o \
	sbar.o \
	sv_main.o \
	sv_move.o \
	sv_phys.o \
	sv_user.o \
	view.o \
	wad.o \
	world.o \
	zone.o \
	sys_psp2.o \
	d_edge.o \
	d_fill.o \
	d_init.o \
	d_modech.o \
	d_part.o \
	d_polyse.o \
	d_scan.o \
	d_sky.o \
	d_sprite.o \
	d_surf.o \
	d_vars.o \
	draw.o \
	model.o \
	nonintel.o \
	r_aclip.o \
	r_alias.o \
	r_bsp.o \
	r_draw.o \
	r_edge.o \
	r_efrag.o \
	r_light.o \
	r_main.o \
	r_misc.o \
	r_sky.o \
	r_sprite.o \
	r_surf.o \
	r_vars.o \
	screen.o \
	snd_dma.o \
	snd_mix.o \
	snd_mem.o \
	snd_psp2.o \
	vid_psp2.o \
	net_bsd.o \
	net_udp_psp2.o \
	in_psp2.o \
	mods_psp2.o
	
CPPSOURCES	:= audiodec

CFILES	:= $(COMMON_OBJS)
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O3 -g -DHAVE_OGGVORBIS -DHAVE_MPG123 -DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER -DGIT_VERSION=\"$(GIT_VERSION)\"
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< ..\build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" param.sfo
	cp -f param.sfo ../build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip ../$(TARGET).vpk -r ..\build\sce_sys\* ..\build\eboot.bin 
	#-------------------------------------------------------------------
	
%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(TARGET).elf.unstripped.elf ../$(TARGET).vpk ../build/eboot.bin ../build/sce_sys/param.sfo ./param.sfo