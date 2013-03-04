if caperf_support
SUBDIRS = \
	$(LIBDWARF_DIR) \
	$(LIBCADATA_DIR) \
	$(CAPERF_DIR) \
	ca_agent \
	$(CA_OPROFILE_DIR) \
	ca 
else
SUBDIRS = \
	$(LIBDWARF_DIR) \
	$(LIBCADATA_DIR) \
	ca_agent \
	$(CA_OPROFILE_DIR) \
	ca 
endif

install-exec-hook:
	@if test -e "${LIBDWARF_DIR}libdwarf.so"; then \
		install -pD -m 755 ${LIBDWARF_DIR}libdwarf.so $(DESTDIR)$(prefix)/lib/libdwarf.so ; \
		/sbin/ldconfig ;\
	fi

uninstall-hook:
	@if test -e "${LIBDWARF_DIR}libdwarf.so"; then \
		if test -e "$(DESTDIR)$(prefix)/lib/libdwarf.so"; then \
			rm -f $(DESTDIR)$(prefix)/lib/libdwarf.so ; \
			/sbin/ldconfig ; \
		fi ; \
	fi

