SYSROOT :=
FRAMEWORKSDIR := Library/Frameworks

CFLAGS :=

CFLAGS += -DPRIVATE -D__OS_EXPOSE_INTERNALS__

all: SystemConfiguration-Extra scutil_extra

.generated_helper:
	mig $(CURDIR)/SystemConfiguration/helper.defs && touch $(CURDIR)/.generated_helper

SystemConfiguration/helper.h: .generated_helper
	cp $(CURDIR)/helper.h $(CURDIR)/SystemConfiguration/helper.h

SystemConfiguration/helperUser.c:
	cp $(CURDIR)/helperUser.c $(CURDIR)/SystemConfiguration/helperUser.c

SystemConfiguration-Extra: SystemConfiguration/helper.h SystemConfiguration/helperUser.c
	$(CC) $(CURDIR)/SystemConfiguration/*.c $(CFLAGS) $(LDFLAGS) \
	  -dynamiclib \
	  -Wl,-framework,{CoreFoundation,IOKit,SystemConfiguration} \
	  -compatibility_version 1.0.0 -current_version 1109.100.4 \
	  -install_name $(SYSROOT)/$(FRAMEWORKSDIR)/$@.framework/$@ \
	  -o $@

scutil_extra: SystemConfiguration-Extra
	$(CC) $(CURDIR)/scutil/*.c -fconstant-cfstrings -fstack-protector-all $(CFLAGS) $(LDFLAGS) \
	  -I$(CURDIR)/SystemConfiguration -I$(CURDIR)/libsystem_configuration -I$(CURDIR)/Plugins/common \
	  $(CURDIR)/Plugins/common/InterfaceNamerControlPrefs.c $(CURDIR)/Plugins/common/IPMonitorControlPrefs.c \
	  $(CURDIR)/SystemConfiguration-Extra \
	  -Wl,-framework,{CoreFoundation,SystemConfiguration} -ledit \
	  -o scutil_extra

install: all
	install -d $(DESTDIR)/usr/sbin
	install -d $(DESTDIR)/usr/share/man/man8
	install -d $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework

	install -m644 SystemConfiguration/Info.plist $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework/
	install -m755 SystemConfiguration-Extra $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework/
	install -m755 scutil_extra $(DESTDIR)/usr/sbin/
	install -m644 scutil/scutil.8 $(DESTDIR)/usr/share/man/man8/
	ln -s scutil.8 $(DESTDIR)/usr/share/man/man8/scutil_extra.8
	install -d $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework
	install -m755 get-mobility-info $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework/
	install -m755 get-network-info $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework/

clean:
	rm -f SystemConfiguration/helper.h SystemConfiguration/helperUser.c SystemConfiguration-Extra
	rm -f helper.h helperUser.c helperServer.c .generated_helper
