# The olsr.org Optimized Link-State Routing daemon(olsrd)
# Copyright (c) 2004, Andreas Tønnesen(andreto@olsr.org)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met:
#
# * Redistributions of source code must retain the above copyright 
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright 
#   notice, this list of conditions and the following disclaimer in 
#   the documentation and/or other materials provided with the 
#   distribution.
# * Neither the name of olsr.org, olsrd nor the names of its 
#   contributors may be used to endorse or promote products derived 
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Visit http://www.olsr.org for more information.
#
# If you find this software useful feel free to make a donation
# to the project. For more information see the website or contact
# the copyright holders.
#

# Please also write a new version to:
# gui/win32/Main/Frontend.rc (line 71, around "CAPTION [...]")
# gui/win32/Inst/installer.nsi (line 57, around "MessageBox MB_YESNO [...]")
VERS =		0.5.6-r2

TOPDIR = .
include Makefile.inc

# pass generated variables to save time
MAKECMD = $(MAKE) OS="$(OS)" WARNINGS="$(WARNINGS)"

LIBS +=		$(OS_LIB_DYNLOAD)

ifeq ($(OS), win32)
LDFLAGS +=	-Wl,--out-implib=libolsrd.a
LDFLAGS +=	-Wl,--export-all-symbols
endif

SWITCHDIR =	src/olsr_switch
CFGDIR =	src/cfgparser
include $(CFGDIR)/local.mk
TAG_SRCS =	$(SRCS) $(HDRS) $(wildcard $(CFGDIR)/*.[ch] $(SWITCHDIR)/*.[ch])

.PHONY: default_target switch
default_target: $(EXENAME)

$(EXENAME):	$(OBJS) src/builddata.o
		$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

cfgparser:	$(CFGDEPS) src/builddata.o
		$(MAKE) -C $(CFGDIR)

switch:		
	$(MAKECMD) -C $(SWITCHDIR)

# generate it always
.PHONY: src/builddata.c
src/builddata.c:
	@$(RM) "$@"
	@echo "#include \"defs.h\"" >> "$@" 
	@echo "const char olsrd_version[] = \"olsr.org - $(VERS)\";" >> "$@" 
	@date +"const char build_date[] = \"%Y-%m-%d %H:%M:%S\";" >> "$@" 
	@echo "const char build_host[] = \"$(shell hostname)\";" >> "$@" 


.PHONY: help libs clean_libs libs_clean clean uberclean install_libs libs_install install_bin install_olsrd install build_all install_all clean_all 

clean:
	-rm -f $(OBJS) $(SRCS:%.c=%.d) $(EXENAME) $(EXENAME).exe src/builddata.c $(TMPFILES)
ifeq ($(OS), win32)
	-rm -f libolsrd.a
	-rm -f gui/win32/Main/olsrd_cfgparser.lib
	-rm -f olsr-setup.exe
	-rm -fr gui/win32/Main/Release
	-rm -fr gui/win32/Shim/Release
endif

uberclean:	clean clean_libs
	-rm -f $(TAGFILE)
#	BSD-xargs has no "--no-run-if-empty" aka "-r"
	find . \( -name '*.[od]' -o -name '*~' \) -not -path "*/.hg*" -print0 | xargs -0 rm -f
	$(MAKECMD) -C $(SWITCHDIR) clean
	$(MAKECMD) -C $(CFGDIR) clean

install: install_olsrd

install_bin:
		mkdir -p $(SBINDIR)
		install -m 755 $(EXENAME) $(SBINDIR)
		$(STRIP) $(SBINDIR)/$(EXENAME)

install_olsrd:	install_bin
		@echo ========= C O N F I G U R A T I O N - F I L E ============
		@echo $(EXENAME) uses the configfile $(CFGFILE)
		@echo a default configfile. A sample RFC-compliance aimed
		@echo configfile can be found in olsrd.conf.default.rfc.
		@echo However none of the larger OLSRD using networks use that
		@echo so install a configfile with activated link quality exstensions
		@echo per default.
		@echo can be found at files/olsrd.conf.default.lq
		@echo ==========================================================
		mkdir -p $(ETCDIR)
		-cp -i files/olsrd.conf.default.lq $(CFGFILE)
		@echo -------------------------------------------
		@echo Edit $(CFGFILE) before running olsrd!!
		@echo -------------------------------------------
		@echo Installing manpages $(EXENAME)\(8\) and $(CFGNAME)\(5\)
		mkdir -p $(MANDIR)/man8/
		cp files/olsrd.8.gz $(MANDIR)/man8/$(EXENAME).8.gz
		mkdir -p $(MANDIR)/man5/
		cp files/olsrd.conf.5.gz $(MANDIR)/man5/$(CFGNAME).5.gz

tags:
		$(TAGCMD) -o $(TAGFILE) $(TAG_SRCS)

rpm:
		@$(RM) olsrd-current.tar.bz2
		@echo "Creating olsrd-current.tar.bz2 ..."
		@./list-excludes.sh | tar  --exclude-from=- --exclude="olsrd-current.tar.bz2" -C .. -cjf olsrd-current.tar.bz2 olsrd-current
		@echo "Building RPMs..."
		@rpmbuild -ta olsrd-current.tar.bz2
#
# PLUGINS
#

# This is quite ugly but at least it works
ifeq ($(OS),linux)
SUBDIRS = $(notdir $(shell find lib -maxdepth 2 -name Makefile -not -path lib/Makefile -printf "%h\n"|sort))
else
ifeq ($(OS),win32)
SUBDIRS := dot_draw httpinfo mini pgraph secure txtinfo
else
SUBDIRS := bmf dot_draw dyn_gw dyn_gw_plain httpinfo mini nameservice pgraph secure txtinfo
endif
endif

libs:
		set -e;for dir in $(SUBDIRS);do $(MAKECMD) -C lib/$$dir LIBDIR=$(LIBDIR);done

libs_clean clean_libs:
		-for dir in $(SUBDIRS);do $(MAKECMD) -C lib/$$dir LIBDIR=$(LIBDIR) clean;done
ifeq ($(OS), win32)
		-rm -f lib/pgraph/olsrd_pgraph.dll
		-rm -f lib/txtinfo/olsrd_txtinfo.dll
		-rm -f lib/httpinfo/olsrd_httpinfo.dll
		-rm -f lib/secure/olsrd_secure.dll
		-rm -f lib/dot_draw/olsrd_dot_draw.dll
		-rm -f lib/mini/olsrd_mini.dll
endif

libs_install install_libs:
		set -e;for dir in $(SUBDIRS);do $(MAKECMD) -C lib/$$dir LIBDIR=$(LIBDIR) install;done

httpinfo:
		$(MAKECMD) -C lib/httpinfo clean
		$(MAKECMD) -C lib/httpinfo 
		$(MAKECMD) -C lib/httpinfo DESTDIR=$(DESTDIR) install 

tas:
		$(MAKECMD) -C lib/tas clean
		$(MAKECMD) -C lib/tas
		$(MAKECMD) -C lib/tas DESTDIR=$(DESTDIR) install

dot_draw:
		$(MAKECMD) -C lib/dot_draw clean
		$(MAKECMD) -C lib/dot_draw
		$(MAKECMD) -C lib/dot_draw DESTDIR=$(DESTDIR) install

nameservice:
		$(MAKECMD) -C lib/nameservice clean
		$(MAKECMD) -C lib/nameservice
		$(MAKECMD) -C lib/nameservice DESTDIR=$(DESTDIR) install

dyn_gw:
		$(MAKECMD) -C lib/dyn_gw clean
		$(MAKECMD) -C lib/dyn_gw
		$(MAKECMD) -C lib/dyn_gw DESTDIR=$(DESTDIR) install

dyn_gw_plain:
		$(MAKECMD) -C lib/dyn_gw_plain clean
		$(MAKECMD) -C lib/dyn_gw_plain
		$(MAKECMD) -C lib/dyn_gw_plain DESTDIR=$(DESTDIR) install

secure:
		$(MAKECMD) -C lib/secure clean
		$(MAKECMD) -C lib/secure
		$(MAKECMD) -C lib/secure DESTDIR=$(DESTDIR) install

pgraph:
		$(MAKECMD) -C lib/pgraph clean
		$(MAKECMD) -C lib/pgraph 
		$(MAKECMD) -C lib/pgraph DESTDIR=$(DESTDIR) install 

bmf:
		$(MAKECMD) -C lib/bmf clean
		$(MAKECMD) -C lib/bmf 
		$(MAKECMD) -C lib/bmf DESTDIR=$(DESTDIR) install 

quagga:
		$(MAKECMD) -C lib/quagga clean
		$(MAKECMD) -C lib/quagga 
		$(MAKECMD) -C lib/quagga DESTDIR=$(DESTDIR) install 

txtinfo:
		$(MAKECMD) -C lib/txtinfo clean
		$(MAKECMD) -C lib/txtinfo 
		$(MAKECMD) -C lib/txtinfo DESTDIR=$(DESTDIR) install 

build_all:	all switch libs
install_all:	install install_libs
clean_all:	uberclean clean_libs
