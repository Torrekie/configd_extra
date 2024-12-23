SYSROOT :=
FRAMEWORKSDIR := Library/Frameworks

CFLAGS :=

CFLAGS += -DPRIVATE -D__OS_EXPOSE_INTERNALS__

all: SystemConfiguration-Extra

.generated_helper:
	mig $(CURDIR)/SystemConfiguration/helper.defs && touch $(CURDIR)/.generated_helper

SystemConfiguration/helper.h: .generated_helper
	ln -s ../helper.h $(CURDIR)/SystemConfiguration/helper.h

SystemConfiguration/helperUser.c:
	ln -s ../helperUser.c $(CURDIR)/SystemConfiguration/helperUser.c

SystemConfiguration-Extra: SystemConfiguration/helper.h SystemConfiguration/helperUser.c
	$(CC) $(CURDIR)/SystemConfiguration/*.c $(CFLAGS) $(LDFLAGS) \
	  -dynamiclib \
	  -Wl,-framework,{CoreFoundation,IOKit,SystemConfiguration} \
	  -compatibility_version 1.0.0 -current_version 1109.100.4 \
	  -install_name $(SYSROOT)/$(FRAMEWORKSDIR)/$@.framework/$@ \
	  -o $@

clean:
	rm -f SystemConfiguration/helper.h SystemConfiguration/helperUser.c SystemConfiguration-Extra
	rm -f helper.h helperUser.c helperServer.c .generated_helper
