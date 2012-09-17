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

#include "mysqlc_resultsetmetadata.hxx"
#include "mysqlc_general.hxx"
#include "cppconn/exception.h"

#include <rtl/ustrbuf.hxx>

using namespace mysqlc;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::sdbc;
using ::rtl::OUString;

// -------------------------------------------------------------------------
OResultSetMetaData::~OResultSetMetaData()
{
}


sal_Int32 SAL_CALL OResultSetMetaData::getColumnDisplaySize( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnDisplaySize" );

    try
    {
        meta->getColumnDisplaySize( column );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getColumnDisplaySize", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Int32 SAL_CALL OResultSetMetaData::getColumnType( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnType" );
    checkColumnIndex( column );

    try
    {
        return mysqlc::mysqlToOOOType( meta->getColumnType( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}

/*
  XXX: This method doesn't throw exceptions at all.
  Should it declare that it throws ?? What if throw() is removed?
  Does it change the API, the open-close principle?
*/
sal_Int32 SAL_CALL OResultSetMetaData::getColumnCount()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnCount" );
    try
    {
        return meta->getColumnCount();
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isCaseSensitive( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isCaseSensitive" );
    checkColumnIndex( column );

    try
    {
        return meta->isCaseSensitive( column );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getSchemaName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getSchemaName" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getSchemaName( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getColumnName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnName" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getColumnName( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getTableName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getTableName" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getTableName( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getCatalogName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getCatalogName" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getCatalogName( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getColumnTypeName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnTypeName" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getColumnTypeName( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getColumnLabel( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnLabel" );
    checkColumnIndex( column );

    try
    {
        return convert( meta->getColumnLabel( column ) );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return OUString(); // fool compiler
}


OUString SAL_CALL OResultSetMetaData::getColumnServiceName( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getColumnServiceName" );
    checkColumnIndex( column );

    OUString aRet = OUString();
    return aRet;
}


sal_Bool SAL_CALL OResultSetMetaData::isCurrency( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isCurrency" );
    checkColumnIndex( column );

    try
    {
        return meta->isCurrency( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isAutoIncrement( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isAutoIncrement" );
    checkColumnIndex( column );

    try
    {
        return meta->isAutoIncrement( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isSigned( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isSigned" );
    checkColumnIndex( column );

    try
    {
        return meta->isSigned( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Int32 SAL_CALL OResultSetMetaData::getPrecision( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getPrecision" );
    checkColumnIndex( column );

    try
    {
        return meta->getPrecision( column );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getPrecision", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Int32 SAL_CALL OResultSetMetaData::getScale( sal_Int32 column )
throw( ::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::getScale" );
    checkColumnIndex( column );
    try
    {
        return meta->getScale( column );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getScale", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return 0; // fool compiler
}


sal_Int32 SAL_CALL OResultSetMetaData::isNullable( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isNullable" );
    checkColumnIndex( column );

    try
    {
        return meta->isNullable( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isSearchable( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isSearchable" );
    checkColumnIndex( column );

    try
    {
        return meta->isSearchable( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isReadOnly( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isReadOnly" );
    checkColumnIndex( column );

    try
    {
        return meta->isReadOnly( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isDefinitelyWritable( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isDefinitelyWritable" );
    checkColumnIndex( column );

    try
    {
        return meta->isDefinitelyWritable( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


sal_Bool SAL_CALL OResultSetMetaData::isWritable( sal_Int32 column )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::isWritable" );
    checkColumnIndex( column );

    try
    {
        return meta->isWritable( column ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "OResultSetMetaData::getMetaData", *this );
    }
    catch ( sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_encoding );
    }
    return sal_False; // fool compiler
}


void OResultSetMetaData::checkColumnIndex( sal_Int32 columnIndex )
throw ( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::OResultSetMetaData::checkColumnIndex" );
    if ( columnIndex < 1 || columnIndex > ( sal_Int32 ) meta->getColumnCount() )
    {

        ::rtl::OUStringBuffer buf;
        buf.appendAscii( "Column index out of range (expected 1 to " );
        buf.append( sal_Int32( meta->getColumnCount() ) );
        buf.appendAscii( ", got " );
        buf.append( sal_Int32( columnIndex ) );
        buf.append( sal_Unicode( '.' ) );
        throw SQLException( buf.makeStringAndClear(), *this, OUString(), 1, Any() );
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

