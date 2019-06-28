AM_CPPFLAGS =								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce4-systemload-gpu-plugin\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

plugindir =								\
	$(libdir)/xfce4/panel/plugins
	
plugin_LTLIBRARIES =							\
	libsystemloadgpu.la

libsystemloadgpu_la_SOURCES =					\
	systemload.c							\
	uptime.c							\
	uptime.h							\
	memswap.c							\
	memswap.h							\
	gpu.c							\
	gpu.h							\
	cpu.c								\
	cpu.h

libsystemloadgpu_la_CFLAGS =					\
	$(LIBXFCE4UI_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(UPOWER_GLIB_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libsystemloadgpu_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libsystemloadgpu_la_LIBADD =						\
	$(LIBXFCE4UI_LIBS)						\
	$(LIBXFCE4PANEL_LIBS) 						\
	$(UPOWER_GLIB_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel/plugins

desktop_DATA =								\
	systemloadgpu.desktop

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	systemloadgpu.desktop.in

CLEANFILES =								\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
