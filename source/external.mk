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

#----------------- Boost -------------------------

BOOST=boost
BOOST_TARFILE_NAME=boost_1_55_0

BOOST_TAR=$(EXTERNAL_DIR)/$(BOOST_TARFILE_NAME).tar.gz
BOOST_PATCH_FILE=$(PATCHDIR)/$(BOOST_TARFILE_NAME).patch
BOOST_INC_DIR_IN=$(OUT_COMP_GEN)/$(BOOST_TARFILE_NAME)/$(BOOST)
BOOST_INC_DIR=$(OUT_COMP_INC)
BOOST_INC=-I$(BOOST_INC_DIR)

BOOST_UNPACKED_FLAG=$(OUT_COMP_GEN)/so_unpacked_$(BOOST)
BOOST_PATCHED_FLAG=$(OUT_COMP_GEN)/so_patched_$(BOOST)
BOOST_DELIVER_FLAG=$(OUT_COMP_GEN)/so_deliver_$(BOOST)

#---------------- MySQL Connector C ---------------

include $(EXTERNAL_DIR)/mysql-connector-c.mk


MYSQLCONNC=mysqlconnc
MYSQLCONNC_DIR=$(OUT_COMP_GEN)
MYSQLCONNC_TAR=$(EXTERNAL_DIR)/$(MYSQL_CONNECTOR_C_TAR)


MYSQLCONNC_INSTALLDIR=$(OUT_COMP_GEN)/$(MYSQL_CONNECTOR_C_TAR_DIR)
#MYSQLCONNC_INSTALLDIR=$(MYSQLCONNC_DIR)/install_$(MYSQLCONNC)
MYSQLCONNC_LIB=libmysqlclient.$(SHAREDLIB_EXT)
MYSQLCONNC_INC_DIR=$(OUT_COMP_INC)/$(MYSQLCONNC)
MYSQLCONNC_INC=-I$(MYSQLCONNC_INC_DIR)

#ifeq "$(OS)" "WIN"
#MYSQLCONNC_LICENSES = $(foreach license,$(MYSQL_CONNECTOR_C_LICENSE),$(subst /,$(PS),$(OUT_COMP_GEN)/$(MYSQL_CONNECTOR_C_TAR_DIR)/$(license)))
#else
MYSQLCONNC_LICENSES = $(foreach license,$(MYSQL_CONNECTOR_C_LICENSE),$(subst /,$(PS),$(MYSQLCONNC_INSTALLDIR)/$(license)))
#endif

MYSQLCONNC_PATCH=$(PATCHDIR)/$(MYSQL_CONNECTOR_C_TAR_DIR).patch

MYSQLCONNC_UNPACKED_FLAG=$(MYSQLCONNC_DIR)/so_unpacked_$(MYSQLCONNC)
MYSQLCONNC_PATCHED_FLAG=$(MYSQLCONNC_DIR)/so_patched_$(MYSQLCONNC)
MYSQLCONNC_CONFIGURED_FLAG=$(MYSQLCONNC_DIR)/so_configured_$(MYSQLCONNC)
MYSQLCONNC_BUILD_FLAG=$(MYSQLCONNC_DIR)/so_built_$(MYSQLCONNC)
MYSQLCONNC_DELIVER_FLAG=$(MYSQLCONNC_DIR)/so_deliver_$(MYSQLCONNC)

# TODO generator for WIN
ifeq "$(OS)" "WIN"
	MS_VC90_REDIST_DLL= \
			msvcr90.dll \
			msvcp90.dll \
			msvcm90.dll \
			Microsoft.VC90.CRT.manifest
	MS_VC90_REDIST_FILES=$(foreach msfile,$(MS_VC90_REDIST_DLL),\
	"$(subst \bin,\redist\x86\Microsoft.VC90.CRT,$(OO_SDK_CPP_HOME))\$(msfile)")
	# Hack: patch needs CRs
	PATCHDIR=external/win/
	CMAKEGENERATOR=Visual Studio 9 2008
	CMAKEBUILDCMD=devenv.com libmysql.sln /build $(CMAKE_BUILD_TYPE)
	MYSQL_CONNECTOR_C_LIBFILE=$(MYSQLCONNC_INSTALLDIR)/libmysql/$(CMAKE_BUILD_TYPE)/$(MYSQL_CONNECTOR_C_LIB)
	MYSQLCONNCPP_CFLAGS+=-DCPPDBC_WIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS
else
	CMAKEGENERATOR=Unix Makefiles
	CMAKEBUILDCMD=$(MAKE) VERBOSE=1 $(MAKEFLAGS) libmysql
	MYSQL_CONNECTOR_C_LIBFILE=$(MYSQLCONNC_INSTALLDIR)/libmysql/$(MYSQL_CONNECTOR_C_LIB)
endif


MYSQLCONNC_CMAKE_FLAGS= \
	-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	-DCMAKE_INSTALL_PREFIX=$(MYSQLCONNC_INSTALLDIR)

ifeq "$(OS)" "MACOSX"
ifeq "$(UNOPKG_PLATFORM)" "MacOSX_x86"
MYSQLCONNC_CMAKE_FLAGS += -DCMAKE_OSX_ARCHITECTURES=i386
endif
endif

CMAKECMD=$(CMAKE) \
	-G "$(CMAKEGENERATOR)" $(MYSQLCONNC_CMAKE_FLAGS)


#---------------- MySQL Connector C++ ---------------

include $(EXTERNAL_DIR)/mysql-connector-cpp.mk

MYSQLCONNCPP=mysqlconncpp
MYSQLCONNCPP_DIR=$(OUT_COMP_GEN)
MYSQLCONNCPP_TAR=$(EXTERNAL_DIR)/$(MYSQL_CONNECTOR_CPP_TAR)

MYSQLCONNCPP_INSTALLDIR=$(MYSQLCONNCPP_DIR)/$(MYSQLCONNCPP)_install
MYSQLCONNCPP_LIB=$(SHAREDLIB_PRE)mysqlcppconn.$(SHAREDLIB_EXT)
MYSQLCONNCPP_BUILD=$(MYSQLCONNCPP_DIR)/$(MYSQL_CONNECTOR_CPP_VER)

MYSQLCONNCPP_INC=-I$(OUT_COMP_INC)
MYSQLCONNCPP_INC+=-I$(OUT_COMP_INC)/cppconn

MYSQLCONNCPP_LICENSES=$(foreach license,$(MYSQL_CONNECTOR_CPP_LICENSE),$(subst /,$(PS),$(MYSQLCONNCPP_BUILD)/$(license)))
MYSQLCONNCPP_PATCH=$(PATCHDIR)/$(MYSQL_CONNECTOR_CPP_VER).patch

MYSQLCONNCPP_UNPACKED_FLAG=$(MYSQLCONNCPP_DIR)/so_unpacked_$(MYSQLCONNCPP)
MYSQLCONNCPP_PATCHED_FLAG=$(MYSQLCONNCPP_DIR)/so_patched_$(MYSQLCONNCPP)
MYSQLCONNCPP_BUILD_FLAG=$(MYSQLCONNCPP_DIR)/so_built_$(MYSQLCONNCPP)
MYSQLCONNCPP_DELIVER_FLAG=$(MYSQLCONNCPP_DIR)/so_deliver_$(MYSQLCONNCPP)

# this is for static build
MYSQLCONNCPP_CFLAGS+=-DCPPCONN_LIB_BUILD

#ifeq "$(OS)" "WIN"
MYSQLCONNCPP_LINK_FLAGS=$(LIBRARY_LINK_FLAGS)
#else
#MYSQLCONNCPP_LINK_FLAGS=$(COMP_LINK_FLAGS)
#endif

MYSQLCONNCPP_SLOFILES= \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_art_resultset.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_art_rset_metadata.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_connection.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_debug.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_driver.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_metadata.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_parameter_metadata.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_prepared_statement.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_ps_resultset.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_ps_resultset_metadata.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_resultbind.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_resultset.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_resultset_metadata.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_statement.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_uri.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_util.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/mysql_warning.$(OBJ_EXT) \
	\
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/library_loader.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/mysql_client_api.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/mysql_native_connection_wrapper.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/mysql_native_driver_wrapper.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/mysql_native_resultset_wrapper.$(OBJ_EXT) \
	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/mysql_native_statement_wrapper.$(OBJ_EXT)

#	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/libmysql_dynamic_proxy.$(OBJ_EXT) \
#	$(MYSQLCONNCPP_BUILD)/driver/nativeapi/libmysql_static_proxy.$(OBJ_EXT) \

# --------------- MySQL settings ------------------

# Connector C++ OBJ
$(MYSQLCONNCPP_SLOFILES): $(MYSQLCONNCPP_BUILD)/%.$(OBJ_EXT) : $(SDKTYPEFLAG) $(BOOST_DELIVER_FLAG) $(MYSQLCONNC_DELIVER_FLAG) $(MYSQLCONNCPP_DELIVER_FLAG)
	$(CC) -Dmysqlcppconn_EXPORTS \
		$(CC_FLAGS) \
		$(MYSQLCONNCPP_CFLAGS) \
		$(CC_INCLUDES) \
		-I$(OUT_COMP_INC) \
		$(MYSQLCONNCPP_INC) \
		$(MYSQLCONNC_INC) \
		$(BOOST_INC) \
		$(CC_DEFINES) $(CC_OUTPUT_SWITCH)$(subst /,$(PS),$@) $(basename $@).cpp


# Connector C++ Lib
ifeq "$(OS)" "WIN"
$(OUT_COMP_LIB)/$(MYSQLCONNCPP_LIB) : $(MYSQLCONNCPP_SLOFILES)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN))
	$(LINK) \
		$(MYSQLCONNCPP_LINK_FLAGS) \
		/OUT:$@ \
		/MAP:$(OUT_COMP_GEN)/$(subst $(SHAREDLIB_EXT),map,$(@F)) \
		$(MYSQLCONNCPP_SLOFILES) \
		msvcrt.lib msvcprt.lib kernel32.lib
	$(LINK_MANIFEST)
else
#$(SHAREDLIB_OUT)/%.$(SHAREDLIB_EXT) : $(SLOFILES) $(COMP_MAPFILE)
$(OUT_COMP_LIB)/$(MYSQLCONNCPP_LIB) : $(MYSQLCONNCPP_SLOFILES)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(LINK) \
		$(MYSQLCONNCPP_LINK_FLAGS) $(LINK_LIBS) -o $@ $(MYSQLCONNCPP_SLOFILES) \
		$(STLPORTLIB) \
		$(STC++LIB)

ifeq "$(OS)" "MACOSX"
# ToDo
#	$(INSTALL_NAME_URELIBS)  $@
endif
endif

#------------------------------ Boost ------------------------------------------

$(BOOST_UNPACKED_FLAG): $(BOOST_TAR)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	cd $(subst /,$(PS),$(@D)) && $(ZCAT) -d  "$(CURDIR)/$<" | $(TAR) xv
	@echo flagged > $(subst /,$(PS),$@)

$(BOOST_PATCHED_FLAG) : $(BOOST_PATCH_FILE) $(BOOST_UNPACKED_FLAG)
#	-$(MKDIR) $(subst /,$(PS),$(@D))
	cd $(subst /,$(PS),$(@D)/$(BOOST_TARFILE_NAME)) && $(PATCH) -p3 -i "$(CURDIR)/$<"
	@echo flagged > $(subst /,$(PS),$@)

$(BOOST_DELIVER_FLAG) : $(BOOST_PATCHED_FLAG)
	-$(DELRECURSIVE) $(subst /,$(PS),$(BOOST_INC_DIR)/boost)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_INC))
	$(COPYRECURSIVE) $(subst /,$(PS),$(BOOST_INC_DIR_IN)) $(subst /,$(PS),$(BOOST_INC_DIR)/boost)
	@echo flagged > $(subst /,$(PS),$@)

#--------------------------- MySQL Connector C ---------------------------------

$(MYSQLCONNC_UNPACKED_FLAG): $(MYSQLCONNC_TAR)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	cd $(subst /,$(PS),$(@D)) && $(ZCAT) -d  "$(CURDIR)/$<" | $(TAR) xv
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNC_PATCHED_FLAG) : $(MYSQLCONNC_PATCH) $(MYSQLCONNC_UNPACKED_FLAG)
	cd $(subst /,$(PS),$(@D)/$(MYSQL_CONNECTOR_C_TAR_DIR)) && $(PATCH) -p2 -i "$(CURDIR)/$<"
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNC_CONFIGURED_FLAG) : $(MYSQLCONNC_PATCHED_FLAG)
	cd $(subst /,$(PS),$(@D)/$(MYSQL_CONNECTOR_C_TAR_DIR)) && $(CMAKECMD)
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNC_BUILD_FLAG) : $(MYSQLCONNC_CONFIGURED_FLAG)
	cd $(subst /,$(PS),$(@D)/$(MYSQL_CONNECTOR_C_TAR_DIR)) && $(CMAKEBUILDCMD)
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNC_DELIVER_FLAG) : $(MYSQLCONNC_BUILD_FLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_INC))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE/$(MYSQLCONNC))	
	$(COPY) $(MYSQLCONNC_LICENSES) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE/$(MYSQLCONNC))
	$(COPYRECURSIVE) $(subst /,$(PS),$(MYSQLCONNC_INSTALLDIR)/include) $(subst /,$(PS),$(MYSQLCONNC_INC_DIR))
	@echo flagged > $(subst /,$(PS),$@)

$(OUT_COMP_LIB)/$(MYSQLCONNC_LIB): $(MYSQLCONNC_DELIVER_FLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(COPY) $(subst /,$(PS),$(MYSQL_CONNECTOR_C_LIBFILE)) $(subst /,$(PS),$@)


#------------------------- MySQL Connector C++ ---------------------------------

$(MYSQLCONNCPP_UNPACKED_FLAG): $(MYSQLCONNCPP_TAR)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	cd $(subst /,$(PS),$(@D)) && $(ZCAT) -d  "$(CURDIR)/$<" | $(TAR) xv
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNCPP_PATCHED_FLAG) : $(MYSQLCONNCPP_PATCH) $(MYSQLCONNCPP_UNPACKED_FLAG)
	cd $(subst /,$(PS),$(@D)/$(MYSQL_CONNECTOR_CPP_VER)) && $(PATCH) -p2 -i "$(CURDIR)/$<"
	@echo flagged > $(subst /,$(PS),$@)

$(MYSQLCONNCPP_DELIVER_FLAG) : $(MYSQLCONNCPP_PATCHED_FLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_INC)/cppconn/)
	$(COPYRECURSIVE) $(subst /,$(PS),$(MYSQLCONNCPP_BUILD)/cppconn/*.h) $(subst /,$(PS),$(OUT_COMP_INC)/cppconn)
	-$(MKDIR) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE/$(MYSQLCONNCPP))
	$(COPY) $(MYSQLCONNCPP_LICENSES) $(subst /,$(PS),$(OUT_COMP_GEN)/LICENSE/$(MYSQLCONNCPP))
	@echo flagged > $(subst /,$(PS),$@)

