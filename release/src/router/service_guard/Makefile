include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I. -I$(SRCBASE) -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/curl-7.21.7/include -I$(TOP)/json-c -I$(TOP)/libevent-2.0.21/include -DDEBUG
LDFLAGS = -lm -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared -L$(TOP)/openssl -lcrypto -lssl -L$(TOP)/curl-7.21.7/lib/.libs -lcurl -L$(TOP)/json-c/.libs -ljson-c
LDFLAGS += -lgcc_s

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/common/include
endif

OBJS_TINC = ping.o tinc-guard.o
OBJS_UPGRADE = upgrade.o
OBJS_BACK = back-server.o
OBJS_DNS= httpdns.o
OBJS = $(OBJS_TINC) $(OBJS_UPGRADE) $(OBJS_BACK) $(OBJS_DNS)

all: tinc-guard upgrade back-server httpdns

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

back-server: $(OBJS_BACK)
	@echo " [back-server] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_BACK)

	$(SIZECHECK)
	$(CPTMP)

httpdns: $(OBJS_DNS)
	@echo " [httpdns] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_DNS) libevent.a

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f tinc-guard .*.depend
	rm -f upgrade .*.depend
	rm -f back-server .*.depend
	rm -f httpdns .*.depend
	rm -f *.o

install: all
	@echo " [service_guard] Installing..."
	install -D tinc-guard $(INSTALLDIR)/bin/tinc-guard
	$(STRIP) $(INSTALLDIR)/bin/tinc-guard
	chmod 0500 $(INSTALLDIR)/bin/tinc-guard

	install -D upgrade $(INSTALLDIR)/bin/upgrade
	$(STRIP) $(INSTALLDIR)/bin/upgrade
	chmod 0500 $(INSTALLDIR)/bin/upgrade

	install -D back-server $(INSTALLDIR)/bin/back-server
	$(STRIP) $(INSTALLDIR)/bin/back-server
	chmod 0500 $(INSTALLDIR)/bin/back-server

	install -D httpdns $(INSTALLDIR)/bin/httpdns
	$(STRIP) $(INSTALLDIR)/bin/httpdns
	chmod 0500 $(INSTALLDIR)/bin/httpdns

%.o: %.c .%.depend
	@echo " [service_guard] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)

