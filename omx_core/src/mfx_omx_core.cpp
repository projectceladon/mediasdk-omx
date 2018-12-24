// Copyright (c) 2011-2018 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/********************************************************************************

File: mfx_omx_core.cpp

Defined functions:
  - OMX_Init - initializes OMX Core; should be the first call into OMX
  - OMX_Deinit - deinitializes OMX Core; should be the last call into OMX
  - OMX_ComponentNameEnum - enumerates thru all available components
  - OMX_GetHandle - creates component of the given name and returns its handle
  - OMX_FreeHandle - frees handle of the component
  - OMX_SetupTunnel - setups tunnel connection between components
  - OMX_GetContentPipe - returns content pipe; may be not implemented
  - OMX_GetComponentsOfRole - returns components supporting specified role
  - OMX_GetRolesOfComponent - returns roles supported by the specified component

Defined help functions:
  - mfx_omx_get_field - parses line of config file and returns next field
  - mfx_omx_read_config_file - reads registry information from config file

*********************************************************************************/

// this will force exporting of OMX functions
#define __OMX_EXPORTS
// this will define debug file to dump log (if debug printing is enabled)
#define MFX_OMX_FILE_INIT

#include "mfx_omx_utils.h"

#ifdef MFX_RESOURCES_LIMIT
#include "mfx_omx_component_manager.h"
#endif

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_core"

/*------------------------------------------------------------------------------*/

const char* g_MfxOmxProductName = "mediasdk_omx_product_name: Intel(r) Media SDK OpenMAX Integration Layer Core";
const char* g_MfxOmxCopyright = "mediasdk_omx_copyright: " MFX_OMX_COPYRIGHT;
const char* g_MfxOmxFileVersion = "mediasdk_omx_file_version: " MFX_FILE_VERSION;
const char* g_MfxOmxProductVersion = "mediasdk_omx_product_version: " MFX_PRODUCT_VERSION;

/*------------------------------------------------------------------------------*/

const char* g_MfxOmxHwSoName = "libmfx_omx_components_hw.so";

/*------------------------------------------------------------------------------*/

// structure stores components registry information
struct mfx_omx_component_reg
{
    char m_component_name[OMX_MAX_STRINGNAME_SIZE];
    char m_component_so[MFX_OMX_MAX_PATH];
    OMX_U32 m_component_flags;
    OMX_U32 m_component_roles_num;
    char** m_component_roles;
};

/*------------------------------------------------------------------------------*/

// structure stores information about created components
struct mfx_omx_component
{
    OMX_COMPONENTTYPE* m_component;
    mfx_omx_so_handle  m_so_handle;
};

/*------------------------------------------------------------------------------*/

static bool g_bInitialized = false;
// components registry array
static mfx_omx_component_reg* g_ComponentsRegistry = NULL;
static mfxU32 g_ComponentsRegistryNum = 0;
// created components array
static mfx_omx_component* g_Components = NULL;
static mfxU32 g_ComponentsNum = 0;
// mfx OMX IL Core thread safety
static pthread_mutex_t g_OMXCoreLock = PTHREAD_MUTEX_INITIALIZER;
static mfxU32 g_OMXCoreRefCount = 0;
static mfx_omx_so_handle g_soHandleHw = NULL;

#define MFX_OMX_CORE_LOCK()  int res_lock = pthread_mutex_lock(&g_OMXCoreLock); \
    if (res_lock) return OMX_ErrorUndefined;

#define MFX_OMX_CORE_UNLOCK()  int res_unlock = pthread_mutex_unlock(&g_OMXCoreLock); \
    if (res_unlock) return OMX_ErrorUndefined;

/*------------------------------------------------------------------------------*/

#define MAX_LINE_LENGTH 1024
#define FIELD_SEP " \t:"

/* Searches in the given line field which is separated from other lines by
 * FILED_SEP characters, Returns pointer to the beginning of the field and
 * its size.
 */
static void mfx_omx_get_field(const char *line, char **str, size_t *str_size)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (!line || !str || !str_size) return;

    MFX_OMX_AUTO_TRACE_S(line);

    *str = (char*)line;
    for(; strchr(FIELD_SEP, **str) && (**str); ++(*str));
    if (**str)
    {
        char *p = *str;
        for(; !strchr(FIELD_SEP, *p) && (*p); ++p);
        *str_size = (p - *str);

        MFX_OMX_AUTO_TRACE_I32(*str_size);
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("field not found");
        *str = NULL;
        *str_size = 0;
        MFX_OMX_AUTO_TRACE_I32(*str_size);
    }
}

/*------------------------------------------------------------------------------*/

static void mfx_omx_get_component_roles(mfx_omx_component_reg* reg)
{
    OMX_ERRORTYPE omx_sts = OMX_ErrorNone;
    OMX_COMPONENTTYPE component;
    mfx_omx_so_handle so_handle = NULL;

    int b_not_hw = 0;

    strcmp_s(reg->m_component_so, MFX_OMX_MAX_PATH, g_MfxOmxHwSoName, &b_not_hw);

    if (!b_not_hw)
    {
        if (!g_soHandleHw) g_soHandleHw = mfx_omx_so_load(reg->m_component_so);
        so_handle = g_soHandleHw;
    }
    else
    {
        so_handle = mfx_omx_so_load(reg->m_component_so);
    }

    MFX_OMX_ComponentInit_Func component_init_func = NULL;

    component_init_func = (MFX_OMX_ComponentInit_Func)mfx_so_get_addr(so_handle, MFX_OMX_COMPONENT_INIT_FUNC);
    if (component_init_func)
    {
        MFX_OMX_ZERO_MEMORY(component);
        SetStructVersion<OMX_COMPONENTTYPE>(&component);

        omx_sts = component_init_func(reg->m_component_name,
                                      reg->m_component_flags,
                                      OMX_FALSE,
                                      (OMX_HANDLETYPE*)&component);
        if (OMX_ErrorNone == omx_sts)
        {
            OMX_U32 roles_num = 0, index = 0;
            char** roles = NULL;
            char* role = NULL;

            for (index = 0; ; ++index)
            {
                role = (char*)calloc(OMX_MAX_STRINGNAME_SIZE, sizeof(char));
                if (role)
                {
                    omx_sts = component.ComponentRoleEnum(&component, (OMX_U8*)role, index);
                    if (OMX_ErrorNone == omx_sts)
                    {
                        roles_num = reg->m_component_roles_num;
                        roles = (char**)realloc(
                          reg->m_component_roles,
                          (roles_num + 1)*sizeof(char*));
                        if (roles)
                        {
                            reg->m_component_roles = roles;
                            roles[roles_num] = role;
                            reg->m_component_roles_num = roles_num + 1;
                        }
                    }
                    else
                    {
                        MFX_OMX_FREE(role);
                        break;
                    }
                }
            }
            component.ComponentDeInit(&component);
        }
    }
    if (b_not_hw) mfx_omx_so_free(so_handle);
}

static void mfx_omx_free_component_roles(mfx_omx_component_reg* reg)
{
    OMX_U32 index = 0;

    for (index = 0; index < reg->m_component_roles_num; ++index)
    {
        MFX_OMX_FREE(reg->m_component_roles[index]);
    }
    MFX_OMX_FREE(reg->m_component_roles);
}

/*------------------------------------------------------------------------------*/

/* Config file is either:
 *  - ~/.MFX_OMX_CONFIG_FILE_NAME or
 *  - MFX_OMX_CONFIG_FILE_PATH/MFX_OMX_CONFIG_FILE_NAME
 *
 * Config file format (possible lines):
 *  <component_name>:<component_so>
 *  <component_name>:<component_so>:<component_settings>
 *
 * Line in which mistake will be detected will be ignored.
 */
static OMX_ERRORTYPE mfx_omx_read_config_file(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    char config_filename[MFX_OMX_MAX_PATH] = {0};
    FILE* config_file = NULL;

    if (!config_file)
    {
        snprintf_s_ss(config_filename, MFX_OMX_MAX_PATH, "%s/%s", MFX_OMX_CONFIG_FILE_PATH, MFX_OMX_CONFIG_FILE_NAME);
        config_file = fopen(config_filename, "r");

    }
    if (config_file)
    {
        char line[MAX_LINE_LENGTH] = {0}, *str = NULL;
        char *name = NULL, *value = NULL, *str_flags = NULL;
        size_t line_length = 0, n = 0;//, i = 0;
        mfx_omx_component_reg *components = NULL;

        while (NULL != (str = fgets(line, MAX_LINE_LENGTH, config_file)))
        {
            line_length = n = strnlen_s(line, MAX_LINE_LENGTH);
            for(; n && strchr("\n\r", line[n-1]); --n)
            {
                line[n-1] = '\0';
                --line_length;
            }
            MFX_OMX_AUTO_TRACE_S(line);
            MFX_OMX_AUTO_TRACE_I32(line_length);

            // getting name
            mfx_omx_get_field(line, &str, &n);
            //if (!n && ((str+n+1 - line) < line_length)) continue;
            if (!n || !(str[n]))
            {
                // line is empty or field is the last one
                continue;
            }
            if('#' == str[0])
            {
                // skip lines those are started with '#'
                // it is needed when config file is created by script
                continue;
            }

            name = str;
            name[n] = '\0';
            MFX_OMX_AUTO_TRACE_S(name);

            // getting value
            mfx_omx_get_field(str+n+1, &str, &n);
            if (!n) continue;
            value = str;
            value[n] = '\0';
            MFX_OMX_AUTO_TRACE_S(value);

            // getting optional flags
            if ((size_t)(str+n+1 - line) < line_length)
            {
                mfx_omx_get_field(str+n+1, &str, &n);
                if (n)
                {
                    str_flags = str;
                    str_flags[n] = '\0';
                    MFX_OMX_AUTO_TRACE_S(str_flags);
                }
            }

            components = (mfx_omx_component_reg*)realloc(g_ComponentsRegistry, (g_ComponentsRegistryNum+1)*sizeof(mfx_omx_component_reg));
            if (components)
            {
                g_ComponentsRegistry = components;

                strcpy_s(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_name, OMX_MAX_STRINGNAME_SIZE, name);
                strcpy_s(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_so, MFX_OMX_MAX_PATH, value);
                g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_flags =
                    (str_flags)? strtol(str_flags, NULL, 16): (OMX_U32)MFX_OMX_COMPONENT_FLAGS_NONE;

                g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_roles_num = 0;
                g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_roles = NULL;

                mfx_omx_get_component_roles(&g_ComponentsRegistry[g_ComponentsRegistryNum]);

                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_name);
                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_so);
                MFX_OMX_AUTO_TRACE_U32(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_flags);
                MFX_OMX_AUTO_TRACE_U32(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_roles_num);
                MFX_OMX_AUTO_TRACE_P(g_ComponentsRegistry[g_ComponentsRegistryNum].m_component_roles);

                ++g_ComponentsRegistryNum;
            }
        }
        fclose(config_file);
    }
#ifdef MFX_RESOURCES_LIMIT
    snprintf(config_filename, MFX_OMX_MAX_PATH, "%s/%s", MFX_OMX_CONFIG_FILE_PATH, MFX_OMX_COMPONENT_LIMIT_FILE_NAME);
    MfxOmxComponentManager::GetInstanse().LoadConfiguration(config_filename);
#endif
    MFX_OMX_AUTO_TRACE_P(g_ComponentsRegistry);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if (!g_bInitialized)
    {
        MFX_OMX_AUTO_TRACE_P(g_ComponentsRegistry);
        if (!g_ComponentsRegistry)
        {
            omx_res = mfx_omx_read_config_file();
        }

        g_bInitialized = true;
    }

    g_OMXCoreRefCount++;
    MFX_OMX_AUTO_TRACE_I32(g_OMXCoreRefCount);
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_U32 index = 0;

    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if (g_bInitialized)
    {
        MFX_OMX_AUTO_TRACE_P(g_Components);
        MFX_OMX_AUTO_TRACE_I32(g_ComponentsNum);

        --g_OMXCoreRefCount;
        MFX_OMX_AUTO_TRACE_I32(g_OMXCoreRefCount);

        if(0 == g_OMXCoreRefCount)
        {
            // TODO: is it needed to deinitialize components here?
            MFX_OMX_FREE(g_Components);
            for (index = 0; index < g_ComponentsRegistryNum; ++index)
            {
                mfx_omx_free_component_roles(&g_ComponentsRegistry[index]);
            }
            MFX_OMX_FREE(g_ComponentsRegistry);

            g_ComponentsNum = 0;
            g_ComponentsRegistryNum = 0;

            g_bInitialized = false;
        }
        if (g_soHandleHw)
        {
            mfx_omx_so_free(g_soHandleHw);
            g_soHandleHw = NULL;
        }
    }
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(cComponentName);
    MFX_OMX_AUTO_TRACE_I32(nNameLength);
    MFX_OMX_AUTO_TRACE_I32(nIndex);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !cComponentName)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && (nNameLength < OMX_MAX_STRINGNAME_SIZE))
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // enumerating components
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if ((OMX_ErrorNone == omx_res) && g_bInitialized)
    {
        if (nIndex >= g_ComponentsRegistryNum)
        {
            omx_res = OMX_ErrorNoMore;
        }
        else
        {
            strcpy_s(cComponentName, OMX_MAX_STRINGNAME_SIZE, g_ComponentsRegistry[nIndex].m_component_name);
            MFX_OMX_AUTO_TRACE_S(cComponentName);
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxU32 component_index = 0;
    OMX_COMPONENTTYPE* omx_component = NULL;

    MFX_OMX_AUTO_TRACE_P(pHandle);
    MFX_OMX_AUTO_TRACE_S(cComponentName);
    MFX_OMX_AUTO_TRACE_P(pAppData);
    MFX_OMX_AUTO_TRACE_P(pCallBacks);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pHandle)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !cComponentName)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && strncmp(cComponentName,
                                              MFX_OMX_VALID_COMPONENT_PREFIX,
                                              sizeof(MFX_OMX_VALID_COMPONENT_PREFIX)-1))
    {
        omx_res = OMX_ErrorInvalidComponentName;
    }
    if ((OMX_ErrorNone == omx_res) && (strnlen_s(cComponentName, OMX_MAX_STRINGNAME_SIZE + 1) > OMX_MAX_STRINGNAME_SIZE))
    {
        omx_res = OMX_ErrorInvalidComponentName;
    }
    // creating component
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if ((OMX_ErrorNone == omx_res) && g_bInitialized)
    {
        // searching for the component in the registry
        if (OMX_ErrorNone == omx_res)
        {
            for (component_index = 0; component_index < g_ComponentsRegistryNum; ++component_index)
            {
                MFX_OMX_AUTO_TRACE_I32(component_index);
                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[component_index].m_component_name);
                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[component_index].m_component_so);
                if (!strcmp(g_ComponentsRegistry[component_index].m_component_name, cComponentName))
                {
                    break;
                }
            }
            if (component_index >= g_ComponentsRegistryNum) omx_res = OMX_ErrorComponentNotFound;
        }
#ifdef MFX_RESOURCES_LIMIT
        if(g_ComponentsNum > 0)
        {
            for (int index = 0; index < g_ComponentsNum; ++index)
            {
                MfxOmxComponentManager::GetInstanse().UpdateComponentTable(g_Components[index].m_component);
            }
            if(!MfxOmxComponentManager::GetInstanse().IsResourceAvailable()) omx_res = OMX_ErrorInsufficientResources;
        }
#endif
        // allocating and initializing component
        if (OMX_ErrorNone == omx_res)
        {
            mfx_omx_component *components = NULL;
            mfx_omx_so_handle so_handle = NULL;
            MFX_OMX_ComponentInit_Func component_init_func = NULL;

            MFX_OMX_AUTO_TRACE_I32(g_ComponentsNum);
            MFX_OMX_AUTO_TRACE_P(g_Components);

            components = (mfx_omx_component*)realloc(g_Components, (g_ComponentsNum+1)*sizeof(mfx_omx_component));
            if (!components) omx_res = OMX_ErrorInsufficientResources;
            else
            {
                g_Components = components;

                if (OMX_ErrorNone == omx_res)
                {
                    omx_component = (OMX_COMPONENTTYPE*)calloc(1, sizeof(OMX_COMPONENTTYPE));
                    if (!omx_component) omx_res = OMX_ErrorInsufficientResources;
                    else
                    {
                        SetStructVersion<OMX_COMPONENTTYPE>(omx_component);
                    }
                }
                if (OMX_ErrorNone == omx_res)
                {
                    so_handle = mfx_omx_so_load(g_ComponentsRegistry[component_index].m_component_so);
                    component_init_func = (MFX_OMX_ComponentInit_Func)mfx_so_get_addr(so_handle, MFX_OMX_COMPONENT_INIT_FUNC);

                    if (component_init_func)
                        omx_res = component_init_func(cComponentName,
                                                      g_ComponentsRegistry[component_index].m_component_flags,
                                                      OMX_TRUE,
                                                      (OMX_HANDLETYPE*)omx_component);
                    else omx_res = OMX_ErrorInvalidComponent;
                }
                if (OMX_ErrorNone == omx_res)
                {
                    omx_res = omx_component->SetCallbacks((OMX_HANDLETYPE*)omx_component, pCallBacks, pAppData);
                }
                if (OMX_ErrorNone == omx_res)
                {
                    *pHandle = (OMX_HANDLETYPE*)omx_component;

                    g_Components[g_ComponentsNum].m_component = omx_component;
                    g_Components[g_ComponentsNum].m_so_handle = so_handle;

                    MFX_OMX_AUTO_TRACE_P(g_Components[g_ComponentsNum].m_component);
                    MFX_OMX_AUTO_TRACE_P(g_Components[g_ComponentsNum].m_so_handle);

                    ++g_ComponentsNum;
                }
                else if (OMX_ErrorNone != omx_res)
                {
                    MFX_OMX_FREE(omx_component);
                    mfx_omx_so_free(so_handle);
                }
            }
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxU32 component_index = 0;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;

    MFX_OMX_AUTO_TRACE_P(hComponent);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // searching for the component
    if (OMX_ErrorNone == omx_res)
    {
        for (component_index = 0; component_index < g_ComponentsNum; ++component_index)
        {
            if (omx_component == g_Components[component_index].m_component) break;
        }
        if (component_index >= g_ComponentsNum) omx_res = OMX_ErrorInvalidComponent;
    }
    // releasing component
    if (OMX_ErrorNone == omx_res)
    {
        // TODO: is it needed to check component state here?
        g_Components[component_index].m_component->ComponentDeInit(hComponent);
        MFX_OMX_FREE(g_Components[component_index].m_component);
        mfx_omx_so_free(g_Components[component_index].m_so_handle);

        for (; component_index+1 < g_ComponentsNum; ++component_index)
        {
            g_Components[component_index] = g_Components[component_index+1];
        }
        --g_ComponentsNum;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(hOutput);
    MFX_OMX_UNUSED(nPortOutput);
    MFX_OMX_UNUSED(hInput);
    MFX_OMX_UNUSED(nPortInput);
    OMX_ERRORTYPE omx_res = OMX_ErrorNotImplemented; //OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(hOutput);
    MFX_OMX_AUTO_TRACE_I32(nPortOutput);
    MFX_OMX_AUTO_TRACE_P(hInput);
    MFX_OMX_AUTO_TRACE_I32(nPortInput);

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(hPipe);
    MFX_OMX_UNUSED(szURI);
    OMX_ERRORTYPE omx_res = OMX_ErrorNotImplemented; //OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole(
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxU32 component_index = 0, role_index = 0;

    // errors checking
    if ((OMX_ErrorNone == omx_res) && !role)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !pNumComps)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // getting components os requested role
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if ((OMX_ErrorNone == omx_res) && g_bInitialized)
    {
        // searching for the component in the registry
        if (OMX_ErrorNone == omx_res)
        {
            OMX_U32 num_comps = *pNumComps;

            *pNumComps = 0;
            for (component_index = 0; component_index < g_ComponentsRegistryNum; ++component_index)
            {
                MFX_OMX_AUTO_TRACE_I32(component_index);
                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[component_index].m_component_name);
                for (role_index = 0; role_index < g_ComponentsRegistry[component_index].m_component_roles_num; ++role_index)
                {
                    if (!strcmp(role, g_ComponentsRegistry[component_index].m_component_roles[role_index]))
                    {
                        if (compNames)
                        {
                            if (num_comps <= *pNumComps)
                                omx_res = OMX_ErrorBadParameter;
                            else
                                strcpy_s((char*)compNames[*pNumComps], OMX_MAX_STRINGNAME_SIZE, g_ComponentsRegistry[component_index].m_component_name);
                        }
                        ++(*pNumComps);
                        break;
                    }
                }
                if (OMX_ErrorNone != omx_res) break;
            }
        }
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxU32 component_index = 0;

    MFX_OMX_AUTO_TRACE_S(compName);
    MFX_OMX_AUTO_TRACE_P(pNumRoles);
    MFX_OMX_AUTO_TRACE_P(roles);

    // errors checking
    if ((OMX_ErrorNone == omx_res) && !compName)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !pNumRoles)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // getting component roles
    MFX_OMX_AUTO_TRACE_I32(g_bInitialized);
    if ((OMX_ErrorNone == omx_res) && g_bInitialized)
    {
        // searching for the component in the registry
        if (OMX_ErrorNone == omx_res)
        {
            for (component_index = 0; component_index < g_ComponentsRegistryNum; ++component_index)
            {
                MFX_OMX_AUTO_TRACE_I32(component_index);
                MFX_OMX_AUTO_TRACE_S(g_ComponentsRegistry[component_index].m_component_name);
                if (!strcmp(g_ComponentsRegistry[component_index].m_component_name, compName))
                {
                    break;
                }
            }
            if (component_index >= g_ComponentsRegistryNum) omx_res = OMX_ErrorInvalidComponentName;
        }
        if (OMX_ErrorNone == omx_res)
        {
            OMX_U32 num_roles = g_ComponentsRegistry[component_index].m_component_roles_num;

            if (!roles)
            {
                // in this case we need to return number of roles
                *pNumRoles = num_roles;
            }
            else
            {
                // in this case we are listing all roles
                OMX_U32 array_size = *pNumRoles, i = 0;

                if (array_size < num_roles) omx_res = OMX_ErrorBadParameter;
                else
                {
                    *pNumRoles = 0;
                    for (i = 0; i < num_roles; ++i)
                    {
                        if (!roles[i])
                        {
                            omx_res = OMX_ErrorBadParameter;
                            break;
                        }
                        strcpy_s((char*)roles[i], OMX_MAX_STRINGNAME_SIZE, g_ComponentsRegistry[component_index].m_component_roles[i]);
                        ++(*pNumRoles);
                    }
                }
            }
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    MFX_OMX_CORE_UNLOCK();
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */
