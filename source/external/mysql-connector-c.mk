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

# mysql-connector-c-6.1.3-src.tar.gz
# http://dev.mysql.com/get/Downloads/Connector-C/mysql-connector-c-6.1.3-src.tar.gz

MYSQL_CONNECTOR_C=mysql-connector-c

MYSQL_CONNECTOR_C_LIB=libmysqlclient.$(SHAREDLIB_EXT)

MYSQL_CONNECTOR_C_MAJOR=6
MYSQL_CONNECTOR_C_MINOR=1
MYSQL_CONNECTOR_C_MICRO=3

MYSQL_CONNECTOR_C_VER=$(MYSQL_CONNECTOR_C)-$(MYSQL_CONNECTOR_C_MAJOR).$(MYSQL_CONNECTOR_C_MINOR).$(MYSQL_CONNECTOR_C_MICRO)
MYSQL_CONNECTOR_C_TAR_DIR=${MYSQL_CONNECTOR_C_VER}-src
MYSQL_CONNECTOR_C_TAR=$(MYSQL_CONNECTOR_C_TAR_DIR).tar.gz


MYSQL_CONNECTOR_C_DOWNLOAD=http://cdn.mysql.com/Downloads/Connector-C/$(MYSQL_CONNECTOR_C_TAR)
MYSQL_CONNECTOR_C_MD5=3161d47305234772db6d7ae30288a9ef

MYSQL_CONNECTOR_C_LICENSE= \
				COPYING \
				README
