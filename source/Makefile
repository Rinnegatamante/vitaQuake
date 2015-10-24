TARGET		:= vitaQuake
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

LIBS = -lvita2d -lSceAudio_stub -lSceKernel_stub \
	-lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub -lm \
	-lScePgf_stub -ljpeg -lSceRtc_stub -lc -lScePower_stub \

COMMON_OBJS =	chase.o \
	cl_demo.o \
	cl_input.o \
	cl_main.o \
	cl_parse.o \
	cl_tent.o \
	cmd.o \
	console_psp2.o \
	draw_psp2.o \
	font_data.o \
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
	d_zpoint.o \
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
	net_none.o \
	in_psp2.o \
	cd_null.o	
	
CFILES		:=	$(COMMON_OBJS)
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -Wall -O3
CXXFLAGS  = $(CFLAGS) -fno-exceptions
ASFLAGS = $(CFLAGS)

all: $(TARGET).velf

%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@ $(VITASDK)/bin/db.json

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)

run: $(TARGET).velf
	@sh run_homebrew_unity.sh $(TARGET).velf