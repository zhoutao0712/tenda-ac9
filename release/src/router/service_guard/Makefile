include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I. -I$(SRCBASE) -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/curl-7.21.7/include -I$(TOP)/json-c -DDEBUG
LDFLAGS = -lm -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared -L$(TOP)/openssl -lcrypto -lssl -L$(TOP)/curl-7.21.7/lib/.libs -lcurl -L$(TOP)/json-c/.libs -ljson-c
LDFLAGS += -lgcc_s

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/common/include
endif

OBJS_TINC = tinc-guard.o
OBJS_UPGRADE = upgrade.o
OBJS = $(OBJS_TINC)

all: tinc-guard upgrade

tinc-guard: $(OBJS_TINC)
	@echo " [tinc-guard] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_TINC)

	$(SIZECHECK)
	$(CPTMP)

upgrade: $(OBJS_UPGRADE)
	@echo " [upgrade] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_UPGRADE)

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f tinc-guard .*.depend
	rm -f upgrade .*.depend
	rm -f *.o

install: all
	@echo " [service_guard] Installing..."
	install -D tinc-guard $(INSTALLDIR)/bin/tinc-guard
	$(STRIP) $(INSTALLDIR)/bin/tinc-guard
	chmod 0500 $(INSTALLDIR)/bin/tinc-guard

	install -D upgrade $(INSTALLDIR)/bin/upgrade
	$(STRIP) $(INSTALLDIR)/bin/upgrade
	chmod 0500 $(INSTALLDIR)/bin/upgrade


%.o: %.c .%.depend
	@echo " [service_guard] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
