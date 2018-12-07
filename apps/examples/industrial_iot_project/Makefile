-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

APPNAME = industrial_iot
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = 2048
ASRCS =
CFLAGS += -Wno-error=strict-prototypes
CFLAGS += -I$(TOPDIR)/include
CFLAGS += -I$(TOPDIR)/include/apps/artikapi/base

CFLAGS += -I$(TOPDIR)/include/apps/artikapi/systemio
CFLAGS += -I$(TOPDIR)/include/apps/artikapi/connectivity
CFLAGS += -I$(TOPDIR)/include/apps/artikapi/wifi
CFLAGS += -I$(TOPDIR)/net
CFLAGS += -I$(APPDIR)/include
CFLAGS += -I$(APPDIR)/include/netutils
CSRCS += cloud_communication.c
CSRCS += industrial_iot_wifi.c
CSRCS += industrial_iot_adc.c
CSRCS += industrial_iot_routine.c
CSRCS += industrial_iot_pwm.c
CSRCS += industrial_iot_rest.c
CSRCS += cloud_config.c
CSRCS += api/module-api.c api/wifi-api.c
CSRCS += api/gpio-api.c api/pwm-api.c api/adc-api.c
CSRCS += api/http-api.c api/cloud-api.c api/websocket-api.c api/demo_buzz.c
MAINSRC = industrial_iot_main.c

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
  OBJS += $(MAINOBJ)
endif

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  BIN = ..\\..\\libapps$(LIBEXT)
else
  BIN = ../../libapps$(LIBEXT)
endif
endif

ifeq ($(WINTOOL),y)
  INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  INSTALL_DIR = $(BIN_DIR)
endif

CONFIG_EXAMPLES_ARTIK051_PROGNAME ?= artik051$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_ARTIK051_PROGNAME)

ROOTDEPPATH = --dep-path . --dep-path ./artik051_selftest

ifeq ($(CONFIG_SCSC_WLAN),y)
CFLAGS+=-I$(APPDIR)/examples/slsidemo
endif

# Common build
VPATH = 

all: .built
.PHONY: clean depend distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	@touch .built

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(OBJS) $(MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),$(APPNAME)_main)

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat
else
context:
endif

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
-include Make.dep
