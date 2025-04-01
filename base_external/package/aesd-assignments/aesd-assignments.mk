##############################################################
#
# AESD-ASSIGNMENTS
#
##############################################################

#Fill up the contents below in order to reference your assignment 3 git contents
AESD_ASSIGNMENTS_VERSION = '72ee4319f3d34b921ffb231e0d8fb1ed070ae2b7'
# Note: Be sure to reference the *ssh* repository URL here (not https) to work properly
# with ssh keys and the automated build/test system.
# Your site should start with git@github.com:
AESD_ASSIGNMENTS_SITE = 'git@github.com:cu-ecen-aeld/assignment-5-johniyoods.git'
AESD_ASSIGNMENTS_SITE_METHOD = git
AESD_ASSIGNMENTS_GIT_SUBMODULES = YES

define AESD_ASSIGNMENTS_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(@D)/server/aesdsocket $(@D)/server/aesdsocket.c
endef

define AESD_ASSIGNMENTS_CLEAN_CMDS
	$(RM) -rf $(@D)/server/aesdsocket
endef

define AESD_ASSIGNMENTS_DIRCLEAN_CMDS
	$(RM) -rf $(@D)
endef

# Add dirclean target
AESD_ASSIGNMENTS_TARGETS += dirclean

define AESD_ASSIGNMENTS_INSTALL_TARGET_CMDS
        $(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment5/* $(TARGET_DIR)/bin
        @echo "Installing aesdsocket to $(TARGET_DIR)/usr/bin"
        $(INSTALL) -D -m 0755 $(@D)/server/aesdsocket $(TARGET_DIR)/usr/bin/aesdsocket
        @echo "Installing aesdsocket-start-stop to $(TARGET_DIR)/etc/init.d/S99aesdsocket"
        $(INSTALL) -D -m 0755 $(@D)/server/aesdsocket-start-stop $(TARGET_DIR)/etc/init.d/S99aesdsocket
        chmod +x $(TARGET_DIR)/etc/init.d/S99aesdsocket
endef

$(eval $(call generic-package,AESD_ASSIGNMENTS))
