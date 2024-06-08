include $(top_srcdir)/Make_global.am

lib_LTLIBRARIES = libuallsyms.la

libuallsyms_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libuallsyms.map -version-info $(LIBVERSION)
libuallsyms_la_SOURCES =    uallsyms.c          \
                            uallsyms.h          \
                            arch/x86_64/kbase.c \
                            arch/x86_64/kbase.h \
                            kallsyms/core.c     \
                            kallsyms/core.h     \
                            kinfo/kconfig.c     \
                            kinfo/kconfig.h     \
                            kinfo/uname.c       \
                            kinfo/uname.h       \
                            utils/log.c         \
                            utils/log.h         