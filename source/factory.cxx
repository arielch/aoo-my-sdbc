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
#include "mysqlc_general.hxx"

#include <cppuhelper/factory.hxx>
#include <cppuhelper/compbase2.hxx>
#include <cppuhelper/implementationentry.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>

using rtl::OUString;
using osl::MutexGuard;

using cppu::WeakComponentImplHelper2;

using com::sun::star::beans::XPropertySet;
using com::sun::star::lang::XComponent;
using com::sun::star::lang::XServiceInfo;
using com::sun::star::lang::XSingleComponentFactory;
using com::sun::star::uno::Any;
using com::sun::star::uno::Exception;
using com::sun::star::uno::Reference;
using com::sun::star::uno::RuntimeException;
using com::sun::star::uno::Sequence;
using com::sun::star::uno::UNO_QUERY;
using com::sun::star::uno::XComponentContext;
using com::sun::star::uno::XInterface;

namespace mysqlc
{
    struct MutexHolder { osl::Mutex m_mutex; };

    class OOneInstanceComponentFactory :
        public MutexHolder,
        public WeakComponentImplHelper2< XSingleComponentFactory, XServiceInfo >
    {
        public:
            OOneInstanceComponentFactory( const Reference< XComponentContext > &defaultContext ) :
                WeakComponentImplHelper2< XSingleComponentFactory, XServiceInfo >( this->m_mutex ),
                m_defaultContext( defaultContext )
            {
            }

            // XSingleComponentFactory
            virtual Reference< XInterface > SAL_CALL createInstanceWithContext(
                Reference< XComponentContext > const &xContext )
            throw ( Exception, RuntimeException );
            virtual Reference< XInterface > SAL_CALL createInstanceWithArgumentsAndContext(
                Sequence< Any > const &rArguments,
                Reference< XComponentContext > const &xContext )
            throw ( Exception, RuntimeException );

            // XServiceInfo
            OUString SAL_CALL getImplementationName()
            throw( ::com::sun::star::uno::RuntimeException )
            {
                return MysqlCDriver::getImplementationName_static();
            }
            sal_Bool SAL_CALL supportsService( const OUString &ServiceName )
            throw( ::com::sun::star::uno::RuntimeException )
            {
                Sequence< OUString > aSupp = MysqlCDriver::getSupportedServiceNames_static();
                for ( int i = 0 ; i < aSupp.getLength() ; i ++ )
                    if ( aSupp[i] == ServiceName )
                        return sal_True;
                return sal_False;
            }
            Sequence< OUString > SAL_CALL getSupportedServiceNames( void )
            throw( ::com::sun::star::uno::RuntimeException )
            {
                return MysqlCDriver::getSupportedServiceNames_static();
            }

            // XComponent
            virtual void SAL_CALL disposing();

        private:
            Reference< XInterface >       m_theInstance;
            Reference< XComponentContext > m_defaultContext;
    };

    Reference< XInterface > OOneInstanceComponentFactory::createInstanceWithArgumentsAndContext(
        Sequence< Any > const &rArguments, const Reference< XComponentContext > &ctx )
    throw( RuntimeException, Exception )
    {
        return createInstanceWithContext( ctx );
    }

    Reference< XInterface > OOneInstanceComponentFactory::createInstanceWithContext(
        const Reference< XComponentContext > &ctx )
    throw( RuntimeException, Exception )
    {
        if ( ! m_theInstance.is() )
        {
            // work around the problem in sdbc
            Reference< XComponentContext > useCtx = ctx;
            if ( ! useCtx.is() )
                useCtx = m_defaultContext;
            Reference< XInterface > theInstance = MysqlCDriver::CreateInstance( useCtx );
            MutexGuard guard( osl::Mutex::getGlobalMutex() );
            if ( ! m_theInstance.is () )
            {
                m_theInstance = theInstance;
            }
        }
        return m_theInstance;
    }

    void OOneInstanceComponentFactory::disposing()
    {
        Reference< XComponent > rComp;
        {
            MutexGuard guard( osl::Mutex::getGlobalMutex() );
            rComp = Reference< XComponent >( m_theInstance, UNO_QUERY );
            m_theInstance.clear();
        }
        if ( rComp.is() )
            rComp->dispose();
    }

    void *SAL_CALL CreateDriverSingleton( void *pServiceManager )
    SAL_THROW( () )
    {
        // need to extract the defaultcontext, because the way, sdbc
        // bypasses the servicemanager, does not allow to use the
        // XSingleComponentFactory interface ...
        void *pRet = 0;
        Reference< XSingleComponentFactory > xFactory;
        try
        {
            Reference< XInterface > xSmgr( ( XInterface * ) pServiceManager );

            Reference< XComponentContext > defaultContext;
            Reference< XPropertySet > propSet( xSmgr, UNO_QUERY );
            if ( propSet.is() )
            {
                try
                {
                    propSet->getPropertyValue( C2U( "DefaultContext" ) ) >>= defaultContext;
                }
                catch ( com::sun::star::uno::Exception &e )
                {
                    // if there is no default context, ignore it
                }
            }
            xFactory = new mysqlc::OOneInstanceComponentFactory( defaultContext );
            if ( xFactory.is() )
            {
                xFactory->acquire();
                pRet = xFactory.get();
            }
        }
        catch ( ... ) {}
        return pRet;
    }

}


