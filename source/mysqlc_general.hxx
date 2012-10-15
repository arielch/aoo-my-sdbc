/**************************************************************
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *************************************************************/

#ifndef _MYSQLC_GENERAL_
#define _MYSQLC_GENERAL_

#include <com/sun/star/logging/LogLevel.hpp>
#include <com/sun/star/logging/XLogger.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>

#include <preextstl.h>
#include <cppconn/exception.h>
#include <postextstl.h>


/*
 * creates a unicode-string from an ASCII string
 */
#define C2U( constAsciiStr ) \
    ( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( constAsciiStr ) ) )
/*
 * creates an ASCII string from a unicode-string
 */
#define U2C( ouString ) \
    ( rtl::OUStringToOString( ouString, RTL_TEXTENCODING_ASCII_US ).getStr() )

#define MYSQLC_URI_PREFIX           "sdbc:mysqlc:"
#define MYSQLDC_URI_PREFIX          "sdbc:mysqldc:"
#define MYSQLDC_URI_TCP             "tcp://"
#define MYSQLDC_URI_SOCKET          "unix://"
#define MYSQLDC_URI_PIPE            "pipe://"
#define MYSQLDC_LOCALHOST           "localhost"
#define MYSQLDC_LOCALHOSTIP         "127.0.0.1"
#define MYSQLDC_DEFAULT_PIPENAME    "MySQL"


#ifndef MYSQLDC_IMPLEMENTATION_NAME
#define MYSQLDC_IMPLEMENTATION_NAME "org.aoo-my-sdbc.comp.MysqlDCDriver"
#endif

namespace mysqlc
{
    rtl::OUString getStringFromAny( const ::com::sun::star::uno::Any &_rAny );

    void throwFeatureNotImplementedException(
        const sal_Char *_pAsciiFeatureName,
        const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > &_rxContext,
        const ::com::sun::star::uno::Any *_pNextException = NULL
    )
    throw ( ::com::sun::star::sdbc::SQLException );

    void throwInvalidArgumentException(
        const sal_Char *_pAsciiFeatureName,
        const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > &_rxContext,
        const ::com::sun::star::uno::Any *_pNextException = NULL
    )
    throw ( ::com::sun::star::sdbc::SQLException );

    void translateAndThrow( const ::sql::SQLException &_error, const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > &_context, const rtl_TextEncoding encoding );

    int mysqlToOOOType( int mysqlType ) throw ();


    ::rtl::OUString convert( const ::ext_std::string &_string, const rtl_TextEncoding encoding );

    ::ext_std::string convert( const ::rtl::OUString &_string, const rtl_TextEncoding encoding );


    inline void mysqlc_driver_log( const ::com::sun::star::uno::Reference< com::sun::star::logging::XLogger > &rxLogger,
                                   const sal_Int32 level,
                                   const rtl::OUString &sClass,
                                   const rtl::OUString &sMethod,
                                   const rtl::OUString &sMessage )
    {
        if ( rxLogger.is() )
        {
            rxLogger->logp( level, sClass, sMethod, sMessage );
        }
    }
}

#endif
