#
# Makefile for sidplay.aif (ER6)
#

APP=sidplay

AIF = $(APP).aif
MBM = $(APP).mbm
DEST3 = $(EPOCROOT)/epoc32/release/armi/udeb/
DEST4 = $(EPOCROOT)/epoc32/release/armi/urel/

#
# NOTE: only change the original 48x48 bitmap, the rest
# are converted with the 'convert' program
#
BITMAPS_ORIG=$(APP)_48i.bmp $(APP)_48m.bmp
BITMAPS_GEN=$(APP)_32i.bmp $(APP)_32m.bmp \
	$(APP)_24i.bmp $(APP)_24m.bmp \
	$(APP)_16i.bmp $(APP)_16m.bmp
BITMAPS=$(BITMAPS_ORIG) $(BITMAPS_GEN)


default:
	@echo "Use this to build MBM and AIF for $(APP)"
	@echo ""
	@echo "    make $(MBM)	build the .mbm file"
	@echo "    make $(AIF)	invoke AIFTOOL to build the AIFs"
	@echo "    make install	save the AIFs with their EIKON projects"
	@echo "    make all		do all these steps in sequence"

all:	$(MBM) $(AIF) install

$(BITMAPS_GEN) : $(BITMAPS_ORIG)
	convert -scale 32x32 $(APP)_48i.bmp $(APP)_32i.bmp
	convert -scale 24x24 $(APP)_48i.bmp $(APP)_24i.bmp
	convert -scale 16x16 $(APP)_48i.bmp $(APP)_16i.bmp
	convert -scale 32x32 $(APP)_48m.bmp $(APP)_32m.bmp
	convert -scale 24x24 $(APP)_48m.bmp $(APP)_24m.bmp
	convert -scale 16x16 $(APP)_48m.bmp $(APP)_16m.bmp

$(MBM) mbm : $(BITMAPS)
	bmconv $(MBM) \
	/c12$(APP)_48i.bmp /c12$(APP)_48m.bmp \
	/c12$(APP)_32i.bmp /c12$(APP)_32m.bmp \
	/c12$(APP)_24i.bmp /c12$(APP)_24m.bmp \
	/c12$(APP)_16i.bmp /c12$(APP)_16m.bmp

$(AIF) aif : $(MBM) $(APP).rss
	aiftool $(APP) $(APP).mbm

install: $(AIF) $(MBM)
	cp -f $(AIF) $(DEST3)
	cp -f $(AIF) $(DEST4)

clean:
	rm -f *~ *.rsc *.aif *.mbm $(AIF) $(MBM) $(DEST1)$(AIF) $(DEST2)$(AIF) $(DEST3)$(AIF) $(DEST4)$(AIF) $(BITMAPS_GEN)
