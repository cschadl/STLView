CXX=g++
MATHSTUFF=$(HOME)/workspace/mathstuff
STLIMPORT=$(HOME)/workspace/stl-import
INCLUDE=-I $(MATHSTUFF) -I $(STLIMPORT)
PKGFLAGS=`pkg-config --cflags gtkmm-2.4 gtkglextmm-1.2`
PKGLIBS=`pkg-config --libs gtkmm-2.4 gtkglextmm-1.2`
CFLAGS=-Wall -O3
OUTDIR=Release
EXECUTABLE=stlview

GLOBS=STLDrawArea.o MainWindow.o GLCamera.o stlview.o
STLIMPORTOBJS=stl_import.o triangle_mesh.o

# Release configuration
OBJS=$(GLOBS) $(STLIMPORTOBJS)
OUTOBJS=$(addprefix $(OUTDIR)/, $(OBJS))
OUTEXE=$(OUTDIR)/$(EXECUTABLE)

# Debug configuration
OUTDIR_DEBUG=Debug
OUTOBJS_DEBUG=$(addprefix $(OUTDIR_DEBUG)/, $(OBJS))
OUTEXE_DEBUG=$(OUTDIR_DEBUG)/$(EXECUTABLE)
CFLAGS_DEBUG=-Wall -ggdb3
CPPFLAGS_DEBUG=-DDEBUG

################################
## Release config

all: $(OUTEXE)

$(OUTEXE): $(OUTOBJS)
	$(CXX) $(OUTOBJS) $(PKGLIBS) -o $(OUTEXE)

# This generates dependency rules
-include $(OUTOBJS:.o=.d)

# Rule for STLIMPORT stuff
$(addprefix $(OUTDIR)/, $(STLIMPORTOBJS)): $(OUTDIR)/%.o : $(STLIMPORT)/%.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDE) -o $@ $<	
	$(CXX) -MM $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR)\/$$&/' > $(OUTDIR)/$*.d

## Default rule
$(OUTDIR)/%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(PKGFLAGS) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(PKGFLAGS) $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR)\/$$&/' > $(OUTDIR)/$*.d
	
clean:
	rm -rf $(OUTDIR)/*.o $(OUTEXE) $(OUTDIR)/*.d
	
#################################
## Debug config
## I don't know why the fuck I can't
## just set OUTDIR=Debug and do $(OUTEXE)
## but I'm sick of screwing with it and
## I need to get on with my life.

debug: $(OUTEXE_DEBUG)

$(OUTEXE_DEBUG): $(OUTOBJS_DEBUG)
	$(CXX) $(OUTOBJS_DEBUG) $(PKGLIBS) -o $(OUTEXE_DEBUG)
	
# This generates dependency rules
-include $(OUTOBJS_DEBUG:.o=.d)
	
# Rule for STLIMPORT stuff
$(addprefix $(OUTDIR_DEBUG)/, $(STLIMPORTOBJS)): $(OUTDIR_DEBUG)/%.o : $(STLIMPORT)/%.cpp
	$(CXX) -c $(CFLAGS_DEBUG) $(CPPFLAGS_DEBUG) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR_DEBUG)\/$$&/' > $(OUTDIR_DEBUG)/$*.d

## Default rule
$(OUTDIR_DEBUG)/%.o: %.cpp
	$(CXX) -c $(CFLAGS_DEBUG) $(CPPFLAGS_DEBUG) $(PKGFLAGS) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(PKGFLAGS) $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR_DEBUG)\/$$&/' > $(OUTDIR_DEBUG)/$*.d

# This works for some reason
debug_clean: OUTDIR=Debug
debug_clean: clean