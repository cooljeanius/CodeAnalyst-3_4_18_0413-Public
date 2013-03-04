SUBDIRS = \
	src

BUILD = 0
PKG_DIR=$(PACKAGE_NAME)-$(PACKAGE_VERSION)-$(BUILD)

package:
	rm -rf $(PKG_DIR)
	mkdir -p $(PKG_DIR)
	cp -Pr $(prefix)/* $(PKG_DIR)/
	tar cvzf $(PKG_DIR).tgz $(PKG_DIR) 
	rm -rf $(PKG_DIR)	
