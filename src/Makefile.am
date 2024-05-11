include $(top_srcdir)/Make_global.am

lib_LTLIBRARIES = libuallsyms.la

libuallsyms_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libuallsyms.map -version-info $(LIBVERSION)
libuallsyms_la_SOURCES =    uallsyms.c      \
                            uallsyms.h      \
                            uname.c         \
                            uname.h         \
                            arch/x86_64.c   \
                            arch/x86_64.h