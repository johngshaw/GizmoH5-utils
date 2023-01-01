#
# ... Makefile for convertGizmoH5
#
.SILENT:

XCUDA_TOP= ${XCUDA_HOME}
include ${XCUDA_HOME}/config/Makefile.${XCUDA_ARCH}

# -----------------------------------------------------------------------------------
#
# Main targets
#
.PHONY: help
help:
	@echo "" 
	@echo "------------------ Targets --------------------"
	@echo ""
	@echo "  all"
	@echo "  {"
	@echo "    convertGizmoH5"
	@echo "    testConvertGizmoH5"
	@echo "    test_H5pio"
	@echo "    disk_2d"
	@echo "  } "
	@echo ""
	@echo "  clearAll"
	@echo "  {"
	@echo "    clean     deletes convertGizmoH5.o"
	@echo "    clear     deletes convertGizmoH5"
	@echo "    clearData deletes data/*"
	@echo "  } "
	@echo ""
	@echo "-----------------------------------------------"
	@echo ""

# -----------------------------------------------------------------------------------
#
# Build targets
#
.PHONY: all
all: 
	$(MAKE) convertGizmoH5

convertGizmoH5: convertGizmoH5.cpp
	@echo " Compiling ... convertGizmoH5"
	g++ -I$(XCUDA_INC) convertGizmoH5.cpp -o convertGizmoH5 $(XCUT_LINK)

testConvertGizmoH5: convertGizmoH5
	@echo " Testing ... convertGizmoH5"
	h5dump -A ./data/noh_ics.hdf5
	convertGizmoH5 -p=2097152 --file=noh_ics.hdf5 > ./data/noh_ics.xdmf
	ls -lh data

H5pio.o: H5pio.h H5pio.cpp
	g++ -I$(XCUDA_INC) -DHAS_XCUT -c H5pio.cpp

test_H5pio: H5pio.o test_H5pio.cpp
	g++ -I$(XCUDA_INC) -DHAS_XCUT test_H5pio.cpp -o test_H5pio H5pio.o $(XCUT_LINK) -lhdf5

disk_2d: H5pio.o disk_2d.cpp
	g++ -I$(XCUDA_INC) -DHAS_XCUT disk_2d.cpp -o disk_2d H5pio.o $(XCUT_LINK) -lhdf5

# -----------------------------------------------------------------------------------
#
# Utility targets
#
.PHONY: clean clear clearData clearAll

clean:
	-$(RM) convertGizmoH5.o
	-$(RM) H5pio.o

clear:
	-$(RM) convertGizmoH5
	-$(RM) test_H5pio
	-$(RM) disk_2d

clearData:
	-$(RM) ./data/*.xdmf

clearAll:
	make clean clear clearData

############################################################################
# DO NOT DELETE
