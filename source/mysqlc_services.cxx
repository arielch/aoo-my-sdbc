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

#include <cppuhelper/factory.hxx>
#include <cppuhelper/implementationentry.hxx>


namespace mysqlc
{
    void *SAL_CALL CreateDriverSingleton( void *pServiceManager );

    static struct cppu::ImplementationEntry ImplEntries[] =
    {
        {
            MysqlCDriver::CreateInstance,
            MysqlCDriver::getImplementationName_static,
            MysqlCDriver::getSupportedServiceNames_static,
            0 /* cppu::createOneInstanceComponentFactory */,
            0,
            0
        },
        { 0, 0, 0, 0, 0, 0 }
    };
}


extern "C"
{
    SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment(
        const sal_Char **ppEnvTypeName, uno_Environment **ppEnv )
    {
        *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
    }

    SAL_DLLPUBLIC_EXPORT void *SAL_CALL component_getFactory(
        const sal_Char *pImplName, void *pServiceManager, void *pRegistryKey )
    {
        if ( mysqlc::MysqlCDriver::getImplementationName_static().compareToAscii( pImplName ) == 0 )
            return mysqlc::CreateDriverSingleton( pServiceManager );
        else
            return ::cppu::component_getFactoryHelper( pImplName,
                                                       pServiceManager,
                                                       pRegistryKey ,
                                                       mysqlc::ImplEntries );
    }
}
