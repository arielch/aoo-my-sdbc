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

#include "mysqlc_general.hxx"
#include "mysqlc_preparedstatement.hxx"
#include "mysqlc_propertyids.hxx"
#include "mysqlc_resultsetmetadata.hxx"

#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/sdbc/DataType.hpp>

#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/parameter_metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>
#include <cppuhelper/typeprovider.hxx>

#include <osl/diagnose.h>
#include <rtl/strbuf.hxx>

using namespace mysqlc;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::container;
using namespace com::sun::star::io;
using namespace com::sun::star::util;
using ::osl::MutexGuard;
using mysqlc::getStringFromAny;


IMPLEMENT_SERVICE_INFO( OPreparedStatement, "com.sun.star.sdbcx.mysqlc.PreparedStatement", "com.sun.star.sdbc.PreparedStatement" );


OPreparedStatement::OPreparedStatement( OConnection *_pConnection, sql::PreparedStatement *_cppPrepStmt )
    : OCommonStatement( _pConnection, _cppPrepStmt )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::OPreparedStatement" );
    m_pConnection = _pConnection;
    m_pConnection->acquire();

    try
    {
        m_paramCount = ( ( sql::PreparedStatement * )cppStatement )->getParameterMetaData()->getParameterCount();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


OPreparedStatement::~OPreparedStatement()
{
    OSL_TRACE( "mysqlc::OPreparedStatement::~OPreparedStatement" );
}


void SAL_CALL OPreparedStatement::acquire()
throw()
{
    OSL_TRACE( "mysqlc::OPreparedStatement::acquire" );
    OCommonStatement::acquire();
}


void SAL_CALL OPreparedStatement::release()
throw()
{
    OSL_TRACE( "mysqlc::OPreparedStatement::release" );
    OCommonStatement::release();
}


Any SAL_CALL OPreparedStatement::queryInterface( const Type &rType )
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::queryInterface" );
    Any aRet = OCommonStatement::queryInterface( rType );
    if ( !aRet.hasValue() )
    {
        aRet = OPreparedStatement_BASE::queryInterface( rType );
    }
    return ( aRet );
}


Sequence< Type > SAL_CALL OPreparedStatement::getTypes()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::getTypes" );
    return concatSequences( OPreparedStatement_BASE::getTypes(), OCommonStatement::getTypes() );
}


Reference< XResultSetMetaData > SAL_CALL OPreparedStatement::getMetaData()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::getMetaData" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    try
    {
        if ( !m_xMetaData.is() )
        {
            m_xMetaData = new OResultSetMetaData(
                ( ( sql::PreparedStatement * )cppStatement )->getMetaData(),
                getOwnConnection()->getConnectionEncoding()
            );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
    return m_xMetaData;
}


void SAL_CALL OPreparedStatement::close()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::close" );

    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    try
    {
        clearWarnings();
        clearParameters();
        OCommonStatement::close();
    }
    catch ( SQLException )
    {
        // If we get an error, ignore
    }

    // Remove this Statement object from the Connection object's
    // list
}


sal_Bool SAL_CALL OPreparedStatement::execute()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::execute" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    sal_Bool success = sal_False;
    try
    {
        success = ( ( sql::PreparedStatement * )cppStatement )->execute() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
    return success;
}


sal_Int32 SAL_CALL OPreparedStatement::executeUpdate()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::executeUpdate" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    sal_Int32 affectedRows = sal_False;
    try
    {
        affectedRows = ( ( sql::PreparedStatement * )cppStatement )->executeUpdate();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
    return affectedRows;
}


void SAL_CALL OPreparedStatement::setString( sal_Int32 parameter, const OUString &x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setString" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ext_std::string stringie( ::rtl::OUStringToOString( x, m_pConnection->getConnectionEncoding() ).getStr() );
        ( ( sql::PreparedStatement * )cppStatement )->setString( parameter, stringie );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::clearParameters", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


Reference< XConnection > SAL_CALL OPreparedStatement::getConnection()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::getConnection" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    return ( Reference< XConnection > )m_pConnection;
}

Reference< XResultSet > SAL_CALL OPreparedStatement::executeQuery( const OUString &sql )
throw( SQLException, RuntimeException )
{
    return OCommonStatement::executeQuery( sql );
}

sal_Int32 SAL_CALL OPreparedStatement::executeUpdate( const OUString &sql )
throw( SQLException, RuntimeException )
{
    return OCommonStatement::executeUpdate( sql );
}

sal_Bool SAL_CALL OPreparedStatement::execute( const OUString &sql )
throw( SQLException, RuntimeException )
{
    return OCommonStatement::execute( sql );
}

Reference< XResultSet > SAL_CALL OPreparedStatement::executeQuery()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::executeQuery" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    Reference< XResultSet > xResultSet;
    try
    {
        sql::ResultSet *res = ( ( sql::PreparedStatement * )cppStatement )->executeQuery();
        xResultSet = new OResultSet( this, res, getOwnConnection()->getConnectionEncoding() );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
    return xResultSet;
}


void SAL_CALL OPreparedStatement::setBoolean( sal_Int32 parameter, sal_Bool x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setBoolean" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setBoolean( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setBoolean", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setByte( sal_Int32 parameter, sal_Int8 x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setByte" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setInt( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setByte", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setDate( sal_Int32 parameter, const Date &aData )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setDate" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    rtl::OStringBuffer aBuff;
    aBuff.append( sal_Int32( aData.Year ) );
    aBuff.append( sal_Char( '-' ) );
    aBuff.append( sal_Int32( aData.Month ) );
    aBuff.append( sal_Char( '-' ) );
    aBuff.append( sal_Int32( aData.Day ) );

    ext_std::string dateStr = aBuff.getStr();

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setDateTime( parameter, dateStr );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setDate", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setTime( sal_Int32 parameter, const Time &aVal )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setTime" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    rtl::OStringBuffer aBuff;
    aBuff.append( sal_Int32( aVal.Hours ) );
    aBuff.append( sal_Char( ':' ) );
    aBuff.append( sal_Int32( aVal.Minutes ) );
    aBuff.append( sal_Char( ':' ) );
    aBuff.append( sal_Int32( aVal.Seconds ) );

    ext_std::string timeStr = aBuff.getStr();

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setDateTime( parameter, timeStr );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setTime", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setTimestamp( sal_Int32 parameter, const DateTime &aVal )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setTimestamp" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    // Format is "Year-Month-Day Hours:Minutes:Seconds"

    rtl::OStringBuffer aBuff;
    aBuff.append( sal_Int32( aVal.Year ) );
    aBuff.append( sal_Char( '-' ) );
    aBuff.append( sal_Int32( aVal.Month ) );
    aBuff.append( sal_Char( '-' ) );
    aBuff.append( sal_Int32( aVal.Day ) );

    aBuff.append( sal_Char( ' ' ) );

    aBuff.append( sal_Int32( aVal.Hours ) );
    aBuff.append( sal_Char( ':' ) );
    aBuff.append( sal_Int32( aVal.Minutes ) );
    aBuff.append( sal_Char( ':' ) );
    aBuff.append( sal_Int32( aVal.Seconds ) );

    ext_std::string dateTimeStr = aBuff.getStr();

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setDateTime( parameter, dateTimeStr );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setTimestamp", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setDouble( sal_Int32 parameter, double x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setDouble" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setDouble( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setDouble", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setFloat( sal_Int32 parameter, float x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setFloat" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setDouble( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setFloat", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setInt( sal_Int32 parameter, sal_Int32 x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setInt" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setInt( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setInt", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setLong( sal_Int32 parameter, sal_Int64 aVal )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setLong" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setInt64( parameter, aVal );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setLong", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setNull( sal_Int32 parameter, sal_Int32 sqlType )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setNull" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setNull( parameter, sqlType );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setNull", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setClob( sal_Int32 parameter, const Reference< XClob > & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setClob" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setClob", *this );
}


void SAL_CALL OPreparedStatement::setBlob( sal_Int32 parameter, const Reference< XBlob > & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setBlob" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setBlob", *this );
}


void SAL_CALL OPreparedStatement::setArray( sal_Int32 parameter, const Reference< XArray > & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setArray" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setArray", *this );
}


void SAL_CALL OPreparedStatement::setRef( sal_Int32 parameter, const Reference< XRef > & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setRef" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setRef", *this );
}

namespace
{
    template < class COMPLEXTYPE >
    bool impl_setObject( const Reference< XParameters > &_rxParam, sal_Int32 _parameterIndex, const Any &_value,
                         void ( SAL_CALL XParameters::*_Setter )( sal_Int32, const COMPLEXTYPE & ), bool _throwIfNotExtractable )
    {
        COMPLEXTYPE aValue;
        if ( _value >>= aValue )
        {
            ( _rxParam.get()->*_Setter )( _parameterIndex, aValue );
            return true;
        }

        if ( _throwIfNotExtractable )
            mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", _rxParam );
        return false;
    }

    template < class INTTYPE >
    void impl_setObject( const Reference< XParameters > &_rxParam, sal_Int32 _parameterIndex, const Any &_value,
                         void ( SAL_CALL XParameters::*_Setter )( sal_Int32, INTTYPE ) )
    {
        sal_Int32 nValue( 0 );
        if ( !( _value >>= nValue ) )
            mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", _rxParam );
        ( _rxParam.get()->*_Setter )( _parameterIndex, ( INTTYPE )nValue );
    }
}

void SAL_CALL OPreparedStatement::setObjectWithInfo( sal_Int32 _parameterIndex, const Any &_value, sal_Int32 _targetSqlType, sal_Int32 /* scale */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setObjectWithInfo" );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );
    checkParameterIndex( _parameterIndex );

    if ( !_value.hasValue() )
    {
        setNull( _parameterIndex, _targetSqlType );
        return;
    }

    switch ( _targetSqlType )
    {
        case DataType::DECIMAL:
        case DataType::NUMERIC:
        {
            double nValue( 0 );
            if ( _value >>= nValue )
            {
                setDouble( _parameterIndex, nValue );
                break;
            }
        }
        // run through

        case DataType::CHAR:
        case DataType::VARCHAR:
        case DataType::LONGVARCHAR:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setString, true );
            break;

        case DataType::BIGINT:
        {
            sal_Int64 nValue = 0;
            if ( !( _value >>= nValue ) )
                mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", *this );
            setLong( _parameterIndex, nValue );
        }
        break;

        case DataType::FLOAT:
        case DataType::REAL:
        {
            float nValue = 0;
            if ( _value >>= nValue )
            {
                setFloat( _parameterIndex, nValue );
                break;
            }
        }
        // run through if we couldn't set a float value

        case DataType::DOUBLE:
        {
            double nValue( 0 );
            if ( !( _value >>= nValue ) )
                mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", *this );
            setDouble( _parameterIndex, nValue );
        }
        break;

        case DataType::DATE:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setDate, true );
            break;

        case DataType::TIME:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setTime, true );
            break;

        case DataType::TIMESTAMP:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setTimestamp, true );
            break;

        case DataType::BINARY:
        case DataType::VARBINARY:
        case DataType::LONGVARBINARY:
        {
            if  (   impl_setObject( this, _parameterIndex, _value, &XParameters::setBytes, false )
                    ||  impl_setObject( this, _parameterIndex, _value, &XParameters::setBlob, false )
                    ||  impl_setObject( this, _parameterIndex, _value, &XParameters::setClob, false )
                )
                break;

            Reference< ::com::sun::star::io::XInputStream > xBinStream;
            if ( _value >>= xBinStream )
            {
                setBinaryStream( _parameterIndex, xBinStream, xBinStream->available() );
                break;
            }

            mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", *this );
        }
        break;

        case DataType::BIT:
        case DataType::BOOLEAN:
        {
            bool bValue( false );
            if ( _value >>= bValue )
            {
                setBoolean( _parameterIndex, bValue );
                break;
            }
            sal_Int32 nValue( 0 );
            if ( _value >>= nValue )
            {
                setBoolean( _parameterIndex, ( nValue != 0 ) );
                break;
            }
            mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", *this );
        }
        break;

        case DataType::TINYINT:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setByte );
            break;

        case DataType::SMALLINT:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setShort );
            break;

        case DataType::INTEGER:
            impl_setObject( this, _parameterIndex, _value, &XParameters::setInt );
            break;

        default:
            mysqlc::throwInvalidArgumentException( "OPreparedStatement::setObjectWithInfo", *this );
            break;
    }
}


void SAL_CALL OPreparedStatement::setObjectNull( sal_Int32 parameter, sal_Int32 /* sqlType */, const OUString & /* typeName */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setObjectNull" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setObjectNull", *this );
}


void SAL_CALL OPreparedStatement::setObject( sal_Int32 parameter, const Any & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setObject" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setObject", *this );
}


void SAL_CALL OPreparedStatement::setShort( sal_Int32 parameter, sal_Int16 x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setShort" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setInt( parameter, x );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setShort", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setBytes( sal_Int32 parameter, const Sequence< sal_Int8 > &x )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setBytes" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    ext_std::string blobby( ( char * )x.getConstArray(), x.getLength() );
    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->setString( parameter, blobby );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setBytes", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::setCharacterStream( sal_Int32 parameter,
        const Reference< XInputStream > & /* x */,
        sal_Int32 /* length */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setCharacterStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setCharacterStream", *this );
}


void SAL_CALL OPreparedStatement::setBinaryStream( sal_Int32 parameter,
        const Reference< XInputStream > & /* x */,
        sal_Int32 /* length */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setBinaryStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );
    checkParameterIndex( parameter );

    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::setBinaryStream", *this );
}


void SAL_CALL OPreparedStatement::clearParameters()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::clearParameters" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OPreparedStatement::rBHelper.bDisposed );

    try
    {
        ( ( sql::PreparedStatement * )cppStatement )->clearParameters();
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::clearParameters", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_pConnection->getConnectionEncoding() );
    }
}


void SAL_CALL OPreparedStatement::clearBatch()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::clearBatch" );
    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::clearBatch", *this );
}


void SAL_CALL OPreparedStatement::addBatch()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::addBatch" );
    mysqlc::throwFeatureNotImplementedException( "OPreparedStatement::addBatch", *this );
}


Sequence< sal_Int32 > SAL_CALL OPreparedStatement::executeBatch()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::executeBatch" );
    Sequence< sal_Int32 > aRet = Sequence< sal_Int32 > ();
    return aRet;
}


void OPreparedStatement::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any &rValue )
throw( Exception )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::setFastPropertyValue_NoBroadcast" );
    switch ( nHandle )
    {
        case PROPERTY_ID_RESULTSETCONCURRENCY:
            break;
        case PROPERTY_ID_RESULTSETTYPE:
            break;
        case PROPERTY_ID_FETCHDIRECTION:
            break;
        case PROPERTY_ID_USEBOOKMARKS:
            break;
        default:
            /* XXX: Recursion ?? */
            OPreparedStatement::setFastPropertyValue_NoBroadcast( nHandle, rValue );
    }
}


void OPreparedStatement::checkParameterIndex( sal_Int32 column )
{
    OSL_TRACE( "mysqlc::OPreparedStatement::checkColumnIndex" );
    if ( column < 1 || column > ( sal_Int32 ) m_paramCount )
    {
        OUString buf( RTL_CONSTASCII_USTRINGPARAM( "Parameter index out of range" ) );
        throw SQLException( buf, *this, OUString(), 1, Any () );
    }
}

