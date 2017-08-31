prefix = /usr
libdir = $(prefix)/lib
incdir = $(prefix)/include
pkgconfdir = $(libdir)/pkgconfig

PKGDEPS = dbus-1
CFLAGS := `pkg-config --cflags $(PKGDEPS)` -fPIC -Wall -O2 -I./include $(CFLAGS)
LDFLAGS := `pkg-config --libs-only-L $(PKGDEPS)` $(LDFLAGS)
LDLIBS := `pkg-config --libs-only-l --libs-only-other $(PKGDEPS)` $(LDLIBS)

LIBS=libplayback-1.la

%.lo: src/%.c
	libtool --tag=CC --mode=compile $(CC) $(CFLAGS) $(CPPFLAGS) -c $<

libplayback-1.la: bluetooth.lo mute.lo playback.lo playback-types.lo privacy.lo
	libtool --mode=link --tag=CC $(CC) $(LDFLAGS) -rpath $(libdir) -version-number 0:0:5 -o $@ $^ $(LDLIBS)

install/%.la: %.la
	install -d $(DESTDIR)$(libdir)
	libtool --mode=install install -c $(notdir $@) $(DESTDIR)$(libdir)/$(notdir $@)
install: $(addprefix install/,$(LIBS))
	libtool --mode=finish $(libdir)
	install -d $(DESTDIR)$(incdir)/libplayback-1/libplayback
	install -d $(DESTDIR)$(pkgconfdir)
	install include/libplayback/playback.h $(DESTDIR)$(incdir)/libplayback-1/libplayback
	install include/libplayback/playback-macros.h $(DESTDIR)$(incdir)/libplayback-1/libplayback
	install include/libplayback/playback-types.h $(DESTDIR)$(incdir)/libplayback-1/libplayback
	install libplayback-1.pc $(DESTDIR)$(pkgconfdir)

clean:
	rm -rf *.o *.lo *.la .libs
