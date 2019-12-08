include Makefile.src Makefile.comp

CFILES  =$(wildcard ./*.c)
LIB_OBJS=$(patsubst %.c,%.o,$(CFILES))
LIBS=mqtt.a
CINC+=-m32

.PHONY: all clean
all: $(LIBS)
	@cp *.a ../

%.a:$(LIB_OBJS)
	$(info |     [CC]   Creating library file $@ from $^)
	$(AR) cr lib$@ $^
	cp lib$@ ../../..

%.o:%.c
	$(CC) $(CFLAGS) $(CINC) -DMQTTS -o $@ -c $<

clean:
	@rm -f *.o *.a *.d
