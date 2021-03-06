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

#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/ResultSetType.hpp>
#include <com/sun/star/sdbc/ResultSetConcurrency.hpp>
#include <com/sun/star/sdbc/TransactionIsolation.hpp>
#include <com/sun/star/sdbc/KeyRule.hpp>
#include <com/sun/star/sdbc/Deferrability.hpp>
#include <com/sun/star/sdbc/IndexType.hpp>
#include <com/sun/star/sdbc/BestRowScope.hpp>
#include <com/sun/star/sdbc/ColumnType.hpp>
#include <com/sun/star/lang/XInitialization.hpp>

#include <cppconn/resultset.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "mysqlc_databasemetadata.hxx"
#include "mysqlc_general.hxx"
#include "mysqlc_statement.hxx"
#include "mysqlc_driver.hxx"
#include "mysqlc_preparedstatement.hxx"


using namespace mysqlc;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;

using ::rtl::OUString;
using mysqlc::getStringFromAny;
using ::rtl::OUStringToOString;


static std::string wild( "%" );


Reference< XResultSet > lcl_createMetaDataResultSet( const Reference< XComponentContext> &rxContext )
{
    Reference< XResultSet > xRet;
    try
    {
        xRet.set(
            rxContext->getServiceManager()->createInstanceWithContext(
                C2U( "org.openoffice.comp.helper.DatabaseMetaDataResultSet" ), rxContext ), UNO_QUERY );
    }
    catch ( ... ) {}

    return xRet;
}


void lcl_setRows_throw( const Reference< XResultSet > &_xResultSet, sal_Int32 _nType, const std::vector< std::vector< Any > > &_rRows )
{
    Reference< XInitialization> xIni( _xResultSet, UNO_QUERY );
    Sequence< Any > aArgs( 2 );
    aArgs[0] <<= _nType;

    Sequence< Sequence< Any > > aRows( _rRows.size() );

    std::vector< std::vector< Any > >::const_iterator aIter = _rRows.begin();
    Sequence< Any > *pRowsIter = aRows.getArray();
    Sequence< Any > *pRowsEnd  = pRowsIter + aRows.getLength();
    for ( ; pRowsIter != pRowsEnd; ++pRowsIter, ++aIter )
    {
        if ( !aIter->empty() )
        {
            Sequence<Any> aSeq( &( *aIter->begin() ), aIter->size() );
            ( *pRowsIter ) = aSeq;
        }
    }
    aArgs[1] <<= aRows;
    xIni->initialize( aArgs );
}


ODatabaseMetaData::ODatabaseMetaData(
    const Reference< XComponentContext > &rContext,
    OConnection &_rCon )
    : m_xContext( rContext )
    , m_rConnection( _rCon )
    , m_bUseCatalog( sal_True )
    , meta( _rCon.getConnectionSettings()->cppConnection->getMetaData() )
    , identifier_quote_string_set( false )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::ODatabaseMetaData" );
    if ( !m_rConnection.isCatalogUsed() )
    {
        osl_incrementInterlockedCount( &m_refCount );
        m_bUseCatalog = !( usesLocalFiles() || usesLocalFilePerTable() );
        osl_decrementInterlockedCount( &m_refCount );
    }
}


ODatabaseMetaData::~ODatabaseMetaData()
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::~ODatabaseMetaData" );
}


OUString ODatabaseMetaData::impl_getStringMetaData( const sal_Char *_methodName, const std::string & ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    OUString stringMetaData;
    try
    {
        stringMetaData = mysqlc::convert( ( meta->*_Method )(), m_rConnection.getConnectionEncoding() );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return stringMetaData;
}


OUString ODatabaseMetaData::impl_getStringMetaData( const sal_Char *_methodName, std::string ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    OUString stringMetaData;
    try
    {
        stringMetaData = mysqlc::convert( ( meta->*_Method )(), m_rConnection.getConnectionEncoding() );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return stringMetaData;
}


OUString ODatabaseMetaData::impl_getStringMetaData( const sal_Char *_methodName, const sql::SQLString & ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    OUString stringMetaData;
    try
    {
        stringMetaData = mysqlc::convert( ( meta->*_Method )(), m_rConnection.getConnectionEncoding() );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return stringMetaData;
}


OUString ODatabaseMetaData::impl_getStringMetaData( const sal_Char *_methodName, sql::SQLString ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    OUString stringMetaData;
    try
    {
        stringMetaData = mysqlc::convert( ( meta->*_Method )(), m_rConnection.getConnectionEncoding() );
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return stringMetaData;
}


sal_Int32 ODatabaseMetaData::impl_getInt32MetaData( const sal_Char *_methodName, unsigned int ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    sal_Int32 int32MetaData( 0 );
    try
    {
        int32MetaData = ( meta->*_Method )();
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return int32MetaData;
}


sal_Bool ODatabaseMetaData::impl_getBoolMetaData( const sal_Char *_methodName, bool ( sql::DatabaseMetaData::*_Method )() )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    sal_Bool boolMetaData( 0 );
    try
    {
        boolMetaData = ( meta->*_Method )() ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return boolMetaData;
}


sal_Bool ODatabaseMetaData::impl_getBoolMetaData( const sal_Char *_methodName, bool ( sql::DatabaseMetaData::*_Method )( int ), sal_Int32 _arg )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::%s", _methodName );
    sal_Bool boolMetaData( 0 );
    try
    {
        boolMetaData = ( meta->*_Method )( _arg ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( _methodName, *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( _methodName, *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return boolMetaData;
}


sal_Bool ODatabaseMetaData::impl_getRSTypeMetaData( const sal_Char *_methodName, bool ( sql::DatabaseMetaData::*_Method )( int ), sal_Int32 _resultSetType )
{
    int resultSetType( sql::ResultSet::TYPE_FORWARD_ONLY );
    switch ( _resultSetType )
    {
        case ResultSetType::SCROLL_INSENSITIVE:    resultSetType = sql::ResultSet::TYPE_SCROLL_INSENSITIVE;    break;
        case ResultSetType::SCROLL_SENSITIVE:    resultSetType = sql::ResultSet::TYPE_SCROLL_SENSITIVE;        break;
    }

    return impl_getBoolMetaData( _methodName, _Method, resultSetType );
}


OUString SAL_CALL ODatabaseMetaData::getCatalogSeparator()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getCatalogSeparator", &sql::DatabaseMetaData::getCatalogSeparator );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxBinaryLiteralLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxBinaryLiteralLength", &sql::DatabaseMetaData::getMaxBinaryLiteralLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxRowSize()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxRowSize", &sql::DatabaseMetaData::getMaxRowSize );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxCatalogNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxCatalogNameLength", &sql::DatabaseMetaData::getMaxCatalogNameLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxCharLiteralLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxCharLiteralLength", &sql::DatabaseMetaData::getMaxCharLiteralLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnNameLength", &sql::DatabaseMetaData::getMaxColumnNameLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnsInIndex()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnsInIndex", &sql::DatabaseMetaData::getMaxColumnsInIndex );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxCursorNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxCursorNameLength", &sql::DatabaseMetaData::getMaxCursorNameLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxConnections()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxConnections", &sql::DatabaseMetaData::getMaxConnections );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnsInTable()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnsInTable", &sql::DatabaseMetaData::getMaxColumnsInTable );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxStatementLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxStatementLength", &sql::DatabaseMetaData::getMaxStatementLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxTableNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxTableNameLength", &sql::DatabaseMetaData::getMaxTableNameLength );
}

sal_Int32 SAL_CALL ODatabaseMetaData::getMaxTablesInSelect()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxTablesInSelect", &sql::DatabaseMetaData::getMaxTablesInSelect );
}


sal_Bool SAL_CALL ODatabaseMetaData::doesMaxRowSizeIncludeBlobs()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "doesMaxRowSizeIncludeBlobs", &sql::DatabaseMetaData::doesMaxRowSizeIncludeBlobs );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesLowerCaseQuotedIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesLowerCaseQuotedIdentifiers", &sql::DatabaseMetaData::storesLowerCaseQuotedIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesLowerCaseIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesLowerCaseIdentifiers", &sql::DatabaseMetaData::storesLowerCaseIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesMixedCaseQuotedIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesMixedCaseQuotedIdentifiers", &sql::DatabaseMetaData::storesMixedCaseQuotedIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesMixedCaseIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesMixedCaseIdentifiers", &sql::DatabaseMetaData::storesMixedCaseIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesUpperCaseQuotedIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesUpperCaseQuotedIdentifiers", &sql::DatabaseMetaData::storesUpperCaseQuotedIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::storesUpperCaseIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "storesUpperCaseIdentifiers", &sql::DatabaseMetaData::storesUpperCaseIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsAlterTableWithAddColumn()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsAlterTableWithAddColumn", &sql::DatabaseMetaData::supportsAlterTableWithAddColumn );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsAlterTableWithDropColumn()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsAlterTableWithDropColumn", &sql::DatabaseMetaData::supportsAlterTableWithDropColumn );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxIndexLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxIndexLength", &sql::DatabaseMetaData::getMaxIndexLength );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsNonNullableColumns()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsNonNullableColumns", &sql::DatabaseMetaData::supportsNonNullableColumns );
}


OUString SAL_CALL ODatabaseMetaData::getCatalogTerm()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getCatalogTerm", &sql::DatabaseMetaData::getCatalogTerm );
}


OUString SAL_CALL ODatabaseMetaData::getIdentifierQuoteString()
throw( SQLException, RuntimeException )
{
    if ( identifier_quote_string_set == false )
    {
        identifier_quote_string = impl_getStringMetaData( "getIdentifierQuoteString", &sql::DatabaseMetaData::getIdentifierQuoteString );
        identifier_quote_string_set = true;
    }
    return identifier_quote_string;
}


OUString SAL_CALL ODatabaseMetaData::getExtraNameCharacters()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getExtraNameCharacters", &sql::DatabaseMetaData::getExtraNameCharacters );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsDifferentTableCorrelationNames()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsDifferentTableCorrelationNames", &sql::DatabaseMetaData::supportsDifferentTableCorrelationNames );
}


sal_Bool SAL_CALL ODatabaseMetaData::isCatalogAtStart()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "isCatalogAtStart", &sql::DatabaseMetaData::isCatalogAtStart );
}


sal_Bool SAL_CALL ODatabaseMetaData::dataDefinitionIgnoredInTransactions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "dataDefinitionIgnoredInTransactions", &sql::DatabaseMetaData::dataDefinitionIgnoredInTransactions );
}


sal_Bool SAL_CALL ODatabaseMetaData::dataDefinitionCausesTransactionCommit()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "dataDefinitionCausesTransactionCommit", &sql::DatabaseMetaData::dataDefinitionCausesTransactionCommit );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsDataManipulationTransactionsOnly()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsDataManipulationTransactionsOnly", &sql::DatabaseMetaData::supportsDataManipulationTransactionsOnly );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsDataDefinitionAndDataManipulationTransactions", &sql::DatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsPositionedDelete()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsPositionedDelete", &sql::DatabaseMetaData::supportsPositionedDelete );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsPositionedUpdate()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsPositionedUpdate", &sql::DatabaseMetaData::supportsPositionedUpdate );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOpenStatementsAcrossRollback()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOpenStatementsAcrossRollback", &sql::DatabaseMetaData::supportsOpenStatementsAcrossRollback );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOpenStatementsAcrossCommit()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOpenStatementsAcrossCommit", &sql::DatabaseMetaData::supportsOpenStatementsAcrossCommit );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOpenCursorsAcrossCommit()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOpenCursorsAcrossCommit", &sql::DatabaseMetaData::supportsOpenCursorsAcrossCommit );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOpenCursorsAcrossRollback()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOpenCursorsAcrossRollback", &sql::DatabaseMetaData::supportsOpenCursorsAcrossRollback );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsTransactionIsolationLevel( sal_Int32 level )
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsTransactionIsolationLevel", &sql::DatabaseMetaData::supportsTransactionIsolationLevel, level );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSchemasInDataManipulation()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSchemasInDataManipulation", &sql::DatabaseMetaData::supportsSchemasInDataManipulation );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsANSI92FullSQL()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsANSI92FullSQL", &sql::DatabaseMetaData::supportsANSI92FullSQL );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsANSI92EntryLevelSQL()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsANSI92EntryLevelSQL", &sql::DatabaseMetaData::supportsANSI92EntryLevelSQL );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsIntegrityEnhancementFacility()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsIntegrityEnhancementFacility", &sql::DatabaseMetaData::supportsIntegrityEnhancementFacility );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSchemasInIndexDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSchemasInIndexDefinitions", &sql::DatabaseMetaData::supportsSchemasInIndexDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSchemasInTableDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSchemasInTableDefinitions", &sql::DatabaseMetaData::supportsSchemasInTableDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCatalogsInTableDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCatalogsInTableDefinitions", &sql::DatabaseMetaData::supportsCatalogsInTableDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCatalogsInIndexDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCatalogsInIndexDefinitions", &sql::DatabaseMetaData::supportsCatalogsInIndexDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCatalogsInDataManipulation()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCatalogsInDataManipulation", &sql::DatabaseMetaData::supportsCatalogsInDataManipulation );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOuterJoins()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOuterJoins", &sql::DatabaseMetaData::supportsOuterJoins );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxStatements()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxStatements", &sql::DatabaseMetaData::getMaxStatements );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxProcedureNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxProcedureNameLength", &sql::DatabaseMetaData::getMaxProcedureNameLength );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxSchemaNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxSchemaNameLength", &sql::DatabaseMetaData::getMaxSchemaNameLength );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsTransactions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsTransactions", &sql::DatabaseMetaData::supportsTransactions );
}


sal_Bool SAL_CALL ODatabaseMetaData::allProceduresAreCallable()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "allProceduresAreCallable", &sql::DatabaseMetaData::allProceduresAreCallable );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsStoredProcedures()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsStoredProcedures", &sql::DatabaseMetaData::supportsStoredProcedures );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSelectForUpdate()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSelectForUpdate", &sql::DatabaseMetaData::supportsSelectForUpdate );
}


sal_Bool SAL_CALL ODatabaseMetaData::allTablesAreSelectable()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "allTablesAreSelectable", &sql::DatabaseMetaData::allTablesAreSelectable );
}


sal_Bool SAL_CALL ODatabaseMetaData::isReadOnly()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "isReadOnly", &sql::DatabaseMetaData::isReadOnly );
}


sal_Bool SAL_CALL ODatabaseMetaData::usesLocalFiles()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "usesLocalFiles", &sql::DatabaseMetaData::usesLocalFiles );
}


sal_Bool SAL_CALL ODatabaseMetaData::usesLocalFilePerTable()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "usesLocalFilePerTable", &sql::DatabaseMetaData::usesLocalFilePerTable );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsTypeConversion()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsTypeConversion", &sql::DatabaseMetaData::supportsTypeConversion );
}


sal_Bool SAL_CALL ODatabaseMetaData::nullPlusNonNullIsNull()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "nullPlusNonNullIsNull", &sql::DatabaseMetaData::nullPlusNonNullIsNull );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsColumnAliasing()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsColumnAliasing", &sql::DatabaseMetaData::supportsColumnAliasing );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsTableCorrelationNames()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsTableCorrelationNames", &sql::DatabaseMetaData::supportsTableCorrelationNames );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsConvert( sal_Int32 /* fromType */, sal_Int32 /* toType */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::supportsConvert" );
    try
    {
        /* ToDo -> use supportsConvert( fromType, toType) */
        return meta->supportsConvert() ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::supportsConvert", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::supportsConvert", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return sal_False;
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsExpressionsInOrderBy()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsExpressionsInOrderBy", &sql::DatabaseMetaData::supportsExpressionsInOrderBy );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsGroupBy()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsGroupBy", &sql::DatabaseMetaData::supportsGroupBy );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsGroupByBeyondSelect()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsGroupByBeyondSelect", &sql::DatabaseMetaData::supportsGroupByBeyondSelect );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsGroupByUnrelated()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsGroupByUnrelated", &sql::DatabaseMetaData::supportsGroupByUnrelated );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsMultipleTransactions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsMultipleTransactions", &sql::DatabaseMetaData::supportsMultipleTransactions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsMultipleResultSets()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsMultipleResultSets", &sql::DatabaseMetaData::supportsMultipleResultSets );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsLikeEscapeClause()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsLikeEscapeClause", &sql::DatabaseMetaData::supportsLikeEscapeClause );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsOrderByUnrelated()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsOrderByUnrelated", &sql::DatabaseMetaData::supportsOrderByUnrelated );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsUnion()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsUnion", &sql::DatabaseMetaData::supportsUnion );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsUnionAll()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsUnionAll", &sql::DatabaseMetaData::supportsUnionAll );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsMixedCaseIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsMixedCaseIdentifiers", &sql::DatabaseMetaData::supportsMixedCaseIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsMixedCaseQuotedIdentifiers()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsMixedCaseQuotedIdentifiers", &sql::DatabaseMetaData::supportsMixedCaseQuotedIdentifiers );
}


sal_Bool SAL_CALL ODatabaseMetaData::nullsAreSortedAtEnd()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "nullsAreSortedAtEnd", &sql::DatabaseMetaData::nullsAreSortedAtEnd );
}


sal_Bool SAL_CALL ODatabaseMetaData::nullsAreSortedAtStart()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "nullsAreSortedAtStart", &sql::DatabaseMetaData::nullsAreSortedAtStart );
}


sal_Bool SAL_CALL ODatabaseMetaData::nullsAreSortedHigh()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "nullsAreSortedHigh", &sql::DatabaseMetaData::nullsAreSortedHigh );
}


sal_Bool SAL_CALL ODatabaseMetaData::nullsAreSortedLow()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "nullsAreSortedLow", &sql::DatabaseMetaData::nullsAreSortedLow );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSchemasInProcedureCalls()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSchemasInProcedureCalls", &sql::DatabaseMetaData::supportsSchemasInProcedureCalls );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSchemasInPrivilegeDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSchemasInPrivilegeDefinitions", &sql::DatabaseMetaData::supportsSchemasInPrivilegeDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCatalogsInProcedureCalls()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCatalogsInProcedureCalls", &sql::DatabaseMetaData::supportsCatalogsInProcedureCalls );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCatalogsInPrivilegeDefinitions()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCatalogsInPrivilegeDefinitions", &sql::DatabaseMetaData::supportsCatalogsInPrivilegeDefinitions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCorrelatedSubqueries()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCorrelatedSubqueries", &sql::DatabaseMetaData::supportsCorrelatedSubqueries );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSubqueriesInComparisons()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSubqueriesInComparisons", &sql::DatabaseMetaData::supportsSubqueriesInComparisons );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSubqueriesInExists()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSubqueriesInExists", &sql::DatabaseMetaData::supportsSubqueriesInExists );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSubqueriesInIns()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSubqueriesInIns", &sql::DatabaseMetaData::supportsSubqueriesInIns );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsSubqueriesInQuantifieds()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsSubqueriesInQuantifieds", &sql::DatabaseMetaData::supportsSubqueriesInQuantifieds );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsANSI92IntermediateSQL()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsANSI92IntermediateSQL", &sql::DatabaseMetaData::supportsANSI92IntermediateSQL );
}


OUString SAL_CALL ODatabaseMetaData::getURL()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getURL" );
    return m_rConnection.getConnectionSettings()->connectionURL;
}


OUString SAL_CALL ODatabaseMetaData::getUserName()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getUserName", &sql::DatabaseMetaData::getUserName );
}


OUString SAL_CALL ODatabaseMetaData::getDriverName()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getDriverName" );
    OUString aValue( RTL_CONSTASCII_USTRINGPARAM( "MySQL Connector/OO.org" ) );
    return aValue;
}


OUString SAL_CALL ODatabaseMetaData::getDriverVersion()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getDriverVersion" );
    static const OUString sVersion( RTL_CONSTASCII_USTRINGPARAM( "0.9.2" ) );
    return sVersion;
}


OUString SAL_CALL ODatabaseMetaData::getDatabaseProductVersion()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getDatabaseProductVersion", &sql::DatabaseMetaData::getDatabaseProductVersion );
}


OUString SAL_CALL ODatabaseMetaData::getDatabaseProductName()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getDatabaseProductName", &sql::DatabaseMetaData::getDatabaseProductName );
}


OUString SAL_CALL ODatabaseMetaData::getProcedureTerm()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getProcedureTerm", &sql::DatabaseMetaData::getProcedureTerm );
}


OUString SAL_CALL ODatabaseMetaData::getSchemaTerm()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getSchemaTerm", &sql::DatabaseMetaData::getSchemaTerm );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getDriverMajorVersion()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getDriverMajorVersion" );
    return MYSQLC_VERSION_MAJOR;
}


sal_Int32 SAL_CALL ODatabaseMetaData::getDefaultTransactionIsolation()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getDefaultTransactionIsolation" );
    try
    {
        switch ( meta->getDefaultTransactionIsolation() )
        {
            case sql::TRANSACTION_SERIALIZABLE:        return TransactionIsolation::SERIALIZABLE;
            case sql::TRANSACTION_REPEATABLE_READ:    return TransactionIsolation::REPEATABLE_READ;
            case sql::TRANSACTION_READ_COMMITTED:    return TransactionIsolation::READ_COMMITTED;
            case sql::TRANSACTION_READ_UNCOMMITTED:    return TransactionIsolation::READ_UNCOMMITTED;
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getDriverMajorVersion", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getDriverMajorVersion", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return TransactionIsolation::NONE;
}


sal_Int32 SAL_CALL ODatabaseMetaData::getDriverMinorVersion()
throw( RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getDriverMinorVersion" );
    return MYSQLC_VERSION_MINOR;
}


OUString SAL_CALL ODatabaseMetaData::getSQLKeywords()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getSQLKeywords", &sql::DatabaseMetaData::getSQLKeywords );
}


OUString SAL_CALL ODatabaseMetaData::getSearchStringEscape()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getSearchStringEscape", &sql::DatabaseMetaData::getSearchStringEscape );
}


OUString SAL_CALL ODatabaseMetaData::getStringFunctions()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getStringFunctions", &sql::DatabaseMetaData::getStringFunctions );
}


OUString SAL_CALL ODatabaseMetaData::getTimeDateFunctions()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getTimeDateFunctions", &sql::DatabaseMetaData::getTimeDateFunctions );
}


OUString SAL_CALL ODatabaseMetaData::getSystemFunctions()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getSystemFunctions", &sql::DatabaseMetaData::getSystemFunctions );
}


OUString SAL_CALL ODatabaseMetaData::getNumericFunctions()
throw( SQLException, RuntimeException )
{
    return impl_getStringMetaData( "getNumericFunctions", &sql::DatabaseMetaData::getNumericFunctions );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsExtendedSQLGrammar()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsExtendedSQLGrammar", &sql::DatabaseMetaData::supportsExtendedSQLGrammar );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsCoreSQLGrammar()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsCoreSQLGrammar", &sql::DatabaseMetaData::supportsCoreSQLGrammar );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsMinimumSQLGrammar()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsMinimumSQLGrammar", &sql::DatabaseMetaData::supportsMinimumSQLGrammar );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsFullOuterJoins()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsFullOuterJoins", &sql::DatabaseMetaData::supportsFullOuterJoins );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsLimitedOuterJoins()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsLimitedOuterJoins", &sql::DatabaseMetaData::supportsLimitedOuterJoins );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnsInGroupBy()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnsInGroupBy", &sql::DatabaseMetaData::getMaxColumnsInGroupBy );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnsInOrderBy()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnsInOrderBy", &sql::DatabaseMetaData::getMaxColumnsInOrderBy );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxColumnsInSelect()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxColumnsInSelect", &sql::DatabaseMetaData::getMaxColumnsInSelect );
}


sal_Int32 SAL_CALL ODatabaseMetaData::getMaxUserNameLength()
throw( SQLException, RuntimeException )
{
    return impl_getInt32MetaData( "getMaxUserNameLength", &sql::DatabaseMetaData::getMaxUserNameLength );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsResultSetType( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "supportsResultSetType", &sql::DatabaseMetaData::supportsResultSetType, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsResultSetConcurrency( sal_Int32 setType, sal_Int32 concurrency )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::supportsResultSetConcurrency" );
    /* TODO: Check this out */
    try
    {
        return meta->supportsResultSetConcurrency( setType, concurrency == com::sun::star::sdbc::TransactionIsolation::READ_COMMITTED ?
                sql::TRANSACTION_READ_COMMITTED :
                ( concurrency == com::sun::star::sdbc::TransactionIsolation::SERIALIZABLE ?
                  sql::TRANSACTION_SERIALIZABLE : sql::TRANSACTION_SERIALIZABLE ) ) ? sal_True : sal_False;
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::supportsResultSetConcurrency", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::supportsResultSetConcurrency", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    return sal_False;
}


sal_Bool SAL_CALL ODatabaseMetaData::ownUpdatesAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "ownUpdatesAreVisible", &sql::DatabaseMetaData::ownUpdatesAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::ownDeletesAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "ownDeletesAreVisible", &sql::DatabaseMetaData::ownDeletesAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::ownInsertsAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "ownInsertsAreVisible", &sql::DatabaseMetaData::ownInsertsAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::othersUpdatesAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "othersUpdatesAreVisible", &sql::DatabaseMetaData::othersUpdatesAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::othersDeletesAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "othersDeletesAreVisible", &sql::DatabaseMetaData::othersDeletesAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::othersInsertsAreVisible( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "othersInsertsAreVisible", &sql::DatabaseMetaData::othersInsertsAreVisible, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::updatesAreDetected( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "updatesAreDetected", &sql::DatabaseMetaData::updatesAreDetected, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::deletesAreDetected( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "deletesAreDetected", &sql::DatabaseMetaData::deletesAreDetected, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::insertsAreDetected( sal_Int32 setType )
throw( SQLException, RuntimeException )
{
    return impl_getRSTypeMetaData( "insertsAreDetected", &sql::DatabaseMetaData::insertsAreDetected, setType );
}


sal_Bool SAL_CALL ODatabaseMetaData::supportsBatchUpdates()
throw( SQLException, RuntimeException )
{
    return impl_getBoolMetaData( "supportsBatchUpdates", &sql::DatabaseMetaData::supportsBatchUpdates );
}


Reference< XConnection > SAL_CALL ODatabaseMetaData::getConnection()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getConnection" );
    return ( Reference< XConnection > )&m_rConnection;
}


/*
  Here follow all methods which return(a resultset
  the first methods is an example implementation how to use this resultset
  of course you could implement it on your and you should do this because
  the general way is more memory expensive
*/

Reference< XResultSet > SAL_CALL ODatabaseMetaData::getTableTypes()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getTableTypes" );
    const char *table_types[] = {"TABLE", "VIEW"};
    sal_Int32 requiredVersion[] = {0, 50000};

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;
    rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();

    for ( sal_uInt32 i = 0; i < 2; i++ )
    {
        if ( m_rConnection.getMysqlVersion() >= requiredVersion[i] )
        {
            std::vector< Any > aRow( 1 );
            aRow.push_back( makeAny( mysqlc::convert( table_types[i], encoding ) ) );
            rRows.push_back( aRow );
        }
    }
    lcl_setRows_throw( xResultSet, 5 , rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getTypeInfo()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getTypeInfo" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );

    std::vector< std::vector< Any > > rRows;

    rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
    unsigned int i = 0;
    while ( mysqlc_types[i].typeName )
    {
        std::vector< Any > aRow( 1 );

        aRow.push_back( makeAny( mysqlc::convert( mysqlc_types[i].typeName, encoding ) ) );
        aRow.push_back( makeAny( mysqlc_types[i].dataType ) );
        aRow.push_back( makeAny( mysqlc_types[i].precision ) );
        aRow.push_back( makeAny( mysqlc::convert( mysqlc_types[i].literalPrefix, encoding ) ) );
        aRow.push_back( makeAny( mysqlc::convert( mysqlc_types[i].literalSuffix, encoding ) ) );
        aRow.push_back( makeAny( mysqlc::convert( mysqlc_types[i].createParams, encoding ) ) );
        aRow.push_back( makeAny( mysqlc_types[i].nullable ) );
        aRow.push_back( makeAny( mysqlc_types[i].caseSensitive ) );
        aRow.push_back( makeAny( mysqlc_types[i].searchable ) );
        aRow.push_back( makeAny( mysqlc_types[i].isUnsigned ) );
        aRow.push_back( makeAny( mysqlc_types[i].fixedPrecScale ) );
        aRow.push_back( makeAny( mysqlc_types[i].autoIncrement ) );
        aRow.push_back( makeAny( mysqlc::convert( mysqlc_types[i].localTypeName, encoding ) ) );
        aRow.push_back( makeAny( mysqlc_types[i].minScale ) );
        aRow.push_back( makeAny( mysqlc_types[i].maxScale ) );
        aRow.push_back( makeAny( sal_Int32( 0 ) ) );
        aRow.push_back( makeAny( sal_Int32( 0 ) ) );
        aRow.push_back( makeAny( sal_Int32( 10 ) ) );

        rRows.push_back( aRow );
        i++;
    }

    lcl_setRows_throw( xResultSet, 14, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getCatalogs()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getCatalogs" );

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getCatalogs() );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getCatalogs", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getCatalogs", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 0, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getSchemas()
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getSchemas" );

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    try
    {
        OUString sCatalog = m_rConnection.getCatalog();
        // MySQL_Connection::getSchema() queries "SELECT DATABASE()"
        sql::SQLString sOwnSchema = meta->getConnection()->getSchema();
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getSchemas() );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();

        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            bool bOwnSchema = false;
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                sql::SQLString columnStringValue = rset->getString( i );
                if ( i == 1 )   // TABLE_SCHEM
                {
                    bOwnSchema = columnStringValue.compare( sOwnSchema ) == 0;
                }
                aRow.push_back( makeAny( mysqlc::convert( columnStringValue, encoding ) ) );
            }
            if ( bOwnSchema )
            {
                rRows.push_back( aRow );
            }
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getSchemas", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getSchemas", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 1, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getColumnPrivileges(
    const Any &catalog,
    const OUString &schema,
    const OUString &table,
    const OUString &columnNamePattern )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getColumnPrivileges" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() ),
            cNamePattern( OUStringToOString( columnNamePattern, m_rConnection.getConnectionEncoding() ).getStr() );
    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getColumnPrivileges( cat, sch, tab, cNamePattern.compare( "" ) ? cNamePattern : wild ) );

        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getColumnPrivileges", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getColumnPrivileges", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 2, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getColumns(
    const Any &catalog,
    const OUString &schemaPattern,
    const OUString &tableNamePattern,
    const OUString &columnNamePattern )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getColumns" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;
    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sPattern( OUStringToOString( schemaPattern, m_rConnection.getConnectionEncoding() ).getStr() ),
            tNamePattern( OUStringToOString( tableNamePattern, m_rConnection.getConnectionEncoding() ).getStr() ),
            cNamePattern( OUStringToOString( columnNamePattern, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        std::auto_ptr< sql::ResultSet> rset( meta->getColumns( cat,
                                             sPattern.compare( "" ) ? sPattern : wild,
                                             tNamePattern.compare( "" ) ? tNamePattern : wild,
                                             cNamePattern.compare( "" ) ? cNamePattern : wild ) );
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                if ( i == 5 ) // ColumnType
                {
                    sal_Int32 sdbc_type = mysqlc::mysqlToOOOType( atoi( rset->getString( i ).c_str() ) );
                    aRow.push_back( makeAny( sdbc_type ) );
                }
                else
                {
                    aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
                }
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getColumns", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getColumns", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }
    lcl_setRows_throw( xResultSet, 3, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getTables(
    const Any &catalog,
    const OUString &schemaPattern,
    const OUString &tableNamePattern,
    const Sequence< OUString > &types )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getTables" );

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;
    try
    {
        std::string cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
                sPattern( OUStringToOString( schemaPattern, m_rConnection.getConnectionEncoding() ).getStr() ),
                tNamePattern( OUStringToOString( tableNamePattern, m_rConnection.getConnectionEncoding() ).getStr() );

        std::list<sql::SQLString> tabTypes;
        sal_Int32 nLength = types.getLength();
        for ( const OUString *pStart = types.getConstArray(), *p = pStart, *pEnd = pStart + nLength; p != pEnd; ++p )
        {
            tabTypes.push_back( OUStringToOString( *p, m_rConnection.getConnectionEncoding() ).getStr() );
        }

        sql::SQLString sOwnSchema = meta->getConnection()->getSchema();
        if ( sPattern.compare( wild ) == 0 || sPattern.compare( "" ) == 0 )
            sPattern = sOwnSchema;

        std::auto_ptr< sql::ResultSet> rset( meta->getTables( cat,
                                             sPattern,
                                             tNamePattern.compare( "" ) ? tNamePattern : wild,
                                             tabTypes ) );

        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );

            for ( sal_uInt32 i = 1; ( i <= columns ); ++i )
            {
                sql::SQLString columnStringValue = rset->getString( i );
                aRow.push_back( makeAny( mysqlc::convert( columnStringValue, encoding ) ) );
            }

            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getTables", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getTables", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 4, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getProcedureColumns(
    const Any & /* catalog */,
    const OUString & /* schemaPattern */,
    const OUString & /* procedureNamePattern */,
    const OUString & /* columnNamePattern */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getProcedureColumns" );
    // Currently there is no information available
    return NULL;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getProcedures(
    const Any &catalog,
    const OUString &schemaPattern,
    const OUString &procedureNamePattern )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getProcedures" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sPattern( OUStringToOString( schemaPattern, m_rConnection.getConnectionEncoding() ).getStr() ),
            pNamePattern( OUStringToOString( procedureNamePattern, m_rConnection.getConnectionEncoding() ).getStr() );


    try
    {
        std::auto_ptr< sql::ResultSet> rset( meta->getProcedures( cat,
                                             sPattern.compare( "" ) ? sPattern : wild,
                                             pNamePattern.compare( "" ) ? pNamePattern : wild ) );

        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getProcedures", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getProcedures", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 7, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getVersionColumns(
    const Any & /* catalog */,
    const OUString & /* schema */,
    const OUString & /* table */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getVersionColumns" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;
    lcl_setRows_throw( xResultSet, 16, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getExportedKeys(
    const Any  &catalog ,
    const OUString  &schema ,
    const OUString  &table )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getExportedKeys" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;
    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getExportedKeys( cat, sch, tab ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getExportedKeys", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getExportedKeys", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 8, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getImportedKeys(
    const Any &catalog,
    const OUString &schema,
    const OUString &table )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getImportedKeys" );

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getImportedKeys( cat, sch, tab ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getImportedKeys", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getImportedKeys", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 9, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getPrimaryKeys(
    const Any &catalog,
    const OUString &schema,
    const OUString &table )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getPrimaryKeys" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getPrimaryKeys( cat, sch, tab ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getPrimaryKeys", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getPrimaryKeys", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 10, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getIndexInfo(
    const Any &catalog,
    const OUString &schema,
    const OUString &table,
    sal_Bool unique,
    sal_Bool approximate )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getIndexInfo" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getIndexInfo( cat, sch, tab, unique, approximate ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getIndexInfo", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getIndexInfo", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 11, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getBestRowIdentifier(
    const Any &catalog,
    const OUString &schema,
    const OUString &table,
    sal_Int32 scope,
    sal_Bool nullable )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getBestRowIdentifier" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sch( OUStringToOString( schema, m_rConnection.getConnectionEncoding() ).getStr() ),
            tab( OUStringToOString( table, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getBestRowIdentifier( cat, sch, tab, scope, nullable ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getBestRowIdentifier", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getBestRowIdentifier", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 15, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getTablePrivileges(
    const Any &catalog,
    const OUString &schemaPattern,
    const OUString &tableNamePattern )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getTablePrivileges" );
    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );
    std::vector< std::vector< Any > > rRows;

    std::string    cat( catalog.hasValue() ? OUStringToOString( getStringFromAny( catalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            sPattern( OUStringToOString( schemaPattern, m_rConnection.getConnectionEncoding() ).getStr() ),
            tPattern( OUStringToOString( tableNamePattern, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        static bool fakeTablePrivileges = false;
        if ( fakeTablePrivileges )
        {
            static const sal_Char *allPrivileges[] =
            {
                "ALTER", "DELETE", "DROP", "INDEX", "INSERT", "LOCK TABLES", "SELECT", "UPDATE"
            };
            Any userName; userName <<= getUserName();
            for ( size_t i = 0; i < sizeof( allPrivileges ) / sizeof( allPrivileges[0] ); ++i )
            {
                std::vector< Any > aRow;
                aRow.push_back( makeAny( sal_Int32( i ) ) );
                aRow.push_back( catalog );                                                        // TABLE_CAT
                aRow.push_back( makeAny( schemaPattern ) );                                       // TABLE_SCHEM
                aRow.push_back( makeAny( tableNamePattern ) );                                    // TABLE_NAME
                aRow.push_back( Any() );                                                          // GRANTOR
                aRow.push_back( userName );                                                       // GRANTEE
                aRow.push_back( makeAny( ::rtl::OUString::createFromAscii( allPrivileges[i] ) ) ); // PRIVILEGE
                aRow.push_back( Any() );                                                          // IS_GRANTABLE

                rRows.push_back( aRow );
            }
        }
        else
        {
            rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
            std::auto_ptr< sql::ResultSet> rset( meta->getTablePrivileges( cat, sPattern.compare( "" ) ? sPattern : wild, tPattern.compare( "" ) ? tPattern : wild ) );
            sql::ResultSetMetaData *rs_meta = rset->getMetaData();
            sal_uInt32 columns = rs_meta->getColumnCount();
            while ( rset->next() )
            {
                std::vector< Any > aRow( 1 );
                for ( sal_uInt32 i = 1; i <= columns; i++ )
                {
                    aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
                }
                rRows.push_back( aRow );
            }
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getTablePrivileges", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getTablePrivileges", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 12, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getCrossReference(
    const Any &primaryCatalog,
    const OUString &primarySchema,
    const OUString &primaryTable,
    const Any &foreignCatalog,
    const OUString &foreignSchema,
    const OUString &foreignTable )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getCrossReference" );

    Reference< XResultSet > xResultSet( lcl_createMetaDataResultSet( m_xContext ) );

    std::vector< std::vector< Any > > rRows;

    std::string    primaryCat( primaryCatalog.hasValue() ? OUStringToOString( getStringFromAny( primaryCatalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            foreignCat( foreignCatalog.hasValue() ? OUStringToOString( getStringFromAny( foreignCatalog ), m_rConnection.getConnectionEncoding() ).getStr() : "" ),
            pSchema( OUStringToOString( primarySchema, m_rConnection.getConnectionEncoding() ).getStr() ),
            pTable( OUStringToOString( primaryTable, m_rConnection.getConnectionEncoding() ).getStr() ),
            fSchema( OUStringToOString( foreignSchema, m_rConnection.getConnectionEncoding() ).getStr() ),
            fTable( OUStringToOString( foreignTable, m_rConnection.getConnectionEncoding() ).getStr() );

    try
    {
        rtl_TextEncoding encoding = m_rConnection.getConnectionEncoding();
        std::auto_ptr< sql::ResultSet> rset( meta->getCrossReference( primaryCat, pSchema, pTable, foreignCat, fSchema, fTable ) );
        sql::ResultSetMetaData *rs_meta = rset->getMetaData();
        sal_uInt32 columns = rs_meta->getColumnCount();
        while ( rset->next() )
        {
            std::vector< Any > aRow( 1 );
            for ( sal_uInt32 i = 1; i <= columns; i++ )
            {
                aRow.push_back( makeAny( mysqlc::convert( rset->getString( i ), encoding ) ) );
            }
            rRows.push_back( aRow );
        }
    }
    catch ( sql::MethodNotImplementedException )
    {
        mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getCrossReference", *this );
    }
    catch ( sql::InvalidArgumentException )
    {
        mysqlc::throwInvalidArgumentException( "ODatabaseMetaData::getCrossReference", *this );
    }
    catch ( const sql::SQLException &e )
    {
        mysqlc::translateAndThrow( e, *this, m_rConnection.getConnectionEncoding() );
    }

    lcl_setRows_throw( xResultSet, 13, rRows );
    return xResultSet;
}


Reference< XResultSet > SAL_CALL ODatabaseMetaData::getUDTs(
    const Any & /* catalog */,
    const OUString & /* schemaPattern */,
    const OUString & /* typeNamePattern */,
    const Sequence< sal_Int32 > & /* types */ )
throw( SQLException, RuntimeException )
{
    OSL_TRACE( "mysqlc::ODatabaseMetaData::getUDTs" );
    mysqlc::throwFeatureNotImplementedException( "ODatabaseMetaData::getUDTs", *this );
    return NULL;
}
