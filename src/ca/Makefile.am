SUBDIRS = \
	libs \
	gui \
	events \
	utils \
	scripts \
	cg
# FIXME	[3.0] diffgui \
#    $(CA_RELEASE_TYPE)

install-exec-hook:
	@if test -e "scripts/Setup.sh"; then \
		scripts/Setup.sh $(DESTDIR)$(prefix); \
	fi

uninstall-hook:
	@if test -e "scripts/Uninstall.sh"; then \
		scripts/Uninstall.sh $(DESTDIR)$(prefix);\
	fi
