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

#include "mysqlc_propertyids.hxx"
#include "mysqlc_general.hxx"
#include "mysqlc_resultset.hxx"
#include "mysqlc_resultsetmetadata.hxx"

#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/sdbcx/CompareBookmark.hpp>
#include <com/sun/star/sdbc/ResultSetConcurrency.hpp>
#include <com/sun/star/sdbc/ResultSetType.hpp>
#include <com/sun/star/sdbc/FetchDirection.hpp>
#include <cppuhelper/typeprovider.hxx>
#include <com/sun/star/lang/DisposedException.hpp>

using namespace mysqlc;
using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;
using namespace com::sun::star::container;
using namespace com::sun::star::io;
using namespace com::sun::star::util;
using ::osl::MutexGuard;
using ::rtl::OUString;

#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>

#include <stdio.h>


//    IMPLEMENT_SERVICE_INFO(OResultSet,"com.sun.star.sdbcx.OResultSet","com.sun.star.sdbc.ResultSet");
OUString SAL_CALL OResultSet::getImplementationName()
throw ( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getImplementationName" );
    return ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.sdbcx.mysqlc.ResultSet" ) );
}


Sequence< OUString > SAL_CALL OResultSet::getSupportedServiceNames()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getSupportedServiceNames" );
    Sequence< OUString > aSupported( 2 );
    aSupported[0] = OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.sdbc.ResultSet" ) );
    aSupported[1] = OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.sdbcx.ResultSet" ) );
    return ( aSupported );
}


sal_Bool SAL_CALL OResultSet::supportsService( const OUString &_rServiceName )
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::supportsService" );
    Sequence< OUString > aSupported( getSupportedServiceNames() );
    const OUString *pSupported = aSupported.getConstArray();
    const OUString *pEnd = pSupported + aSupported.getLength();
    for ( ; pSupported != pEnd && !pSupported->equals( _rServiceName ); ++pSupported ) {}

    return ( pSupported != pEnd );
}


OResultSet::OResultSet( OCommonStatement *pStmt, sql::ResultSet *result, rtl_TextEncoding _encoding )
    : OResultSet_BASE( m_aMutex )
    , OPropertySetHelper( OResultSet_BASE::rBHelper )
    , m_aStatement( ( OWeakObject * )pStmt )
    , m_xMetaData( NULL )
    , m_result( result )
    , fieldCount( 0 )
    , m_encoding( _encoding )
{
    OSL_TRACE( "mysqlc::OResultSet::OResultSet" );
    try
    {
        sql::ResultSetMetaData *rs_meta = m_result->getMetaData();
        fieldCount = rs_meta->getColumnCount();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
}


OResultSet::~OResultSet()
{
    OSL_TRACE( "mysqlc::OResultSet::~OResultSet" );
}


void OResultSet::disposing()
{
    OSL_TRACE( "mysqlc::OResultSet::disposing" );
    OPropertySetHelper::disposing();

    MutexGuard aGuard( m_aMutex );

    m_aStatement = NULL;
    m_xMetaData  = NULL;
}


Any SAL_CALL OResultSet::queryInterface( const Type &rType )
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::queryInterface" );
    Any aRet = OPropertySetHelper::queryInterface( rType );
    if ( !aRet.hasValue() )
    {
        aRet = OResultSet_BASE::queryInterface( rType );
    }
    return aRet;
}


Sequence< Type > SAL_CALL OResultSet::getTypes()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getTypes" );
    OTypeCollection aTypes(    ::getCppuType( ( const  Reference< XMultiPropertySet > * ) NULL ),
                               ::getCppuType( ( const Reference< XFastPropertySet > * ) NULL ),
                               ::getCppuType( ( const Reference< XPropertySet > * ) NULL ) );

    return concatSequences( aTypes.getTypes(), OResultSet_BASE::getTypes() );
}


sal_Int32 SAL_CALL OResultSet::findColumn( const OUString &columnName )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::findColumn" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        // find the first column with the name columnName
        sql::ResultSetMetaData *meta = m_result->getMetaData();
        for ( sal_uInt32 i = 1; i <= fieldCount; i++ )
        {
            if ( columnName.equalsIgnoreAsciiCaseAscii( meta->getColumnName( i ).c_str() ) )
            {
                /* SDBC knows them indexed from 1 */
                return i;
            }
        }
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0;
}


Reference< XInputStream > SAL_CALL OResultSet::getBinaryStream( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getBinaryStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getBinaryStream", *this );
    return NULL;
}


Reference< XInputStream > SAL_CALL OResultSet::getCharacterStream( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getCharacterStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getCharacterStream", *this );
    return NULL;
}


sal_Bool SAL_CALL OResultSet::getBoolean( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getBoolean" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getBoolean( column ) ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False;
#if 0
    OUString str = getString( column );
    switch ( str[0] )
    {
        case '1':
        case 't':
        case 'T':
        case 'y':
        case 'Y':
            return sal_True;
    }
    return sal_False;
#endif
}


sal_Int8 SAL_CALL OResultSet::getByte( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getByte" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getInt( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


Sequence< sal_Int8 > SAL_CALL OResultSet::getBytes( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getBytes" );

    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );


    sql::SQLString val = m_result->getString( column );
    if ( !val.length() )
    {
        return Sequence< sal_Int8>();
    }
    else
    {
        return Sequence< sal_Int8 > ( ( sal_Int8 * )val.c_str(), val.length() );
    }
}


Date SAL_CALL OResultSet::getDate( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getDate" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    Date d;
    try
    {
        OUString dateString = getString( column );
        OUString token;
        sal_Int32 nIndex = 0, i = 0;

        do
        {
            token = dateString.getToken ( 0, '-', nIndex );
            switch ( i )
            {
                case 0:
                    d.Year =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                    break;
                case 1:
                    d.Month =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                    break;
                case 2:
                    d.Day =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                    break;
                default:;
            }
            i++;
        }
        while ( nIndex >= 0 );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return d;
}


double SAL_CALL OResultSet::getDouble( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getDouble" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getDouble( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0.0; // fool compiler
}


float SAL_CALL OResultSet::getFloat( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getFloat" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getDouble( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0.0; // fool compiler
}


sal_Int32 SAL_CALL OResultSet::getInt( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getInt" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getInt( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Int32 SAL_CALL OResultSet::getRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->getRow();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Int64 SAL_CALL OResultSet::getLong( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getLong" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );
    try
    {
        return m_result->getInt64( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


Reference< XResultSetMetaData > SAL_CALL OResultSet::getMetaData()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getMetaData" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    try
    {
        if ( !m_xMetaData.is() )
        {
            m_xMetaData = new OResultSetMetaData( m_result->getMetaData(), m_encoding );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSet::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return m_xMetaData;
}


Reference< XArray > SAL_CALL OResultSet::getArray( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getArray" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getArray", *this );
    return NULL;
}


Reference< XClob > SAL_CALL OResultSet::getClob( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getClob" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getClob", *this );
    return NULL;
}


Reference< XBlob > SAL_CALL OResultSet::getBlob( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getBlob" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getBlob", *this );
    return NULL;
}


Reference< XRef > SAL_CALL OResultSet::getRef( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getRef" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getRef", *this );
    return NULL;
}


Any SAL_CALL OResultSet::getObject( sal_Int32 column, const Reference< XNameAccess > & /* typeMap */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getObject" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );

    Any aRet = Any();

    mysqlc::throwFeatureNotImplementedException( "OResultSet::getObject", *this );
    return aRet;
}


sal_Int16 SAL_CALL OResultSet::getShort( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getShort" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return ( sal_Int16 ) m_result->getInt( column );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


OUString SAL_CALL OResultSet::getString( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getString" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    checkColumnIndex( column );

    try
    {
        sql::SQLString val = m_result->getString( column );
        if ( !m_result->wasNull() )
        {
            return OUString( val.c_str(), val.length(), m_encoding );
        }
        else
        {
            return OUString();
        }
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


Time SAL_CALL OResultSet::getTime( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getTime" );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );

    checkColumnIndex( column );
    Time t;
    OUString timeString = getString( column );
    OUString token;
    sal_Int32 nIndex, i = 0;

    nIndex = timeString.indexOf( ' ' ) + 1;

    do
    {
        token = timeString.getToken ( 0, ':', nIndex );
        switch ( i )
        {
            case 0:
                t.Hours =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                break;
            case 1:
                t.Minutes =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                break;
            case 2:
                t.Seconds =  static_cast<sal_uInt16>( token.toInt32( 10 ) );
                break;
        }
        i++;
    }
    while ( nIndex >= 0 );

    return t;
}


DateTime SAL_CALL OResultSet::getTimestamp( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getTimestamp" );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );

    checkColumnIndex( column );
    DateTime dt;
    Date d = getDate( column );
    Time t = getTime( column );

    dt.Year = d.Year;
    dt.Month = d.Month;
    dt.Day = d.Day;
    dt.Hours = t.Hours;
    dt.Minutes = t.Minutes;
    dt.Seconds = t.Seconds;
    return dt;
}


sal_Bool SAL_CALL OResultSet::isBeforeFirst()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::isBeforeFirst" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->isBeforeFirst() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::isAfterLast()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::isAfterLast" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->isAfterLast() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::isFirst()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::isFirst" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->isFirst();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::isLast()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::isLast" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->isLast() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


void SAL_CALL OResultSet::beforeFirst()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::beforeFirst" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        m_result->beforeFirst();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
}


void SAL_CALL OResultSet::afterLast()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::afterLast" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        m_result->afterLast();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
}


void SAL_CALL OResultSet::close() throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::close" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        m_result->close();
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }

    dispose();
}


sal_Bool SAL_CALL OResultSet::first() throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::first" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->first() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::last()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::last" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->last() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::absolute( sal_Int32 row )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::absolute" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->absolute( row ) ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::relative( sal_Int32 row )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::relative" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->relative( row ) ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::previous()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::previous" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->previous() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


Reference< XInterface > SAL_CALL OResultSet::getStatement()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getStatement" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    return m_aStatement.get();
}


sal_Bool SAL_CALL OResultSet::rowDeleted()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::rowDeleted" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    return sal_False;
}


sal_Bool SAL_CALL OResultSet::rowInserted()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::rowInserted" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    return sal_False;
}


sal_Bool SAL_CALL OResultSet::rowUpdated()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::rowUpdated" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    return sal_False;
}


sal_Bool SAL_CALL OResultSet::next()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::next" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->next() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


sal_Bool SAL_CALL OResultSet::wasNull()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::wasNull" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    try
    {
        return m_result->wasNull() ? sal_True : sal_False;
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; //fool
}


void SAL_CALL OResultSet::cancel()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::cancel" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
}


void SAL_CALL OResultSet::clearWarnings()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::clearWarnings" );
}


Any SAL_CALL OResultSet::getWarnings()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getWarnings" );
    Any aRet = Any();
    return aRet;
}


void SAL_CALL OResultSet::insertRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::insertRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    // you only have to implement this if you want to insert new rows
    mysqlc::throwFeatureNotImplementedException( "OResultSet::insertRow", *this );
}


void SAL_CALL OResultSet::updateRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    // only when you allow updates
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateRow", *this );
}


void SAL_CALL OResultSet::deleteRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::deleteRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::deleteRow", *this );
}


void SAL_CALL OResultSet::cancelRowUpdates()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::cancelRowUpdates" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::cancelRowUpdates", *this );
}


void SAL_CALL OResultSet::moveToInsertRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::moveToInsertRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    // only when you allow insert's
    mysqlc::throwFeatureNotImplementedException( "OResultSet::moveToInsertRow", *this );
}


void SAL_CALL OResultSet::moveToCurrentRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::moveToCurrentRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
}


void SAL_CALL OResultSet::updateNull( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateNull" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateNull", *this );
}


void SAL_CALL OResultSet::updateBoolean( sal_Int32 column, sal_Bool /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateBoolean" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateBoolean", *this );
}


void SAL_CALL OResultSet::updateByte( sal_Int32 column, sal_Int8 /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateByte" );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateByte", *this );
}


void SAL_CALL OResultSet::updateShort( sal_Int32 column, sal_Int16 /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateShort" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateShort", *this );
}


void SAL_CALL OResultSet::updateInt( sal_Int32 column, sal_Int32 /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateInt" );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    MutexGuard aGuard( m_aMutex );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateInt", *this );
}


void SAL_CALL OResultSet::updateLong( sal_Int32 column, sal_Int64 /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateLong" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateLong", *this );
}


void SAL_CALL OResultSet::updateFloat( sal_Int32 column, float /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateFloat" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateFloat", *this );
}


void SAL_CALL OResultSet::updateDouble( sal_Int32 column, double /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateDouble" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateDouble", *this );
}


void SAL_CALL OResultSet::updateString( sal_Int32 column, const OUString & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateString" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateString", *this );
}


void SAL_CALL OResultSet::updateBytes( sal_Int32 column, const Sequence< sal_Int8 > & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateBytes" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateBytes", *this );
}


void SAL_CALL OResultSet::updateDate( sal_Int32 column, const Date & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateDate" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateDate", *this );
}


void SAL_CALL OResultSet::updateTime( sal_Int32 column, const Time & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateTime" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateTime", *this );
}


void SAL_CALL OResultSet::updateTimestamp( sal_Int32 column, const DateTime & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateTimestamp" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateTimestamp", *this );
}


void SAL_CALL OResultSet::updateBinaryStream( sal_Int32 column, const Reference< XInputStream > & /* x */,
        sal_Int32 /* length */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateBinaryStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateBinaryStream", *this );
}


void SAL_CALL OResultSet::updateCharacterStream( sal_Int32 column, const Reference< XInputStream > & /* x */,
        sal_Int32 /* length */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateCharacterStream" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateCharacterStream", *this );
}


void SAL_CALL OResultSet::refreshRow()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::refreshRow" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::refreshRow", *this );
}


void SAL_CALL OResultSet::updateObject( sal_Int32 column, const Any & /* x */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateObject" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateObject", *this );
}


void SAL_CALL OResultSet::updateNumericObject( sal_Int32 column, const Any & /* x */, sal_Int32 /* scale */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::updateNumericObject" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    checkColumnIndex( column );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::updateNumericObject", *this );
}


// XRowLocate
Any SAL_CALL OResultSet::getBookmark()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getBookmark" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    Any aRet = Any();

    // if you don't want to support bookmark you must remove the XRowLocate interface
    mysqlc::throwFeatureNotImplementedException( "OResultSet::getBookmark", *this );

    return aRet;
}


sal_Bool SAL_CALL OResultSet::moveToBookmark( const Any & /* bookmark */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::moveToBookmark" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    return sal_False;
}


sal_Bool SAL_CALL OResultSet::moveRelativeToBookmark( const Any & /* bookmark */, sal_Int32 /* rows */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::moveRelativeToBookmark" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::moveRelativeToBookmark", *this );
    return sal_False;
}


sal_Int32 SAL_CALL OResultSet::compareBookmarks( const Any & /* n1 */, const Any & /* n2 */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::compareBookmarks" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );

    mysqlc::throwFeatureNotImplementedException( "OResultSet::compareBookmarks", *this );

    return CompareBookmark::NOT_EQUAL;
}


sal_Bool SAL_CALL OResultSet::hasOrderedBookmarks()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::hasOrderedBookmarks" );
    return sal_False;
}


sal_Int32 SAL_CALL OResultSet::hashBookmark( const Any & /* bookmark */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::hashBookmark" );
    mysqlc::throwFeatureNotImplementedException( "OResultSet::hashBookmark", *this );
    return 0;
}


// XDeleteRows
Sequence< sal_Int32 > SAL_CALL OResultSet::deleteRows( const Sequence< Any > & /* rows */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::deleteRows" );
    MutexGuard aGuard( m_aMutex );
    checkDisposed( OResultSet_BASE::rBHelper.bDisposed );
    Sequence< sal_Int32 > aRet = Sequence< sal_Int32 >();

    mysqlc::throwFeatureNotImplementedException( "OResultSet::deleteRows", *this );
    return aRet;
}


IPropertyArrayHelper *OResultSet::createArrayHelper() const
{
    OSL_TRACE( "mysqlc::OResultSet::createArrayHelper" );
    Sequence< Property > aProps( 5 );
    Property *pProperties = aProps.getArray();
    sal_Int32 nPos = 0;
    DECL_PROP0( FETCHDIRECTION,            sal_Int32 );
    DECL_PROP0( FETCHSIZE,                sal_Int32 );
    DECL_BOOL_PROP1IMPL( ISBOOKMARKABLE ) PropertyAttribute::READONLY );
    DECL_PROP1IMPL( RESULTSETCONCURRENCY, sal_Int32 ) PropertyAttribute::READONLY );
    DECL_PROP1IMPL( RESULTSETTYPE,        sal_Int32 ) PropertyAttribute::READONLY );

    return new OPropertyArrayHelper( aProps );
}


       IPropertyArrayHelper &OResultSet::getInfoHelper()
{
    OSL_TRACE( "mysqlc::OResultSet::getInfoHelper" );
    return ( *const_cast<OResultSet *>( this )->getArrayHelper() );
}


sal_Bool OResultSet::convertFastPropertyValue( Any & /* rConvertedValue */,
        Any & /* rOldValue */,
        sal_Int32 nHandle,
        const Any & /* rValue */ )
throw ( ::com::sun::star::lang::IllegalArgumentException )
{
    OSL_TRACE( "mysqlc::OResultSet::convertFastPropertyValue" );
    switch ( nHandle )
    {
        case PROPERTY_ID_ISBOOKMARKABLE:
        case PROPERTY_ID_CURSORNAME:
        case PROPERTY_ID_RESULTSETCONCURRENCY:
        case PROPERTY_ID_RESULTSETTYPE:
            throw ::com::sun::star::lang::IllegalArgumentException();
        case PROPERTY_ID_FETCHDIRECTION:
        case PROPERTY_ID_FETCHSIZE:
        default:
            ;
    }
    return sal_False;
}


void OResultSet::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any & /* rValue */ )
throw ( Exception )
{
    OSL_TRACE( "mysqlc::OResultSet::setFastPropertyValue_NoBroadcast" );
    switch ( nHandle )
    {
        case PROPERTY_ID_ISBOOKMARKABLE:
        case PROPERTY_ID_CURSORNAME:
        case PROPERTY_ID_RESULTSETCONCURRENCY:
        case PROPERTY_ID_RESULTSETTYPE:
            throw Exception();
        case PROPERTY_ID_FETCHDIRECTION:
            break;
        case PROPERTY_ID_FETCHSIZE:
            break;
        default:
            ;
    }
}


void OResultSet::getFastPropertyValue( Any &_rValue, sal_Int32 nHandle ) const
{
    OSL_TRACE( "mysqlc::OResultSet::getFastPropertyValue" );
    switch ( nHandle )
    {
        case PROPERTY_ID_ISBOOKMARKABLE:
            _rValue <<= sal_False;
            break;
        case PROPERTY_ID_CURSORNAME:
            break;
        case PROPERTY_ID_RESULTSETCONCURRENCY:
            _rValue <<= ResultSetConcurrency::READ_ONLY;
            break;
        case PROPERTY_ID_RESULTSETTYPE:
            _rValue <<= ResultSetType::SCROLL_INSENSITIVE;
            break;
        case PROPERTY_ID_FETCHDIRECTION:
            _rValue <<= FetchDirection::FORWARD;
            break;
        case PROPERTY_ID_FETCHSIZE:
            _rValue <<= sal_Int32( 50 );
            break;
            ;
        default:
            ;
    }
}


void SAL_CALL OResultSet::acquire()
throw()
{
    OSL_TRACE( "mysqlc::OResultSet::acquire" );
    OResultSet_BASE::acquire();
}


void SAL_CALL OResultSet::release()
throw()
{
    OSL_TRACE( "mysqlc::OResultSet::release" );
    OResultSet_BASE::release();
}


::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL OResultSet::getPropertySetInfo() throw( ::com::sun::star::uno::RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::getPropertySetInfo" );
    return ( ::cppu::OPropertySetHelper::createPropertySetInfo( getInfoHelper() ) );
}


void OResultSet::checkColumnIndex( sal_Int32 index )
throw ( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSet::checkColumnIndex" );
    if ( ( index < 1 || index > ( int ) fieldCount ) )
    {
        /* static object for efficiency or thread safety is a problem ? */
        OUString buf( RTL_CONSTASCII_USTRINGPARAM( "index out of range" ) );
        throw SQLException( buf, *this, OUString(), 1, Any() );
    }
}

