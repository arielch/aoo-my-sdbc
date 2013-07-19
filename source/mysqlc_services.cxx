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
#include "mysqlc_optpages.hxx"

#include <cppuhelper/factory.hxx>
#include <cppuhelper/implementationentry.hxx>


namespace mysqlc
{
    static struct cppu::ImplementationEntry ImplEntries[] =
    {
        {
            OptionsPagesHandler::CreateInstance,
            OptionsPagesHandler::GetImplementationName_static,
            OptionsPagesHandler::GetSupportedServiceNames_static,
            cppu::createSingleComponentFactory,
            0,
            0
        },
        {
            MysqlCDriver::CreateInstance,
            MysqlCDriver::getImplementationName_static,
            MysqlCDriver::getSupportedServiceNames_static,
            cppu::createOneInstanceComponentFactory,
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
        return ::cppu::component_getFactoryHelper( pImplName,
                                                   pServiceManager,
                                                   pRegistryKey ,
                                                   mysqlc::ImplEntries );
    }
}
