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

#include "mysqlc_connection.hxx"
#include "mysqlc_databasemetadata.hxx"
#include "mysqlc_driver.hxx"
#include "mysqlc_statement.hxx"
#include "mysqlc_preparedstatement.hxx"
#include "mysqlc_general.hxx"

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/metadata.h>
#include <cppconn/exception.h>

#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/TransactionIsolation.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/beans/NamedValue.hpp>

#include <osl/file.hxx>
#include <rtl/uri.hxx>
#include <rtl/ustrbuf.hxx>


using namespace com::sun::star::uno;
using namespace com::sun::star::container;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace mysqlc;

using ::osl::MutexGuard;
using ::rtl::OUString;
using ::rtl::OUStringToOString;


bool lcl_CheckSocket( const OUString &sSock )
{
    OUString sSockUrl;
    osl::FileBase::RC nError;
    nError = osl::FileBase::getFileURLFromSystemPath( sSock, sSockUrl );
    if ( nError != osl::FileBase::E_None )
        return false;

    osl::DirectoryItem aItem;
    nError = osl::DirectoryItem::get( sSock, aItem );
    if ( nError != osl::FileBase::E_None )
        return false;

    osl::FileStatus aStatus( FileStatusMask_Type );
    nError = aItem.getFileStatus( aStatus );
    if ( nError != osl::FileBase::E_None || !aStatus.isValid( FileStatusMask_Type ) )
        return false;

    return aStatus.getFileType() == osl::FileStatus::Socket;
}


OConnection::OConnection(
    const Reference< XComponentContext > &rxContext, const OUString &rURL, MysqlCDriver &_rDriver, sql::Driver *cppDriver )
    : OMetaConnection_BASE( m_aMutex )
    , OSubComponent<OConnection, OConnection_BASE>( ( ::cppu::OWeakObject * )&_rDriver, this )
    , m_xContext( rxContext )
    , m_aSettings( new ConnectionSettings )
    , m_xMetaData( NULL )
    , m_rDriver( _rDriver )
    , cppDriver( cppDriver )
    , m_bClosed( sal_False )
    , m_bUseCatalog( sal_False )
    , m_bUseOldDateFormat( sal_False )
{
    OSL_TRACE( "mysqlc::OConnection::OConnection" );
    m_rDriver.acquire();

    m_aSettings->connectionURL = rURL;
}


OConnection::~OConnection()
{
    OSL_TRACE( "mysqlc::OConnection::~OConnection" );

    // the connection should be already closed and disposed

    m_rDriver.release();
}


bool OConnection::parseURL()
{
    // parse url. Url has the following format:
    // external server: sdbc:mysqlc:[hostname]:[port]/[dbname]

    // TODO the url should be parsed, and info taken into account!
    //
    // The Application should send a well-formed URL when choosing a
    // socket/named-pipe, actually it sends sdbc:mysqlc:localhost:3306/dbname
    // and the socket name is sent to the Driver on the property
    // "LocalSocket"/"NamedPipe" ONLY in the Wizard, but NOT upon connection
    //
    // URL Protocols (see MySQL_Connection::init)
    //
    //  Unix Socket:        unix://socket_file/schema
    //  Win Named Pipe:     pipe://pipe_name/schema
    //  TCP:                tcp://IP[:port]/schema
    //

    OUString sHostAndPort, sSchema, sURLNoShcema;

    // split "protocols/shcema"
    sal_Int32 nIndex = m_aSettings->connectionURL.lastIndexOf( sal_Unicode( '/' ) );
    if ( nIndex == -1 )
        return false;

    sURLNoShcema = m_aSettings->connectionURL.copy( 0, nIndex );
    sSchema = m_aSettings->connectionURL.copy( nIndex + 1 );

    // schema is mandatory
    if ( !sURLNoShcema.getLength() || !sSchema.getLength() )
        return false;

    m_aSettings->sSchema = sSchema;

    // remove "sdbc:mysqldc:"
    sURLNoShcema = sURLNoShcema.copy( sizeof( MYSQLC_URI_PREFIX ) - 1 );
    // for the MySQLConn/C++ remove "sdbc:mysqldc:" but leave the schema
    m_aSettings->sConnCppUri = m_aSettings->connectionURL.copy( sizeof( MYSQLC_URI_PREFIX ) - 1 );

    // compare the protocols
    if ( 0 == sURLNoShcema.compareToAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_TCP ) ) )
    {
        m_aSettings->eProtocol = CP_TCP;
        m_aSettings->sHostName = sURLNoShcema.copy( sizeof( MYSQLDC_URI_TCP ) - 1 );
        sHostAndPort = m_aSettings->sHostName;
    }
    else if ( 0 == sURLNoShcema.compareToAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_SOCKET ) ) )
    {
#ifdef UNX
        m_aSettings->eProtocol = CP_SOCKET;
        m_aSettings->sSocketOrPipe = sURLNoShcema.copy( sizeof( MYSQLDC_URI_SOCKET ) - 1 );
        m_aSettings->sHostName = C2U( MYSQLDC_LOCALHOST );

        OSL_ENSURE( lcl_CheckSocket( m_aSettings->sSocketOrPipe ), "Invalid socket!" );

        // TODO return true only if socket exists
        return true;
#else
        return false;
#endif
    }
    else if ( 0 == sURLNoShcema.compareToAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_PIPE ) ) )
    {
#ifdef WNT
        m_aSettings->eProtocol = CP_PIPE;
        m_aSettings->sSocketOrPipe = sURLNoShcema.copy( sizeof( MYSQLDC_URI_PIPE ) - 1 );
        m_aSettings->sHostName = C2U( "." );

        return m_aSettings->sSocketOrPipe.getLength();
#else
        return false;
#endif
    }
    else
    {
        // URL pre AOO 3.5
        sHostAndPort = sURLNoShcema;
    }

    if ( sHostAndPort.getLength() )
    {
        OUString sHostName, sPort;
        int nPort = 3306;
        sal_Int32 nIndex = 0;

        // slplit "host:port"
        nIndex = 0;
        sHostName = sHostAndPort.getToken( 0, sal_Unicode( ':' ), nIndex ) ;
        if ( nIndex != -1 )
        {
            sPort = sHostAndPort.copy( nIndex );
            // if ':' is given, then it must be a valid port
            if ( !sHostName.getLength()
                    || !sPort.getLength()
                    || !( nPort = sPort.toInt32() ) )
                return false;

            m_aSettings->sHostName = sHostName;
        }
        else
            m_aSettings->sHostName = sHostAndPort;

        // TODO we shouldn't convert "localhost" to "127.0.0.1"
        // this will hide errors in Base because it will connect via
        // TCP/IP with default port even if a socket/named pipe was set
        // but ignored by Base
        if ( m_aSettings->sHostName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_LOCALHOST ) ) )
            m_aSettings->sHostName = C2U( MYSQLDC_LOCALHOSTIP );

        m_aSettings->nPort = nPort;
        m_aSettings->eProtocol = CP_TCP;

        return true;
    }

    m_aSettings->eProtocol = CP_INVALID;
    return false;
}


rtl_TextEncoding OConnection::getConnectionEncoding()
{
    return m_aSettings->encoding;
}


void SAL_CALL OConnection::release()
throw()
{
    OSL_TRACE( "mysqlc::OConnection::release" );
    relase_ChildImpl();
}

#ifndef SYSTEM_MYSQL
extern "C" {
    void SAL_CALL thisModule() {}
}
#endif

void OConnection::connect( const Sequence< PropertyValue > &info )
throw( SQLException )
{
    OSL_TRACE( "mysqlc::OConnection::construct" );
    MutexGuard aGuard( m_aMutex );

    // don't be so strict, for now
    parseURL();
#if 0
    if ( !parseURL() )
        throw SQLException(
            C2U( "Malformed connection URL" ), *this, OUString(), 0, Any() );
#endif

    m_aSettings->encoding = m_rDriver.getDefaultEncoding();
    m_aSettings->quoteIdentifier = OUString();

    // get user and password for mysql connection
    OUString aUser, aPass, sSocketOrPipe;
    bool bSocketOrPipe = false;
    const PropertyValue *pIter = info.getConstArray();
    const PropertyValue *pEnd  = pIter + info.getLength();

    for ( ; pIter != pEnd; ++pIter )
    {
        if ( !pIter->Name.compareToAscii( RTL_CONSTASCII_STRINGPARAM( "user" ) ) )
        {
            OSL_VERIFY( pIter->Value >>= aUser );
        }
        else if ( !pIter->Name.compareToAscii( RTL_CONSTASCII_STRINGPARAM( "password" ) ) )
        {
            OSL_VERIFY( pIter->Value >>= aPass );
#ifdef UNX
        }
        else if ( !pIter->Name.compareToAscii( RTL_CONSTASCII_STRINGPARAM( "LocalSocket" ) ) )
        {
            bSocketOrPipe = ( pIter->Value >>= sSocketOrPipe ) && sSocketOrPipe.getLength();
#else
        }
        else if ( !pIter->Name.compareToAscii( RTL_CONSTASCII_STRINGPARAM( "NamedPipe" ) ) )
        {
            bSocketOrPipe = ( pIter->Value >>= sSocketOrPipe ) && sSocketOrPipe.getLength();
#endif
        }
        else if ( !pIter->Name.compareToAscii( RTL_CONSTASCII_STRINGPARAM( "PublicConnectionURL" ) ) )
        {
            // sdbc:mysql:mysqlc:
            OUString sPublicConnURL;
            OSL_VERIFY( pIter->Value >>= sPublicConnURL );
            if (sPublicConnURL.getLength())
                m_aSettings->connectionURL = sPublicConnURL;
        }
    }

    if ( bSocketOrPipe )
        m_aSettings->sSocketOrPipe = sSocketOrPipe;

    if ( !aUser.getLength() || !aPass.getLength() )
        throw SQLException(
            C2U( "User and password are mandatory" ), *this, OUString(), 0, Any() );

    try
    {
        sql::ConnectOptionsMap connProps;

        std::string user_str   = OUStringToOString( aUser, m_aSettings->encoding ).getStr();
        std::string pass_str   = OUStringToOString( aPass, m_aSettings->encoding ).getStr();
        std::string schema_str = OUStringToOString( m_aSettings->sSchema, m_aSettings->encoding ).getStr();

        connProps["userName"] = sql::ConnectPropertyVal( user_str );
        connProps["password"] = sql::ConnectPropertyVal( pass_str );
        connProps["schema"]   = sql::ConnectPropertyVal( schema_str );

        OUString aHostName;
        std::string host_str;
        sql::SQLString socket_str;

        if ( m_aSettings->sSocketOrPipe.getLength() )
        {
            socket_str = OUStringToOString( m_aSettings->sSocketOrPipe, m_aSettings->encoding ).getStr();
            rtl::OUStringBuffer aBuff;
#ifdef UNX
            aBuff.appendAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_SOCKET ) );
            connProps["socket"] = socket_str;
#else
            aBuff.appendAscii( RTL_CONSTASCII_STRINGPARAM( MYSQLDC_URI_PIPE ) );
            connProps["pipe"] = socket_str;

            // Set connection timeout explicitly to 0 when using a named pipe
            // mysql_init() sets connection timeout to CONNECT_TIMEOUT
            // which is 20 ifdef __WIN__ (see libmysql/client.c)
            // This fails to connect via named pipe with error
            // "Lost connection to MySQL server at
            // 'waiting for initial communication packet', system error: 0"
            connProps["OPT_CONNECT_TIMEOUT"] = static_cast<long>( 0 );
#endif
            aBuff.append( m_aSettings->sSocketOrPipe );
            aHostName = aBuff.makeStringAndClear();
            host_str = OUStringToOString( aHostName, m_aSettings->encoding ).getStr();
            connProps["hostName"] = sql::ConnectPropertyVal( host_str );
        }
        else
        {
            host_str   = OUStringToOString( m_aSettings->sHostName, m_aSettings->encoding ).getStr();
            connProps["hostName"] = sql::ConnectPropertyVal( host_str );
            connProps["port"]     = sql::ConnectPropertyVal( ( int )( m_aSettings->nPort ) );
        }

        m_aSettings->cppConnection.reset( cppDriver->connect( connProps ) );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }

    // Check if the server is 4.1 or above
    if ( this->getMysqlVersion() < 40100 )
    {
        throw SQLException(
            ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "MySQL Connector/OO.org requires MySQL Server 4.1 or above" ) ),
            *this,
            ::rtl::OUString(),
            0,
            Any() );
    }
    std::auto_ptr<sql::Statement> stmt( m_aSettings->cppConnection->createStatement() );
    stmt->executeUpdate( "SET session sql_mode='ANSI_QUOTES'" );
    stmt->executeUpdate( "SET NAMES utf8" );
}


// XServiceInfo
IMPLEMENT_SERVICE_INFO( OConnection, "com.sun.star.sdbc.drivers.mysqlc.OConnection", "com.sun.star.sdbc.Connection" )


Reference< XStatement > SAL_CALL OConnection::createStatement()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::createStatement" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    // create a statement
    Reference< XStatement > xReturn;
    // the statement can only be executed once
    try
    {
        xReturn = new OStatement( this, m_aSettings->cppConnection->createStatement() );
        m_aStatements.push_back( WeakReferenceHelper( xReturn ) );
        return xReturn;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return xReturn;
}


Reference< XPreparedStatement > SAL_CALL OConnection::prepareStatement( const OUString &_sSql )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::prepareStatement" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );
    const ::rtl::OUString sSqlStatement = transFormPreparedStatement( _sSql );

    Reference< XPreparedStatement > xStatement;
    try
    {
        // create a statement
        // the statement can only be executed more than once
        xStatement = new OPreparedStatement( this,
                                             m_aSettings->cppConnection->prepareStatement( OUStringToOString( sSqlStatement, getConnectionEncoding() ).getStr() ) );
        m_aStatements.push_back( WeakReferenceHelper( xStatement ) );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return xStatement;
}


Reference< XPreparedStatement > SAL_CALL OConnection::prepareCall( const OUString & /*_sSql*/ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::prepareCall" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    mysqlc::throwFeatureNotImplementedException( "OConnection::prepareCall", *this );
    return Reference< XPreparedStatement >();
}


OUString SAL_CALL OConnection::nativeSQL( const OUString &_sSql )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::nativeSQL" );
    MutexGuard aGuard( m_aMutex );

    const ::rtl::OUString sSqlStatement = transFormPreparedStatement( _sSql );
    ::rtl::OUString sNativeSQL;
    try
    {
        sNativeSQL = mysqlc::convert( m_aSettings->cppConnection->nativeSQL( mysqlc::convert( sSqlStatement, getConnectionEncoding() ) ),
                                      getConnectionEncoding() );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return sNativeSQL;
}


void SAL_CALL OConnection::setAutoCommit( sal_Bool autoCommit )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::setAutoCommit" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );
    try
    {
        m_aSettings->cppConnection->setAutoCommit( autoCommit == sal_True ? true : false );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
}


sal_Bool SAL_CALL OConnection::getAutoCommit()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getAutoCommit" );
    // you have to distinguish which if you are in autocommit mode or not
    // at normal case true should be fine here

    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    sal_Bool autoCommit = sal_False;
    try
    {
        autoCommit = m_aSettings->cppConnection->getAutoCommit() == true ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return autoCommit;
}


void SAL_CALL OConnection::commit()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::commit" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );
    try
    {
        m_aSettings->cppConnection->commit();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
}


void SAL_CALL OConnection::rollback()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::rollback" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );
    try
    {
        m_aSettings->cppConnection->rollback();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
}


sal_Bool SAL_CALL OConnection::isClosed()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::isClosed" );
    MutexGuard aGuard( m_aMutex );

    return m_bClosed;
}


Reference< XDatabaseMetaData > SAL_CALL OConnection::getMetaData()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getMetaData" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    Reference< XDatabaseMetaData > xMetaData = m_xMetaData;
    if ( !xMetaData.is() )
    {
        try
        {
            xMetaData = new ODatabaseMetaData( m_xContext, *this ); // need the connection because it can return it
        }
        catch ( sql::SQLException &e )
        {
            mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
        }
        m_xMetaData = xMetaData;
    }

    return xMetaData;
}


void SAL_CALL OConnection::setReadOnly( sal_Bool readOnly )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::setReadOnly" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    m_aSettings->isReadOnly = readOnly;
}


sal_Bool SAL_CALL OConnection::isReadOnly()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::isReadOnly" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    // return if your connection to readonly
    return ( m_aSettings->isReadOnly );
}


void SAL_CALL OConnection::setCatalog( const OUString &catalog )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::setCatalog" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    try
    {
        //        m_aSettings->cppConnection->setCatalog(OUStringToOString(catalog, m_aSettings->encoding).getStr());
        m_aSettings->cppConnection->setSchema( OUStringToOString( catalog, getConnectionEncoding() ).getStr() );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
}


OUString SAL_CALL OConnection::getCatalog()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getCatalog" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    OUString catalog;
    try
    {
        catalog = mysqlc::convert( m_aSettings->cppConnection->getSchema(), getConnectionEncoding() );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return catalog;
}


void SAL_CALL OConnection::setTransactionIsolation( sal_Int32 level )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::setTransactionIsolation" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    sql::enum_transaction_isolation cpplevel = sql::TRANSACTION_SERIALIZABLE;

    switch ( level )
    {
        case TransactionIsolation::READ_UNCOMMITTED:
            cpplevel = sql::TRANSACTION_READ_UNCOMMITTED;
            break;
        case TransactionIsolation::READ_COMMITTED:
            cpplevel = sql::TRANSACTION_READ_COMMITTED;
            break;
        case TransactionIsolation::REPEATABLE_READ:
            cpplevel = sql::TRANSACTION_REPEATABLE_READ;
            break;
        case TransactionIsolation::SERIALIZABLE:
            cpplevel = sql::TRANSACTION_SERIALIZABLE;
            break;
        case TransactionIsolation::NONE:
            cpplevel = sql::TRANSACTION_SERIALIZABLE;
            break;
        default:;
            /* XXX: Exception ?? */
    }
    try
    {
        m_aSettings->cppConnection->setTransactionIsolation( cpplevel );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
}


sal_Int32 SAL_CALL OConnection::getTransactionIsolation()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getTransactionIsolation" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    try
    {
        switch ( m_aSettings->cppConnection->getTransactionIsolation() )
        {
            case sql::TRANSACTION_SERIALIZABLE:        return TransactionIsolation::SERIALIZABLE;
            case sql::TRANSACTION_REPEATABLE_READ:    return TransactionIsolation::REPEATABLE_READ;
            case sql::TRANSACTION_READ_COMMITTED:    return TransactionIsolation::READ_COMMITTED;
            case sql::TRANSACTION_READ_UNCOMMITTED:    return TransactionIsolation::READ_UNCOMMITTED;
            default:
                ;
        }
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return TransactionIsolation::NONE;
}


Reference<XNameAccess> SAL_CALL OConnection::getTypeMap()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getTypeMap" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    Reference<XNameAccess > t;
    {
        t = m_typeMap;
    }
    return ( t );
}


void SAL_CALL OConnection::setTypeMap( const Reference<XNameAccess > &typeMap )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::setTypeMap" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    m_typeMap = typeMap;
}


// XCloseable
void SAL_CALL OConnection::close()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::close" );

    MutexGuard aGuard( m_aMutex );

    // first throw if we are already disposed
    // Client code's workflow should be
    // 1) xConnection.close()
    // 2) xConnection.dispose()
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    // XCloseable does not specify to throw if already closed
    // so we don't throw in this case
    if ( m_aSettings->cppConnection.get() )
    {
        try
        {
            m_aSettings->cppConnection->close();
            m_aSettings->cppConnection.reset();
            m_bClosed = sal_True;
        }
        catch ( sql::SQLException &e )
        {
            mysqlc::translateAndThrow(e, *this, getConnectionEncoding());
        }
    }

    // do NOT dispose on close
    // client code should do it explicitly
    // dispose();
}


// XWarningsSupplier
Any SAL_CALL OConnection::getWarnings()
throw( SQLException, RuntimeException )
{
    Any x = Any();
    OSL_TRACE( "mysqlc::OConnection::getWarnings" );
    // when you collected some warnings -> return it
    return x;
}


void SAL_CALL OConnection::clearWarnings()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::clearWarnings" );
    // you should clear your collected warnings here#
}


void OConnection::buildTypeInfo()
throw( SQLException )
{
    OSL_TRACE( "mysqlc::OConnection::buildTypeInfo" );
}


void OConnection::disposing()
{
    OSL_TRACE( "mysqlc::OConnection::disposing" );
    // we noticed that we should be destroied in near future so we have to dispose our statements
    MutexGuard aGuard( m_aMutex );

    for ( OWeakRefArray::iterator i = m_aStatements.begin(); i != m_aStatements.end() ; ++i )
    {
        Reference< XComponent > xComp( i->get(), UNO_QUERY );
        if ( xComp.is() )
        {
            xComp->dispose();
        }
    }

    m_aStatements.clear();

    OSL_ENSURE( m_bClosed, "Disposing a connection that is not closed!" );
    if ( m_aSettings->cppConnection.get() )
    {
        try
        {
            m_aSettings->cppConnection->close();
            m_bClosed = sal_True;
        }
        catch (...) {}
    }

    m_xMetaData    = WeakReference< XDatabaseMetaData >();

    dispose_ChildImpl();
    OConnection_BASE::disposing();
}


/* ToDo - upcast the connection to MySQL_Connection and use ::getSessionVariable() */

OUString OConnection::getMysqlVariable( const char *varname )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getMysqlVariable" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    OUString ret;
    ::rtl::OUStringBuffer aStatement;
    aStatement.appendAscii( "SHOW SESSION VARIABLES LIKE '" );
    aStatement.appendAscii( varname );
    aStatement.append( sal_Unicode( '\'' ) );

    try
    {
        XStatement *stmt = new OStatement( this, m_aSettings->cppConnection->createStatement() );
        Reference< XResultSet > rs = stmt->executeQuery( aStatement.makeStringAndClear() );
        if ( rs.is() && rs->next() )
        {
            Reference< XRow > xRow( rs, UNO_QUERY );
            ret = xRow->getString( 2 );
        }
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }

    return ret;
}


sal_Int32 OConnection::getMysqlVersion()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OConnection::getMysqlVersion" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OConnection_BASE::rBHelper.bDisposed );

    sal_Int32 version( 0 );
    try
    {
        version = 10000 * m_aSettings->cppConnection->getMetaData()->getDatabaseMajorVersion();
        version += 100 * m_aSettings->cppConnection->getMetaData()->getDatabaseMinorVersion();
        version += m_aSettings->cppConnection->getMetaData()->getDatabasePatchVersion();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, getConnectionEncoding() );
    }
    return version;
}


// TODO: Not used
//sal_Int32 OConnection::sdbcColumnType(OUString typeName)
//{
//    OSL_TRACE("mysqlc::OConnection::sdbcColumnType");
//    int i = 0;
//    while (mysqlc_types[i].typeName) {
//        if (OUString::createFromAscii(mysqlc_types[i].typeName).equals(
//            typeName.toAsciiUpperCase()))
//        {
//            return mysqlc_types[i].dataType;
//        }
//        i++;
//    }
//    return 0;
//}


::rtl::OUString OConnection::transFormPreparedStatement( const ::rtl::OUString &_sSQL )
{
    ::rtl::OUString sSqlStatement = _sSQL;
    if ( !m_xParameterSubstitution.is() )
    {
        try
        {
            Sequence< Any > aArgs( 1 );
            Reference< XConnection> xCon = this;
            aArgs[0] <<= NamedValue( OUString( RTL_CONSTASCII_USTRINGPARAM( "ActiveConnection" ) ), makeAny( xCon ) );

            m_xParameterSubstitution.set(
                m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                    OUString( RTL_CONSTASCII_USTRINGPARAM( "org.openoffice.comp.helper.ParameterSubstitution" ) ),
                    aArgs,
                    m_xContext ), UNO_QUERY );
        }
        catch ( const Exception & ) {}
    }
    if ( m_xParameterSubstitution.is() )
    {
        try
        {
            sSqlStatement = m_xParameterSubstitution->substituteVariables( sSqlStatement, sal_True );
        }
        catch ( const Exception & ) { }
    }
    return sSqlStatement;
}

