AM_CPPFLAGS =								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce4-systemload-dummy-plugin\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

plugindir =								\
	$(libdir)/xfce4/panel/plugins
	
plugin_LTLIBRARIES =							\
	libsystemloaddummy.la

libsystemloaddummy_la_SOURCES =					\
	systemload.c							\
	uptime.c							\
	uptime.h							\
	memswap.c							\
	memswap.h							\
	cpu.c								\
	cpu.h

libsystemloaddummy_la_CFLAGS =					\
	$(LIBXFCE4UI_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(UPOWER_GLIB_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libsystemloaddummy_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libsystemloaddummy_la_LIBADD =						\
	$(LIBXFCE4UI_LIBS)						\
	$(LIBXFCE4PANEL_LIBS) 						\
	$(UPOWER_GLIB_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel/plugins

desktop_DATA =								\
	systemloaddummy.desktop

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	systemloaddummy.desktop.in

CLEANFILES =								\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
