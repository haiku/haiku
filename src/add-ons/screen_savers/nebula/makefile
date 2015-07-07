#include $(TOP)/config/make.cfg

CFLAGS	= -O3 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations
#CFLAGS = -g

LIBS	= -lbe -lscreensaver
NAME	= Nebula
FILES 	= Nebula
ASMFILES = draw
STARFILE = draw_stars
STARWIDTHS = 320 512 576 640 800 1024 1152 1280 1400 1600
OBJDIR	= obj.x86

STARFILES	:= $(foreach f, $(STARWIDTHS), $(STARFILE)$(f))

SRCS 		:= $(foreach f, $(FILES), $(f).cpp)
ASMSRCS		:= $(foreach f, $(ASMFILES), $(f).S)

OBJS 		:= $(foreach f, $(FILES) $(ASMFILES) $(STARFILES), $(OBJDIR)/$(f).o)

all:	$(NAME)

install: all
	@mv $(NAME) /boot/home/config/add-ons/Screen\ Savers/

test: all
	@screen_blanker /boot/home/develop/div/ScreenSavers/$(NAME)/$(NAME)

clean:
	@rm $(OBJDIR)/*

Nebula: $(OBJS)
	@echo "Linking $@..."
	@$(CC) $(OBJS) -o $@ -nostart $(LIBS)
	xres -o $@ $@.rsrc
	mimeset -f $@

$(OBJDIR)/%.o : %.cpp
	$(CC) -c $*.cpp -o $(OBJDIR)/$*.o $(CFLAGS)

$(OBJDIR)/$(STARFILE)%.o : $(STARFILE).S
	$(CC) -c $(STARFILE).S -DSTAR_WIDTH=$* -DSTAR_FUNCTION=draw_stars$* -o $@

$(OBJDIR)/%.o : %.S
	$(CC) -c $*.S -o $(OBJDIR)/$*.o  $(CFLAGS)

tar zip backup:
	@zip `basename $(NAME)`-`date +%Y-%m-%d`.zip *.[chS]* *.rsrc makefile
