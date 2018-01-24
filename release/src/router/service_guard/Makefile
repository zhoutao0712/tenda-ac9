include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I. -I$(SRCBASE) -I$(SRCBASE)/include -I$(TOP)/shared -DDEBUG
LDFLAGS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared
LDFLAGS += -lgcc_s

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/common/include
endif

OBJS_TINC = tinc-guard.o
OBJS = $(OBJS_TINC)

all: tinc-guard

tinc-guard: $(OBJS_TINC)
	@echo " [tinc-guard] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_TINC)

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f tinc-guard .*.depend
	rm -f *.o

install: all
	@echo " [service_guard] Installing..."
	install -D tinc-guard $(INSTALLDIR)/bin/tinc-guard
	$(STRIP) $(INSTALLDIR)/bin/tinc-guard
	chmod 0500 $(INSTALLDIR)/bin/tinc-guard


%.o: %.c .%.depend
	@echo " [service_guard] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
