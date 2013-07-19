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

#ifndef __MYSDBC_OPTPAGES_HXX__
#define __MYSDBC_OPTPAGES_HXX__

#include <com/sun/star/awt/XContainerWindowEventHandler.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cppuhelper/compbase2.hxx>
#include <cppuhelper/basemutex.hxx>

#include <boost/utility.hpp>

namespace mysqlc
{

typedef cppu::WeakComponentImplHelper2< com::sun::star::awt::XContainerWindowEventHandler,
                                        com::sun::star::lang::XServiceInfo >
        OptionsPagesHandler_Base;


class OptionsPagesHandler : public cppu::BaseMutex,
                            public OptionsPagesHandler_Base,
                            private boost::noncopyable
{
private:
    com::sun::star::uno::Reference< com::sun::star::uno::XComponentContext > m_xContext;
public:
    OptionsPagesHandler( const com::sun::star::uno::Reference< com::sun::star::uno::XComponentContext > &rxContext );
    ~OptionsPagesHandler();

    // OComponentHelper
    virtual void SAL_CALL disposing();

    // com::sun::star::awt::XContainerWindowEventHandler Methods
    virtual ::sal_Bool SAL_CALL callHandlerMethod( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindow >& xWindow, const ::com::sun::star::uno::Any& EventObject, const ::rtl::OUString& MethodName ) throw (::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedMethodNames(  ) throw (::com::sun::star::uno::RuntimeException);

    // com::sun::star::lang::XServiceInfo Methods
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) throw (::com::sun::star::uno::RuntimeException);

    static rtl::OUString GetImplementationName_static();
    static com::sun::star::uno::Sequence< rtl::OUString > GetSupportedServiceNames_static();
    static com::sun::star::uno::Reference< com::sun::star::uno::XInterface > CreateInstance( const com::sun::star::uno::Reference< com::sun::star::uno::XComponentContext > &rxContext );

};

}

#endif
