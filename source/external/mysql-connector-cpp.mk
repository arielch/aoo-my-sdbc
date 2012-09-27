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

#mysql-connector-c++-1.1.1.tar.gz
#http://cdn.mysql.com/Downloads/Connector-C++/mysql-connector-c++-1.1.1.tar.gz
#MD5=3f3134ac39a5d56fdd360f2ad0121a99

MYSQL_CONNECTOR_CPP=mysql-connector-c++

MYSQL_CONNECTOR_CPP_MAJOR=1
MYSQL_CONNECTOR_CPP_MINOR=1
MYSQL_CONNECTOR_CPP_MICRO=1

MYSQL_CONNECTOR_CPP_VER=$(MYSQL_CONNECTOR_CPP)-$(MYSQL_CONNECTOR_CPP_MAJOR).$(MYSQL_CONNECTOR_CPP_MINOR).$(MYSQL_CONNECTOR_CPP_MICRO)
MYSQL_CONNECTOR_CPP_TAR=$(MYSQL_CONNECTOR_CPP_VER).tar.gz

MYSQL_CONNECTOR_CPP_LICENSE= \
				README \
				COPYING
