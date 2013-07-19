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

#include "mysqlc_optpages.hxx"
#include "mysqlc_general.hxx"

using com::sun::star::awt::XWindow;
using com::sun::star::lang::WrappedTargetException;
using com::sun::star::uno::Any;
using com::sun::star::uno::Reference;
using com::sun::star::uno::RuntimeException;
using com::sun::star::uno::Sequence;
using com::sun::star::uno::XComponentContext;
using com::sun::star::uno::XInterface;
using rtl::OUString;

using namespace mysqlc;

OptionsPagesHandler::OptionsPagesHandler( const Reference< XComponentContext > &rxContext ) :
    OptionsPagesHandler_Base( m_aMutex ),
    m_xContext( rxContext )
{
}

OptionsPagesHandler::~OptionsPagesHandler()
{}

// OComponentHelper
void SAL_CALL OptionsPagesHandler::disposing()
{}

// com::sun::star::awt::XContainerWindowEventHandler Methods
sal_Bool SAL_CALL OptionsPagesHandler::callHandlerMethod(
    const Reference< XWindow >& xWindow,
    const Any& EventObject,
    const OUString& MethodName )
throw (WrappedTargetException, RuntimeException)
{
    return sal_False;
}

Sequence< OUString > SAL_CALL OptionsPagesHandler::getSupportedMethodNames(  )
throw ( RuntimeException)
{
    return Sequence< OUString >();
}

OUString SAL_CALL OptionsPagesHandler::getImplementationName(  )
throw (RuntimeException)
{
    return GetImplementationName_static();
}

sal_Bool SAL_CALL OptionsPagesHandler::supportsService( const OUString& ServiceName )
throw (RuntimeException)
{
    return GetSupportedServiceNames_static()[0].equals( ServiceName );
}

Sequence< OUString > SAL_CALL OptionsPagesHandler::getSupportedServiceNames(  )
throw (RuntimeException)
{
    return GetSupportedServiceNames_static();
}

OUString OptionsPagesHandler::GetImplementationName_static()
{
    return C2U( "org.aoo-my-sdbc.comp.OptionsPageHandler" );
}

Sequence< OUString > OptionsPagesHandler::GetSupportedServiceNames_static()
{
    Sequence< OUString > aRet(1);
    aRet[0] = C2U( "org.aoo-my-sdbc.OptionsPageHandler" );

    return aRet;
}

Reference< XInterface > OptionsPagesHandler::CreateInstance(
    const Reference< XComponentContext > &rxContext )
{
    return (*(new OptionsPagesHandler( rxContext )));
}



