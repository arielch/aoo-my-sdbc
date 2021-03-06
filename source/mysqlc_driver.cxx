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

#include "mysqlc_driver.hxx"
#include "mysqlc_connection.hxx"
#include "mysqlc_general.hxx"

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/logging/XLoggerPool.hpp>

#include <cppconn/exception.h>
#ifdef SYSTEM_MYSQL_CPPCONN
#include <mysql_driver.h>
#endif

#include <osl/module.hxx>
#include <osl/thread.h>
#include <osl/file.hxx>
#include <rtl/uri.hxx>


using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::logging;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace mysqlc;

using rtl::OUString;


MysqlCDriver::MysqlCDriver( const Reference< XComponentContext > &rxContext )
    : ODriver_BASE( m_aMutex )
    , m_xContext( rxContext )
#ifndef SYSTEM_MYSQL_CPPCONN
    , m_hCppConnModule( NULL )
    , m_bAttemptedLoadCppConn( false )
#endif
{
    OSL_TRACE( "mysqlc::MysqlCDriver::MysqlCDriver" );
    cppDriver = NULL;

    try
    {
        Reference< XLoggerPool > xLoggerPool(
            m_xContext->getValueByName( rtl::OUString(
                                       RTL_CONSTASCII_USTRINGPARAM( "/singletons/com.sun.star.logging.LoggerPool" ) ) ),
            UNO_QUERY_THROW );
        m_xLogger.set( xLoggerPool->getNamedLogger( rtl::OUString(
                           RTL_CONSTASCII_USTRINGPARAM( MY_SDBC_LOGGER ) ) ),
                       UNO_QUERY_THROW );
        if ( m_xLogger->isLoggable( com::sun::star::logging::LogLevel::INFO ) )
        {
            mysqlc_driver_log( m_xLogger,
                               com::sun::star::logging::LogLevel::INFO,
                               C2U( "mysqlc::MysqlCDriver" ), C2U( "MysqlCDriver" ),
                               C2U( "driver instantiated" ) );
        }
    }
    catch ( ... )
    {}
}


void MysqlCDriver::disposing()
{
    OSL_TRACE( "mysqlc::MysqlCDriver::disposing" );
    ::osl::MutexGuard aGuard( m_aMutex );

    // when driver will be destroied so all our connections have to be destroied as well
    for ( OWeakRefArray::iterator i = m_xConnections.begin(); m_xConnections.end() != i; ++i )
    {
        Reference< XComponent > xComp( i->get(), UNO_QUERY );
        if ( xComp.is() )
        {
            xComp->dispose();
        }
    }
    m_xConnections.clear();

    ODriver_BASE::disposing();
}


OUString SAL_CALL MysqlCDriver::getImplementationName()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getImplementationName" );
    return getImplementationName_static();
}


sal_Bool SAL_CALL MysqlCDriver::supportsService( const OUString &_rServiceName )
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::supportsService" );
    Sequence< OUString > aSupported( getSupportedServiceNames() );
    const OUString *pSupported = aSupported.getConstArray();
    const OUString *pEnd = pSupported + aSupported.getLength();
    for ( ; pSupported != pEnd && !pSupported->equals( _rServiceName ); ++pSupported ) {}

    return ( pSupported != pEnd );
}


Sequence< OUString > SAL_CALL MysqlCDriver::getSupportedServiceNames()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getSupportedServiceNames" );
    return getSupportedServiceNames_static();
}


extern "C" {
    static void SAL_CALL thisModule() {}
}

void MysqlCDriver::impl_initCppConn_lck_throw()
{
#ifdef SYSTEM_MYSQL_CPPCONN
    cppDriver = get_driver_instance();
#else
    if ( !m_bAttemptedLoadCppConn )
    {
        const OUString sModuleName = OUString( RTL_CONSTASCII_USTRINGPARAM( CPPCONN_LIB ) );
        m_hCppConnModule = osl_loadModuleRelative( &thisModule, sModuleName.pData, 0 );
        m_bAttemptedLoadCppConn = true;
    }

    // attempted to load - was it successful?
    if ( !m_hCppConnModule )
    {
        OSL_ENSURE( false, "MysqlCDriver::impl_initCppConn_lck_throw: could not load the " CPPCONN_LIB " library!" );
        throw SQLException(
            OUString( RTL_CONSTASCII_USTRINGPARAM( "Unable to load the " CPPCONN_LIB " library." ) ),
            *this,
            OUString( RTL_CONSTASCII_USTRINGPARAM( "08001" ) ),  // "unable to connect"
            0,
            Any()
        );
    }

    rtl::OUString sMySQLClientLib( RTL_CONSTASCII_USTRINGPARAM( MYSQL_LIB ) );

    ::rtl::OUString moduleBase;
    OSL_VERIFY( ::osl::Module::getUrlFromAddress( &thisModule, moduleBase ) );
    ::rtl::OUString sMySQLClientLibURL;
    try
    {
        sMySQLClientLibURL = ::rtl::Uri::convertRelToAbs( moduleBase, sMySQLClientLib.pData );
    }
    catch ( const ::rtl::MalformedUriException &e )
    {
#if OSL_DEBUG_LEVEL > 0
        ::rtl::OString sMessage( "OConnection::construct: malformed URI: " );
        sMessage += ::rtl::OUStringToOString( e.getMessage(), osl_getThreadTextEncoding() );
        OSL_ENSURE( false, sMessage.getStr() );
#else
        ( void )e; // silence compiler
#endif
    }

    ::rtl::OUString sMySQLClientLibPath;
    osl_getSystemPathFromFileURL( sMySQLClientLibURL.pData, &sMySQLClientLibPath.pData );

    // find the factory symbol
    const OUString sSymbolName = OUString( RTL_CONSTASCII_USTRINGPARAM( "get_driver_instance_by_name" ) );
    typedef void* ( * FGetMySQLDriver )( const char * const );

    const FGetMySQLDriver pFactoryFunction = ( FGetMySQLDriver )( osl_getFunctionSymbol( m_hCppConnModule, sSymbolName.pData ) );
    if ( !pFactoryFunction )
    {
        OSL_ENSURE( false, "MysqlCDriver::impl_initCppConn_lck_throw: could not find the factory symbol in " CPPCONN_LIB "!" );
        throw SQLException(
            OUString( RTL_CONSTASCII_USTRINGPARAM( CPPCONN_LIB " is invalid: missing the driver factory function." ) ),
            *this,
            OUString( RTL_CONSTASCII_USTRINGPARAM( "08001" ) ),  // "unable to connect"
            0,
            Any()
        );
    }

    try
    {
        cppDriver = static_cast< sql::Driver * >( ( *pFactoryFunction )(
            rtl::OUStringToOString( sMySQLClientLibPath, osl_getThreadTextEncoding() ).getStr() ) );
    }
    catch ( sql::InvalidArgumentException &e)
    {
        mysqlc::translateAndThrow( e, *this, getDefaultEncoding() );
    }

#endif
    if ( !cppDriver )
    {
        throw SQLException(
            OUString( RTL_CONSTASCII_USTRINGPARAM( "Unable to obtain the MySQL_Driver instance from Connector/C++." ) ),
            *this,
            OUString( RTL_CONSTASCII_USTRINGPARAM( "08001" ) ),  // "unable to connect"
            0,
            Any()
        );
    }
}

Reference< XConnection > SAL_CALL MysqlCDriver::connect( const OUString &url, const Sequence< PropertyValue > &info )
throw( SQLException, RuntimeException )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    OSL_TRACE( "mysqlc::MysqlCDriver::connect" );
    if ( !acceptsURL( url ) )
    {
        return NULL;
    }

    if ( !cppDriver )
    {
        impl_initCppConn_lck_throw();
        OSL_POSTCOND( cppDriver, "MySQLCDriver::connect: internal error." );
        if ( !cppDriver )
            throw RuntimeException( OUString( RTL_CONSTASCII_USTRINGPARAM( "MySQLCDriver::connect: internal error." ) ), *this );
    }

    Reference< XConnection > xConn;
    // create a new connection with the given properties and append it to our vector
    try
    {
        OConnection *pCon = new OConnection( m_xContext, url, *this, cppDriver );
        xConn = pCon;

        pCon->connect( info );
        m_xConnections.push_back( WeakReferenceHelper( *pCon ) );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getDefaultEncoding() );
    }
    return xConn;
}


sal_Bool SAL_CALL MysqlCDriver::acceptsURL( const OUString &url )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::acceptsURL" );
    return ( /*!url.compareToAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_PREFIX ) ) ||*/
             !url.compareToAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLC_URI_PREFIX ) ) );
}


Sequence< DriverPropertyInfo > SAL_CALL MysqlCDriver::getPropertyInfo( const OUString &url, const Sequence< PropertyValue > & /* info */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getPropertyInfo" );
    if ( acceptsURL( url ) )
    {
        ::std::vector< DriverPropertyInfo > aDriverInfo;

        aDriverInfo.push_back( DriverPropertyInfo(
                                   OUString( RTL_CONSTASCII_USTRINGPARAM( "Hostname" ) )
                                   , OUString( RTL_CONSTASCII_USTRINGPARAM( "Name of host" ) )
                                   , sal_True
                                   , OUString( RTL_CONSTASCII_USTRINGPARAM( "localhost" ) )
                                   , Sequence< OUString >() )
                             );
        aDriverInfo.push_back( DriverPropertyInfo(
                                   OUString( RTL_CONSTASCII_USTRINGPARAM( "Port" ) )
                                   , OUString( RTL_CONSTASCII_USTRINGPARAM( "Port" ) )
                                   , sal_True
                                   , OUString( RTL_CONSTASCII_USTRINGPARAM( "3306" ) )
                                   , Sequence< OUString >() )
                             );
        return Sequence< DriverPropertyInfo >( &( aDriverInfo[0] ), aDriverInfo.size() );
    }

    return Sequence< DriverPropertyInfo >();
}


sal_Int32 SAL_CALL MysqlCDriver::getMajorVersion()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getMajorVersion" );
    return MYSQLC_VERSION_MAJOR;
}


sal_Int32 SAL_CALL MysqlCDriver::getMinorVersion()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getMinorVersion" );
    return MYSQLC_VERSION_MINOR;
}


OUString SAL_CALL MysqlCDriver::getImplementationName_static()
throw ( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getImplementationName_Static" );
    return OUString( RTL_CONSTASCII_USTRINGPARAM( MYSQLDC_IMPLEMENTATION_NAME ) );
}


Sequence< OUString > SAL_CALL MysqlCDriver::getSupportedServiceNames_static()
throw ( RuntimeException )
{
    OSL_TRACE( "mysqlc::MysqlCDriver::getSupportedServiceNames_Static" );
    // which service is supported
    // for more information @see com.sun.star.sdbc.Driver
    Sequence< OUString > aSNS( 1 );
    aSNS[0] = OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.sdbc.Driver" ) );
    return aSNS;
}


Reference< XInterface >  SAL_CALL MysqlCDriver::CreateInstance( const Reference< XComponentContext > &rxContext )
throw ( RuntimeException )
{
    return( *( new MysqlCDriver( rxContext ) ) );
}

namespace mysqlc
{
    void release( oslInterlockedCount &_refCount,
                  ::cppu::OBroadcastHelper &rBHelper,
                  Reference< XInterface > &_xInterface,
                  ::com::sun::star::lang::XComponent *_pObject )
    {
        if ( osl_decrementInterlockedCount( &_refCount ) == 0 )
        {
            osl_incrementInterlockedCount( &_refCount );

            if ( !rBHelper.bDisposed && !rBHelper.bInDispose )
            {
                // remember the parent
                Reference< XInterface > xParent;
                {
                    ::osl::MutexGuard aGuard( rBHelper.rMutex );
                    xParent = _xInterface;
                    _xInterface = NULL;
                }

                // First dispose
                _pObject->dispose();

                // only the alive ref holds the object
                OSL_ASSERT( _refCount == 1 );

                // release the parent in the destructor
                if ( xParent.is() )
                {
                    ::osl::MutexGuard aGuard( rBHelper.rMutex );
                    _xInterface = xParent;
                }
            }
        }
        else
        {
            osl_incrementInterlockedCount( &_refCount );
        }
    }

    void checkDisposed( sal_Bool _bThrow )
    throw ( DisposedException )
    {
        if ( _bThrow )
        {
            throw DisposedException();
        }
    }

} /* mysqlc */

