# Makefile for concord

SRC=../src
INSTALLDIR=/usr/local/share/concord
SYMLINKDIR=/usr/local/bin

FILES=\
    ../src/concord/concord.c \
    ../src/concord/versioninfo.c \
    ../src/snaplib/coordsys/crdsys_src_default.c \
    ../src/snaplib/coordsys/crdsys_src_lists.c \
    ../src/snaplib/coordsys/emprojr.c \
    ../src/snaplib/coordsys/gnproj.c \
    ../src/snaplib/coordsys/gnprojr.c \
    ../src/snaplib/coordsys/isometrc.c \
    ../src/snaplib/coordsys/lambert.c \
    ../src/snaplib/coordsys/lambertr.c \
    ../src/snaplib/coordsys/nzmg.c \
    ../src/snaplib/coordsys/nzmgr.c \
    ../src/snaplib/coordsys/paramdef.c \
    ../src/snaplib/coordsys/psproj.c \
    ../src/snaplib/coordsys/psprojr.c \
    ../src/snaplib/coordsys/tmproj.c \
    ../src/snaplib/coordsys/tmprojr.c \
    ../src/snaplib/coordsys/crdsys_conv.c \
    ../src/snaplib/coordsys/crdsys_csys.c \
    ../src/snaplib/coordsys/crdsys_csys_describe.c \
    ../src/snaplib/coordsys/crdsys_csys_parse.c \
    ../src/snaplib/coordsys/crdsys_ell.c \
    ../src/snaplib/coordsys/crdsys_ell_func.c \
    ../src/snaplib/coordsys/crdsys_ell_parse.c \
    ../src/snaplib/coordsys/crdsys_hrs.c \
    ../src/snaplib/coordsys/crdsys_hrs_func.c \
    ../src/snaplib/coordsys/crdsys_hrs_parse.c \
    ../src/snaplib/coordsys/crdsys_prj.c \
    ../src/snaplib/coordsys/crdsys_prj_func.c \
    ../src/snaplib/coordsys/crdsys_prj_install.c \
    ../src/snaplib/coordsys/crdsys_prj_parse.c \
    ../src/snaplib/coordsys/crdsys_prj_register.c \
    ../src/snaplib/coordsys/crdsys_rf.c \
    ../src/snaplib/coordsys/crdsys_rfdef.c \
    ../src/snaplib/coordsys/crdsys_rfdef_bw.c \
    ../src/snaplib/coordsys/crdsys_rfdef_grid.c \
    ../src/snaplib/coordsys/crdsys_rfdef_linzdef.c \
    ../src/snaplib/coordsys/crdsys_rffunc.c \
    ../src/snaplib/coordsys/crdsys_rffunc_grid.c \
    ../src/snaplib/coordsys/crdsys_rf_func.c \
    ../src/snaplib/coordsys/crdsys_rf_parse.c \
    ../src/snaplib/coordsys/crdsys_src.c \
    ../src/snaplib/coordsys/crdsys_src_csdef.c \
    ../src/snaplib/coordsys/emproj.c \
    ../src/snaplib/dbl4/dbl4_utl_binsrc.c \
    ../src/snaplib/dbl4/dbl4_utl_blob.c \
    ../src/snaplib/dbl4/dbl4_utl_date.c \
    ../src/snaplib/dbl4/dbl4_utl_grid.c \
    ../src/snaplib/dbl4/dbl4_utl_lnzdef.c \
    ../src/snaplib/dbl4/dbl4_utl_trig.c \
    ../src/snaplib/dbl4/snap_dbl4_interface.c \
    ../src/snaplib/geoid/geoid.c \
    ../src/snaplib/geoid/griddata.c \
    ../src/snaplib/util/fileutil.c \
    ../src/snaplib/util/binfile.c \
    ../src/snaplib/util/chkalloc.c \
    ../src/snaplib/util/dateutil.c \
    ../src/snaplib/util/datafile.c \
    ../src/snaplib/util/dms.c \
    ../src/snaplib/util/dstring.c \
    ../src/snaplib/util/getversion.c \
    ../src/snaplib/util/get_date.c \
    ../src/snaplib/util/errdef.c \
    ../src/snaplib/util/filelist.c \
    ../src/snaplib/util/geodetic.c \
    ../src/snaplib/util/iostring.c \
    ../src/snaplib/util/license.c \
    ../src/snaplib/util/linklist.c \
    ../src/snaplib/util/strarray.c 

OBJECTS = $(FILES:.c=.o)

CC = g++
CFLAGS = -g -std=c++0x -fpermissive -I$(SRC)/snaplib -I$(SRC)/snaplib/dbl4

all: bin/concord reference_files

bin/concord: $(OBJECTS)
	rm -rf bin
	mkdir bin
	$(CC) $(OBJECTS) -lm -o bin/concord

reference_files:
	rm -rf bin/config
	mkdir -p bin/config/coordsys
	cp $(SRC)/coordsys/* bin/config/coordsys
	
test: bin/concord
	cd ../test && sh test.sh

clean:
	rm -rf bin
	rm -rf $(OBJECTS)
	rm -rf ../test/out

uninstall:
	rm -rf $(INSTALLDIR)
	rm -f $(SYMLINKDIR)/concord


install: bin/concord reference_files uninstall
	mkdir $(INSTALLDIR)
	cp -r bin/* $(INSTALLDIR)
	ln -s $(INSTALLDIR)/concord $(SYMLINKDIR)/concord
