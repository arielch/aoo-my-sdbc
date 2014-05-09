#**************************************************************
#
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#
#**************************************************************

# On LINUX:
#	make PRJ=$OO_SDK_HOME DEBUG=yes MAKEFLAGS=-j6
# On WINDOWS
#   copy the extension folder inside the SDK C++ examples folder, so that
#	this makefile must is located in Basis\sdk\examples\cpp\aoo-my-sdbc\source
#
PRJ=../../../..
SETTINGS=$(PRJ)/settings

EXTERNAL_DIR=external
PATCHDIR=$(EXTERNAL_DIR)

include $(SETTINGS)/settings.mk
include $(SETTINGS)/std.mk
include $(SETTINGS)/dk.mk

include settings/project.mk
include settings/version.mk
include settings/platform.mk

#this should be in odk/settings/std.mk
OUT_DEPLOY=$(OUT)/deploy

EXTENSION_ID=com.sun.star.mysql-connector-ooo-$(EXTENSION_PLATFORM)
EXTENSION_VERSION=$(MYSQLC_VERSION_MAJOR).$(MYSQLC_VERSION_MINOR).$(MYSQLC_VERSION_MICRO)

MY_SDBC_LOGGER=org.aoo-my-sdbc.MySDBCDriver

# Define non-platform/compiler specific settings
COMP_NAME=aoo-my-sdbc

ifeq "$(DEBUG)" "yes"
COMP_NAME=aoo-my-sdbc-debug
else
ifneq "$(RELEASESTATUS)" ""
COMP_NAME=aoo-my-sdbc-$(RELEASESTATUS)
endif
endif

OUT_COMP_INC=$(OUT_INC)/$(COMP_NAME)
OUT_COMP_GEN=$(OUT_MISC)/$(COMP_NAME)
OUT_COMP_SLO=$(OUT_SLO)/$(COMP_NAME)
OUT_COMP_LIB=$(SHAREDLIB_OUT)/$(COMP_NAME)
OUT_COMP_BIN=$(OUT_BIN)/$(COMP_NAME)
OUT_COMP_DEPLOY=$(OUT_DEPLOY)/$(COMP_NAME)

COMP_IMPL_NAME=$(COMP_NAME).uno.$(SHAREDLIB_EXT)
COMP_PACKAGE_FILENAME=$(COMP_NAME)-$(EXTENSION_VERSION)-$(subst _,-,$(EXTENSION_PLATFORM)).$(UNOOXT_EXT)
COMP_PACKAGE = $(OUT_COMP_BIN)/$(COMP_PACKAGE_FILENAME)
COMP_PACKAGE_URL = $(subst \\,\,"$(COMP_PACKAGE)")
COMP_UNOPKG_MANIFEST    = $(OUT_COMP_GEN)/$(COMPONENT_NAME)/META-INF/manifest.xml
COMP_UNOPKG_DESCRIPTION = $(OUT_COMP_GEN)/$(COMPONENT_NAME)/description.xml
COMP_MAPFILE = $(OUT_COMP_GEN)/$(COMP_NAME).uno.map
COMP_COMPONENTS = $(OUT_COMP_GEN)/$(COMP_NAME).components

COMP_DRIVER_CFG=registry/data/org/openoffice/Office/DataAccess/Drivers.xcu
COMP_DRIVER_LOGGING_CFG=registry/data/org/openoffice/Office/Logging.xcu

DESCRIPTION_IDENTIFIER=$(EXTENSION_ID)
DESCRIPTION_PLATFORM=$(TARGET_PLATFORM)
DESCRIPTION_UPDATE_FILENAME=$(EXTENSION_PLATFORM)-update.xml

# Allow deploying the extension when running it
ENV_USER_INSTALL=-env:"UserInstallation=$(URLPREFIX)$(OUT_COMP_DEPLOY)"
OFFICE_START_OPTIONS=-nologo -nofirststartwizard -norestore -quickstart=no -base
OXT_DEPLOYEDFLAG=$(OUT_COMP_GEN)/so_deployed_$(COMP_NAME).flag

COMP_CXXFILES = \
	mysqlc_optpages.cxx \
	mysqlc_connection.cxx \
	mysqlc_databasemetadata.cxx \
	mysqlc_driver.cxx \
	mysqlc_general.cxx \
	mysqlc_preparedstatement.cxx \
	mysqlc_propertyids.cxx \
	mysqlc_resultset.cxx \
	mysqlc_resultsetmetadata.cxx \
	mysqlc_services.cxx \
	mysqlc_statement.cxx \
	mysqlc_types.cxx


COMP_SLOFILES = $(patsubst %.cxx,$(OUT_COMP_SLO)/%.$(OBJ_EXT),$(COMP_CXXFILES))

COMP_CFLAGS += \
	-DMYSQLC_VERSION_MAJOR=$(MYSQLC_VERSION_MAJOR) \
	-DMYSQLC_VERSION_MINOR=$(MYSQLC_VERSION_MINOR) \
	-DMYSQLC_VERSION_MICRO=$(MYSQLC_VERSION_MICRO)


ifeq "$(DEBUG)" "yes"
	CMAKE_BUILD_TYPE=Debug
	MY_SDBC_LOGLEVEL=-2147483648
else
	CMAKE_BUILD_TYPE=Release
	MY_SDBC_LOGLEVEL=2147483647
endif

# Targets
.PHONY: ALL update-desc MySQLConnector MySQLConnector.run
ALL : \
	MySQLConnector update-desc

include $(SETTINGS)/stdtarget.mk
include external.mk


COMP_CFLAGS +=-DCPPDBC_EXPORTS -DCPPCONN_LIB_BUILD

#-DCPPCONN_LIB=\"libmysqlcppconn.so\"
COMP_CFLAGS += -DCPPCONN_LIB=\"$(MYSQLCONNCPP_LIB)\"
COMP_CFLAGS += -DMYSQL_LIB=\"$(MYSQLCONNC_LIB)\"
COMP_CFLAGS += -DMY_SDBC_LOGGER=\"$(MY_SDBC_LOGGER)\"

COMP_CFLAGS += $(BOOST_INC) $(MYSQLCONNCPP_INC)

ifeq "$(OS)" "LINUX" || "$(OS)" "MACOSX"
# Enable dependency checking
DEPFILES=$(wildcard $(addsuffix .d, $(basename ${COMP_SLOFILES})))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
CC_DEFINES+= -Wall -Wextra -MMD -MP -MF "$(addsuffix .d,$(basename $@))"
endif


$(OUT_COMP_SLO)/%.$(OBJ_EXT) : %.cxx $(SDKTYPEFLAG) $(BOOST_DELIVER_FLAG) $(MYSQLCONNCPP_DELIVER_FLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(CC) $(CC_FLAGS) $(COMP_CFLAGS) $(CC_INCLUDES) -I$(OUT_COMP_INC) $(CC_DEFINES) $(CC_OUTPUT_SWITCH)$(subst /,$(PS),$@) $<


ifeq "$(OS)" "WIN"
$(OUT_COMP_LIB)/%.$(SHAREDLIB_EXT) : $(COMP_SLOFILES)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN))
	$(LINK) \
		$(COMP_LINK_FLAGS) \
		/OUT:$@ \
		/MAP:$(OUT_COMP_GEN)/$(subst $(SHAREDLIB_EXT),map,$(@F)) \
		$(COMP_SLOFILES) \
		$(CPPUHELPERLIB) $(CPPULIB) $(SALLIB) \
		msvcrt.lib msvcprt.lib kernel32.lib
	$(LINK_MANIFEST)
else
$(OUT_COMP_LIB)/%.$(SHAREDLIB_EXT) : $(COMP_SLOFILES)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(LINK) $(COMP_LINK_FLAGS) $(LINK_LIBS) -o $@ $(COMP_SLOFILES) \
		$(CPPUHELPERLIB) $(CPPULIB) $(SALLIB) \
		$(STC++LIB) $(CPPUHELPERDYLIB) $(CPPUDYLIB) $(SALDYLIB)
ifeq "$(OS)" "MACOSX"
	$(INSTALL_NAME_URELIBS)  $@
endif
endif

# rule for Drivers.xcu
$(OUT_COMP_GEN)/$(COMP_DRIVER_CFG) : $(COMP_DRIVER_CFG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(COPY) $< $@

# rule for Logging.xcu
$(OUT_COMP_GEN)/$(COMP_DRIVER_LOGGING_CFG) : $(COMP_DRIVER_LOGGING_CFG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(SDK_CAT) $< | $(SDK_SED) \
		-e "s/#MY_SDBC_LOGGER#/$(MY_SDBC_LOGGER)/" \
		-e "s/#MY_SDBC_LOGLEVEL#/$(MY_SDBC_LOGLEVEL)/" > $@

# rule for extension description.xml
$(COMP_UNOPKG_DESCRIPTION) :  xml/description.xml
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(SDK_CAT) $< | $(SDK_SED) \
	-e "s/#EXTENSION_VERSION#/$(EXTENSION_VERSION)/" \
	-e "s/#DESCRIPTION_IDENTIFIER#/$(DESCRIPTION_IDENTIFIER)/" \
	-e "s/#EXTENSION_PLATFORM#/$(DESCRIPTION_EXTENSION_PLATFORM)/" \
	-e "s/#DESCRIPTION_OOO_MIN_VER#/$(DESCRIPTION_OOO_MIN_VER)/" \
	-e "s/#DESCRIPTION_OOO_MAX_VER#/$(DESCRIPTION_OOO_MAX_VER)/" \
	-e "s/#DESCRIPTION_DEP_NAME_MIN#/$(DESCRIPTION_DEP_NAME_MIN)/" \
	-e "s/#DESCRIPTION_DEP_NAME_MAX#/$(DESCRIPTION_DEP_NAME_MAX)/" \
	-e "s/#DESCRIPTION_PUBLISHER_URL#/$(subst /,\/,$(DESCRIPTION_PUBLISHER_URL))/" \
	-e "s/#DESCRIPTION_PUBLISHER_NAME#/$(DESCRIPTION_PUBLISHER_NAME)/" \
	-e "s/#DESCRIPTION_DISPLAY_NAME#/$(DESCRIPTION_DISPLAY_NAME)/" \
	-e "s/#DESCRIPTION_UPDATE#/$(subst /,\/,$(DESCRIPTION_UPDATE_BASEURL)$(DESCRIPTION_UPDATE_FILENAME))/"  > $@


# rule for update description
$(OUT_COMP_BIN)/$(DESCRIPTION_UPDATE_FILENAME) : xml/update.xml
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(SDK_CAT) $< | $(SDK_SED) \
		-e "s/#DESCRIPTION_IDENTIFIER#/$(DESCRIPTION_IDENTIFIER)/" \
		-e "s/#EXTENSION_VERSION#/$(EXTENSION_VERSION)/" \
		-e "s/#DESCRIPTION_PUBLISHER_URL#/$(subst /,\/,$(DESCRIPTION_PUBLISHER_URL))/" \
		-e "s/#DESCRIPTION_PUBLISHER_NAME#/$(DESCRIPTION_PUBLISHER_NAME)/" \
		-e "s/#UPDATE_WEBSITE#/$(subst /,\/,$(UPDATE_WEBSITE))/" \
		-e "s/#UPDATE_DOWNLOAD#/$(subst /,\/,$(UPDATE_DOWNLOAD_BASE)$(COMP_PACKAGE_FILENAME))/" \
		-e "s/#RELEASE_NOTES#/$(subst /,\/,$(RELEASE_NOTES))/" > $@


# rule for component package manifest
$(COMP_UNOPKG_MANIFEST) : xml/manifest.xml
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(SDK_CAT) $< | $(SDK_SED) \
		-e "s/#COMP_NAME#/$(COMP_NAME)/" \
		-e "s/#UNOPKG_PLATFORM#/$(UNOPKG_PLATFORM)/" > $@

# rule for component pasive registration
$(COMP_COMPONENTS) : xml/components.xml
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(SDK_CAT) $< | $(SDK_SED) \
		-e "s/#UNOPKG_PLATFORM#/$(UNOPKG_PLATFORM)/" \
		-e "s/#COMP_IMPL_NAME#/$(COMP_IMPL_NAME)/" > $@

# rule for component package file
$(COMP_PACKAGE) : $(OUT_COMP_LIB)/$(COMP_IMPL_NAME) $(OUT_COMP_LIB)/$(MYSQLCONNC_LIB) $(OUT_COMP_LIB)/$(MYSQLCONNCPP_LIB)  $(OUT_COMP_GEN)/$(COMP_DRIVER_CFG) $(OUT_COMP_GEN)/$(COMP_DRIVER_LOGGING_CFG)   $(COMP_UNOPKG_MANIFEST) $(COMP_COMPONENTS)  $(COMP_UNOPKG_DESCRIPTION)
	-$(DEL) $(subst \\,\,$(subst /,$(PS),$@))
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN)/$(UNOPKG_PLATFORM))
	$(COPY) $(subst /,$(PS),$<) $(subst /,$(PS),$(OUT_COMP_GEN)/$(UNOPKG_PLATFORM))
	$(COPY) $(subst /,$(PS),$(OUT_COMP_LIB)/$(MYSQLCONNC_LIB)) $(subst /,$(PS),$(OUT_COMP_GEN)/$(UNOPKG_PLATFORM))
	$(COPY) $(subst /,$(PS),$(OUT_COMP_LIB)/$(MYSQLCONNCPP_LIB)) $(subst /,$(PS),$(OUT_COMP_GEN)/$(UNOPKG_PLATFORM))
ifeq "$(OS)" "WIN"
	$(COPY) $(MS_VC90_REDIST_FILES) $(subst /,$(PS),$(OUT_COMP_GEN)/$(UNOPKG_PLATFORM))
endif
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE)
	$(COPY) $(subst /,$(PS),../LICENSE) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE/)
	cd $(subst /,$(PS),$(OUT_COMP_GEN)) && $(SDK_ZIP) -u $(COMP_PACKAGE) $(COMP_NAME).components description.xml
	cd $(subst /,$(PS),$(OUT_COMP_GEN)) && $(SDK_ZIP) -ur $(COMP_PACKAGE) $(UNOPKG_PLATFORM)/ LICENSE/ META-INF/  registry/
	$(SDK_ZIP) -ur $@ images/ description/


update-desc : $(OUT_COMP_BIN)/$(DESCRIPTION_UPDATE_FILENAME)

MySQLConnector : $(COMP_PACKAGE)
# MySQLConnector : $(OUT_COMP_LIB)/$(COMP_IMPL_NAME)
# MySQLConnector : $(COMP_SLOFILES)
# Test MySQL Connector C++
# MySQLConnector : $(OUT_COMP_LIB)/$(MYSQLCONNCPP_LIB)
# MySQLConnector : $(MYSQLCONNCPP_SLOFILES)
# MySQLConnector : $(MYSQLCONNCPP_DELIVER_FLAG)
# MySQLConnector : $(MYSQLCONNCPP_PATCHED_FLAG)
# MySQLConnector : $(MYSQLCONNCPP_UNPACKED_FLAG)
# Test MySQL Connector C
# MySQLConnector : $(OUT_COMP_LIB)/$(MYSQLCONNC_LIB)
# MySQLConnector : $(MYSQLCONNC_DELIVER_FLAG)
# MySQLConnector : $(MYSQLCONNC_BUILD_FLAG)
# MySQLConnector : $(MYSQLCONNC_CONFIGURED_FLAG)
# MySQLConnector : $(MYSQLCONNC_PATCHED_FLAG)
# MySQLConnector : $(MYSQLCONNC_UNPACKED_FLAG)
# Test boost
# MySQLConnector : $(BOOST_DELIVER_FLAG)
# MySQLConnector : $(BOOST_PATCHED_FLAG)
# MySQLConnector : $(BOOST_UNPACKED_FLAG)
	@echo --------------------------------------------------------------------------------
	@echo Please use the following command to deploy the example!
	@echo -
	@echo $(MAKE) MySQLConnector.run
	@echo -
	@echo --------------------------------------------------------------------------------

MySQLConnector.run: $(OXT_DEPLOYEDFLAG)
	"$(OFFICE_PROGRAM_PATH)$(PS)soffice" $(OFFICE_START_OPTIONS)  $(ENV_USER_INSTALL)

$(OXT_DEPLOYEDFLAG) : $(COMP_PACKAGE)
	-$(DEL) $(subst \\,\,$(subst /,$(PS),$@))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_DEPLOY))
	$(DEPLOYTOOL) $(COMP_PACKAGE_URL) $(ENV_USER_INSTALL)
	@echo flagged > $(subst /,$(PS),$@)

.PHONY: clean
clean :
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_INC))
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_GEN))
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_SLO))
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_LIB))
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_BIN))
	-$(DELRECURSIVE) $(subst /,$(PS),$(OUT_COMP_DEPLOY))

.PHONY: dummy
dummy: $(COMP_UNOPKG_MANIFEST)
	@echo $(abspath $(subst \\,\,$(subst /,$(PS),../external)))

