SYSROOT :=
FRAMEWORKSDIR := Library/Frameworks

override CFLAGS += -DPRIVATE -D__OS_EXPOSE_INTERNALS__ -fconstant-cfstrings -fstack-protector-all

all: SystemConfiguration-Extra scutil_extra configd_dnsinfo scselect

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
	$(CC) $(CURDIR)/scutil/*.c $(CFLAGS) $(LDFLAGS) \
	  -I$(CURDIR)/SystemConfiguration -I$(CURDIR)/libsystem_configuration -I$(CURDIR)/Plugins/common \
	  $(CURDIR)/Plugins/common/InterfaceNamerControlPrefs.c $(CURDIR)/Plugins/common/IPMonitorControlPrefs.c \
	  $(CURDIR)/SystemConfiguration-Extra \
	  -Wl,-framework,{CoreFoundation,SystemConfiguration} -ledit \
	  -o $@

configd_dnsinfo:
	$(CC) $(CURDIR)/configd/main.c $(CFLAGS) $(LDFLAGS) \
	  -Wl,-framework,{CoreFoundation,SystemConfiguration} \
	  -o $@

scselect:
	$(CC) $(CURDIR)/$@.tproj/$@.c $(CFLAGS) $(LDFLAGS) \
	  -Wl,-framework,{CoreFoundation,SystemConfiguration} \
	  -o $@

install: all
	install -d $(DESTDIR)/usr/sbin
	install -d $(DESTDIR)/usr/libexec
	install -d $(DESTDIR)/usr/share/man/man8
	install -d $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework

	# SystemConfiguration-Extra
	install -m644 SystemConfiguration/Info.plist $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework/
	install -m755 SystemConfiguration-Extra $(DESTDIR)/$(FRAMEWORKSDIR)/SystemConfiguration-Extra.framework/

	# scutil_extra
	install -m755 scutil_extra $(DESTDIR)/usr/sbin/
	install -m644 scutil/scutil.8 $(DESTDIR)/usr/share/man/man8/
	ln -s scutil.8 $(DESTDIR)/usr/share/man/man8/scutil_extra.8

	# SystemConfiguration
	install -d $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework
	install -m755 get-mobility-info $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework/
	install -m755 get-network-info $(DESTDIR)/System/Library/Frameworks/SystemConfiguration.framework/

	# configd_dnsinfo
	install -d $(DESTDIR)/Library/LaunchDaemons
	install -m644 configd/configd_dnsinfo.plist $(DESTDIR)/Library/LaunchDaemons/
	install -m755 configd_dnsinfo $(DESTDIR)/usr/libexec/
	install -m644 configd/configd_dnsinfo.8 $(DESTDIR)/usr/share/man/man8/
	install -m644 configd/configd.8 $(DESTDIR)/usr/share/man/man8/

	# scselect
	install -m755 scselect $(DESTDIR)/usr/sbin/
	install -m644 scselect.tproj/scselect.8 $(DESTDIR)/usr/share/man/man8/

clean:
	rm -f SystemConfiguration/helper.h SystemConfiguration/helperUser.c SystemConfiguration-Extra
	rm -f helper.h helperUser.c helperServer.c .generated_helper
