MODULE := backends/platform/amigaos3

# change amigaos3-graphics-cgx.o to amigaos3-graphics-aga.o

MODULE_OBJS = \
	amigaos3-main.o \
	amigaos3-aga.o \
	amigaos3-graphics-aga.o

#amigaos3-graphics-cgx.o

# We don't use rules.mk but rather manually update OBJS and MODULE_DIRS.
MODULE_OBJS := $(addprefix $(MODULE)/, $(MODULE_OBJS))
OBJS := $(MODULE_OBJS) $(OBJS)
MODULE_DIRS += $(sort $(dir $(MODULE_OBJS)))

# A rule to ouput .asm files for source files, allows us to inspect
# what the compiler produced
%.asm: %.c
	$(QUIET)$(MKDIR) $(*D)/$(DEPDIR)
	$(QUIET_CC)$(CC) $(CXX_UPDATE_DEP_FLAG) $(CFLAGS) $(CPPFLAGS) -Wa,-adhln -g -c $(<) > $*.asm
%.asm: %.cpp
	$(QUIET)$(MKDIR) $(*D)/$(DEPDIR)
	$(QUIET_CXX)$(CXX) $(CXX_UPDATE_DEP_FLAG) $(CXXFLAGS) $(CPPFLAGS) -Wa,-adhln -g -c $(<) > $*.asm



# Special target to create an AmigaOS snapshot installation
amigaos3dist: $(EXECUTABLE)
	mkdir -p $(AMIGAOSPATH)
	mkdir -p $(AMIGAOSPATH)/themes
	mkdir -p $(AMIGAOSPATH)/extras
	$(STRIP) $(EXECUTABLE) -g --strip-unneeded -o $(AMIGAOSPATH)/$(EXECUTABLE)
	cp ${srcdir}/backends/platform/amigaos3/overlay* $(AMIGAOSPATH)/
	cp ${srcdir}/icons/scummvm_drawer.info $(AMIGAOSPATH).info
	cp ${srcdir}/icons/scummvm.info $(AMIGAOSPATH)/$(EXECUTABLE).info
	cp $(DIST_FILES_THEMES) $(AMIGAOSPATH)/themes/
ifdef DIST_FILES_ENGINEDATA
	cp $(DIST_FILES_ENGINEDATA) $(AMIGAOSPATH)/extras/
endif
	cat ${srcdir}/README | sed -f ${srcdir}/dists/amiga/convertRM.sed > README.conv
# AmigaOS's shell is not happy with indented comments, thus don't do it.
# AREXX seems to have problems when ${srcdir} is '.'. It will break with a
# "Program not found" error. Therefore we copy the script to the cwd and
# remove it again, once it has finished.
#	cp ${srcdir}/dists/amiga/RM2AG.rx .
#	rx RM2AG.rx README.conv
#	cp README.guide $(AMIGAOSPATH)
#	rm RM2AG.rx
#	rm README.conv
#	rm README.guide
#	cp $(DIST_FILES_DOCS) $(AMIGAOSPATH)

