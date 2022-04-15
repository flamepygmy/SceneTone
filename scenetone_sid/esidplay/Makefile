#
# Makefile for ESidPlay
#
# (c) 2001 Alfred E. Heggestad
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#

default:	er5


$(EPOCROOT)/esidplay/group/libsidplay_er5.marm: $(EPOCROOT)/esidplay/group/libsidplay_er5.mmp
	cd group; makmake libsidplay_er5 marm; cd ..
$(EPOCROOT)/epoc32/release/marm/rel/libsidplay.dll: $(EPOCROOT)/esidplay/group/libsidplay_er5.marm
	cd group; make -f libsidplay_er5.marm; cd ..

$(EPOCROOT)/esidplay/group/esidplay_er5.marm: $(EPOCROOT)/esidplay/group/esidplay_er5.mmp
	cd group; makmake esidplay_er5 marm; cd ..
$(EPOCROOT)/epoc32/release/marm/rel/esidplay.exe: $(EPOCROOT)/esidplay/group/esidplay_er5.marm
	cd group; make -f esidplay_er5.marm; cd ..

$(EPOCROOT)/esidplay/group/sidplay_er5.marm: $(EPOCROOT)/esidplay/group/sidplay_er5.mmp
	cd group; makmake sidplay_er5 marm; cd ..
$(EPOCROOT)/epoc32/release/marm/rel/sidplay.app: $(EPOCROOT)/esidplay/group/sidplay_er5.marm
	cd group; make -f sidplay_er5.marm; cd ..

$(EPOCROOT)/esidplay/recogs/recsid_er5.marm: $(EPOCROOT)/esidplay/recogs/recsid_er5.mmp
	cd recogs; makmake recsid_er5 marm; cd ..
$(EPOCROOT)/epoc32/release/marm/rel/recsid.mdl: $(EPOCROOT)/esidplay/recogs/recsid_er5.marm
	cd recogs; make -f recsid_er5.marm; cd ..

$(EPOCROOT)/esidplay/recogs/recsidfile_er5.marm: $(EPOCROOT)/esidplay/recogs/recsidfile_er5.mmp
	cd recogs; makmake recsidfile_er5 marm; cd ..
$(EPOCROOT)/epoc32/release/marm/rel/recsidfile.rdl: $(EPOCROOT)/esidplay/recogs/recsidfile_er5.marm
	cd recogs; make -f recsidfile_er5.marm; cd ..

$(EPOCROOT)/esidplay/group/sidplay_er5.SIS: group/sidplay_er5.pkg
	cd group; makesis sidplay_er5.pkg

er5:	$(EPOCROOT)/epoc32/release/marm/rel/libsidplay.dll \
	$(EPOCROOT)/epoc32/release/marm/rel/esidplay.exe \
	$(EPOCROOT)/epoc32/release/marm/rel/sidplay.app \
	$(EPOCROOT)/epoc32/release/marm/rel/recsid.mdl \
	$(EPOCROOT)/epoc32/release/marm/rel/recsidfile.rdl 
	@echo "all's well"

sis:	$(EPOCROOT)/esidplay/group/sidplay_er5.SIS

#
# clean up directories - this part taken from /usr/src/linux/Makefile
#
clean:
	if [ -f group/esidplay_er5.marm ]; then \
	  cd group; make -f esidplay_er5 clean; cd ..; \
	fi
	rm -f $(EPOCROOT)/epoc32/release/marm/rel/libsidplay.dll
	rm -f $(EPOCROOT)/epoc32/release/marm/rel/esidplay.exe
	rm -f $(EPOCROOT)/epoc32/release/marm/rel/sidplay.app
	rm -f core `find . \( \
		-iname '*.sis' -o \
		-iname '*.rpp' -o \
		-iname '*.rsc' -o \
		-iname '*.rsg' -o \
		-iname '*.orig' -o \
		-iname '*.marm' -o \
		-iname '*~' -o \
		-iname '*.~*' -o \
		-iname '*.bak' -o \
		-iname '#*#' -o \
		-iname '*.mbm' -o \
		-iname '*.aif' -o \
		-iname '*.wins' -o \
		-iname '*.armi' -o \
		-iname '*.arm4' -o \
		-iname '*.thumb' -o \
		-iname '*.uid.cpp' -o \
		-iname 'abld.bat' -o -size 0 \) \
		-print` TAGS
