// Copyright (c) 2016-2018 Intel Corporation
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

#ifdef MFX_RESOURCES_LIMIT

#include <cstdio>
#include <tinyxml2.h>
#include <map>
#include <string>
#include <climits>
#include <cstdlib>
#include <cctype>
#include <functional>

#include "mfx_omx_defs.h"
#include "mfx_omx_utils.h"
#include "mfx_omx_component_manager.h"

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_component_manager"

std::map<const std::string, CodecType_e> codecNames =
{
        {"VP8" , CodecType_e::CODEC_TYPE_VP8 },
        {"VP9" , CodecType_e::CODEC_TYPE_VP9 },
        {"HEVC", CodecType_e::CODEC_TYPE_HEVC},
        {"AVC" , CodecType_e::CODEC_TYPE_AVC },
        {"MP2" , CodecType_e::CODEC_TYPE_MP2 }
};

std::map<const std::string, ResolutionType_e> resolutionTypeNames =
{
        {"480" , ResolutionType_e::Resolution_480 },
        {"720" , ResolutionType_e::Resolution_720 },
        {"1080", ResolutionType_e::Resolution_1080},
        {"2K"  , ResolutionType_e::Resolution_2K  },
        {"4K"  , ResolutionType_e::Resolution_4K  }
};

MfxOmxComponentManager::MfxOmxComponentManager()
{
    MFX_OMX_AUTO_TRACE_FUNC();
    //overwrite this variable if needed depending on driver or xml settings
    mMaxResourcesNum = MFX_RESOURCES_LIMIT;
    MFX_OMX_AUTO_TRACE_U32(mMaxResourcesNum);

}

void MfxOmxComponentManager::SetDefaultConfiguration() noexcept
{
    MFX_OMX_AUTO_TRACE_FUNC();

    unsigned int numOfCodecs = static_cast<unsigned int>(CodecType_e::CODEC_TYPE_MAX);
    unsigned int numOfResolutions = static_cast<unsigned int>(ResolutionType_e::Resolution_MAX);

    CodecLimitInfo component;
    component.maxLimit = mMaxResourcesNum;
    component.currentLimit = 0;

    mLock.lock();
    for(unsigned int cIndex(1); cIndex < numOfCodecs; ++cIndex )
    {
        component.codecInfo.codecType = static_cast<CodecType_e>(cIndex);
        for(unsigned int rIndex(1); rIndex < numOfResolutions; ++rIndex)
        {
            component.codecInfo.resolutionType = static_cast<ResolutionType_e>(rIndex);
            component.codecInfo.frameRate = 30;
            component.codecInfo.isSecured = false;

            component.codecInfo.isEncoder = true;
            mResources.push_back(component);

            component.codecInfo.isEncoder = false;
            mResources.push_back(component);
        }
    }
    mLock.unlock();
}

bool MfxOmxComponentManager::IsResourceAvailable()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    bool isAvailable = true;
    if(mMaxResourcesNum <= mCurrNumOfComponents)
    {
        MFX_OMX_AUTO_TRACE_MSG("Maximum of the components are reached");
        MFX_OMX_AUTO_TRACE_U32(mMaxResourcesNum);
        isAvailable = false; // maximum of the resources are reached
    }
    if(isAvailable)
    {
        //check limit for every component
        for(auto component: mActiveResources)
        {
            if(!CheckComponent(component)) isAvailable = false;
        }
    }
    mActiveResources.clear();
    mCurrNumOfComponents = 0;
    for(auto& component: mResources)
    {
        component.currentLimit = 0;
    }
    return isAvailable;
}

bool MfxOmxComponentManager::LoadConfiguration(const std::string& configFilePath)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    using namespace tinyxml2;

    mLock.lock();
    mResources.clear();

    XMLDocument document;
    //Try to load XML file from folder
    if(document.LoadFile(configFilePath.c_str()) != XML_NO_ERROR)
    {
        SetDefaultConfiguration();
    }
    else
    {
      //Find first 'Codec' tag
      MFX_OMX_AUTO_TRACE_MSG("Read data from XML");
      mMaxResourcesNum = 0;
      XMLElement* element = document.RootElement()->FirstChildElement("Codec");
      while(element)
      {
          MFX_OMX_AUTO_TRACE_MSG("Element Found");
          //find children tags for 'Codec'
          const XMLElement* codecType       = element->FirstChildElement("CodecType");
          const XMLElement* isEncoder       = element->FirstChildElement("IsEncoder");
          const XMLElement* isSecured       = element->FirstChildElement("IsSecured");
          const XMLElement* resolutionType  = element->FirstChildElement("ResolutionType");
          const XMLElement* frameRate       = element->FirstChildElement("FrameRate");
          const XMLElement* limit           = element->FirstChildElement("Limit");

          //each tag above must be present
          if(codecType && isEncoder && resolutionType && frameRate && limit)
          {
                MFX_OMX_AUTO_TRACE_MSG("Nodes Found");
                CodecLimitInfo currResource;
                MFX_OMX_TRY_AND_CATCH(currResource.codecInfo.codecType = codecNames.at(codecType->GetText()),
                                      currResource.codecInfo.codecType = CodecType_e::CODEC_TYPE_NONE);

                currResource.codecInfo.isEncoder = strcmp(isEncoder->GetText(),"true") == 0;
                currResource.codecInfo.isSecured = strcmp(isSecured->GetText(),"true") == 0;

                MFX_OMX_TRY_AND_CATCH(currResource.codecInfo.resolutionType = resolutionTypeNames.at(resolutionType->GetText()),
                                      currResource.codecInfo.resolutionType = ResolutionType_e::Resolution_NONE);

                long temp = atol(frameRate->GetText());
                currResource.codecInfo.frameRate = (temp < 0 || temp > UINT_MAX) ? 0 : temp;

                temp = atol(limit->GetText());
                currResource.maxLimit = (temp < 0 || temp > UINT_MAX) ? 0 : temp;
                currResource.currentLimit = 0;

                if(!IsResourceExist(currResource.codecInfo))
                {
                    mMaxResourcesNum += currResource.maxLimit;
                    mResources.push_back(std::move(currResource));
                }
          }
          element = element->NextSiblingElement("Codec");
      }
    }
    mLock.unlock();
    return true;
}

bool MfxOmxComponentManager::CheckComponent(CodecInfo currResource)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mLock.lock();

    if(!IsResourceExist(currResource))
    {
        MFX_OMX_AUTO_TRACE_MSG("Component is not supported");
        return false; // resource is not supported
    }

    //get current resource
    auto resource = std::find_if(std::begin(mResources),
                                 std::end(mResources),
                                 std::bind(&MfxOmxComponentManager::IsSameResources, this, std::placeholders::_1, currResource));

    if(resource->currentLimit == resource->maxLimit)
    {
        MFX_OMX_AUTO_TRACE_MSG("Maximum of the current component are reached");
        return false;
    }
    resource->currentLimit++;

    mLock.unlock();
    return true;
}

bool MfxOmxComponentManager::IsResourceExist(CodecInfo info)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    for(auto e: mResources)
    {
        if(IsSameResources(e, info)) return true;
    }
    return false;
}

inline bool MfxOmxComponentManager::IsSameResources(CodecLimitInfo source, CodecInfo target)
{
    return (source.codecInfo.codecType == target.codecType &&
            source.codecInfo.frameRate == target.frameRate &&
            source.codecInfo.isEncoder == target.isEncoder &&
            source.codecInfo.isSecured == target.isSecured &&
            source.codecInfo.resolutionType == target.resolutionType);
}

void MfxOmxComponentManager::UpdateComponentTable(OMX_COMPONENTTYPE* pComponent)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_P(pComponent);

    if(!pComponent) return;
    if(!pComponent->GetParameter && pComponent->GetComponentVersion) return;

    OMX_PARAM_PORTDEFINITIONTYPE sParam;
    OMX_ERRORTYPE retCode = OMX_ErrorNone;
    char compName[OMX_MAX_STRINGNAME_SIZE];
    OMX_VERSIONTYPE compVer, compSpecVer;
    OMX_UUIDTYPE compUUID;

    SetStructVersion<OMX_PARAM_PORTDEFINITIONTYPE>(&sParam);
    sParam.nPortIndex = 1; //MFX_OMX_OUTPUT_PORT_INDEX
    if((retCode = pComponent->GetParameter(pComponent, OMX_IndexParamPortDefinition, &sParam)) != OMX_ErrorNone)
    {
        MFX_OMX_AUTO_TRACE_I32(retCode);
        return;
    }
    if((retCode = pComponent->GetComponentVersion(pComponent, compName, &compVer, &compSpecVer, &compUUID)) != OMX_ErrorNone)
    {
        MFX_OMX_AUTO_TRACE_I32(retCode);
        return;
    }
    if(sParam.eDomain != OMX_PortDomainVideo) return;

    std::string componentName{compName};
    CodecInfo currResource;
    currResource.codecType = CodecType_e::CODEC_TYPE_NONE;;
    currResource.isEncoder = false;
    currResource.isSecured = false;
    currResource.resolutionType = ResolutionType_e::Resolution_NONE;
    currResource.frameRate = 0;

    if(componentName.find("h264")       != std::string::npos) { currResource.codecType = CodecType_e::CODEC_TYPE_AVC;  }
    else if(componentName.find("h265")  != std::string::npos) { currResource.codecType = CodecType_e::CODEC_TYPE_HEVC; }
    else if(componentName.find("vp8")   != std::string::npos) { currResource.codecType = CodecType_e::CODEC_TYPE_VP8;  }
    else if(componentName.find("vp9")   != std::string::npos) { currResource.codecType = CodecType_e::CODEC_TYPE_VP9;  }
    else if(componentName.find("mpeg2") != std::string::npos) { currResource.codecType = CodecType_e::CODEC_TYPE_MP2;  }
    else {  MFX_OMX_AUTO_TRACE_MSG("Not found correct codec name"); return;  /* not found correct codec name*/ }

    if(componentName.find("vd") != std::string::npos) { currResource.isEncoder = true; }
    else if(componentName.find("ve") == std::string::npos) { MFX_OMX_AUTO_TRACE_MSG("Not found encoder/decoder identifier"); return; /*not found encoder/decoder identifier*/ }

    if(componentName.find("secure") != std::string::npos) { currResource.isSecured = true; }

    if (sParam.format.video.nFrameHeight <= 480)      { currResource.resolutionType = ResolutionType_e::Resolution_480;  }
    else if (sParam.format.video.nFrameHeight <= 720) { currResource.resolutionType = ResolutionType_e::Resolution_720;  }
    else if (sParam.format.video.nFrameHeight <= 1080){ currResource.resolutionType = ResolutionType_e::Resolution_1080; }
    else if (sParam.format.video.nFrameHeight <= 1440){ currResource.resolutionType = ResolutionType_e::Resolution_2K;   }
    else if (sParam.format.video.nFrameHeight <= 2160){ currResource.resolutionType = ResolutionType_e::Resolution_4K;   }
    else { MFX_OMX_AUTO_TRACE_MSG("Not found correct frame rate"); return; }

    currResource.frameRate = static_cast<unsigned int>(sParam.format.video.xFramerate >> 16);
    if (currResource.frameRate > 55 && currResource.frameRate < 65)  { currResource.frameRate = 60; }
    // This is a w/a to set default frame rate as 30 in case it is not set from framework.
    else { currResource.frameRate = 30; }

    mLock.lock();
    mActiveResources.push_back(std::move(currResource));
    mCurrNumOfComponents++;
    mLock.unlock();
}

#endif //#ifdef MFX_RESOURCES_LIMIT
