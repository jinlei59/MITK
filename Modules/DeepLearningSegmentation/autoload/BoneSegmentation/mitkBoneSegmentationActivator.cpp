/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef mitkBoneSegmentationActivator_h
#define mitkBoneSegmentationActivator_h

// Microservices
#include <usModuleActivator.h>
#include "usModuleContext.h"
#include "mitkBoneSegmentation.h"
#include <usServiceRegistration.h>

namespace mitk
{
    class BoneSegmentationActivator : public us::ModuleActivator
    {
    public:

        void Load(us::ModuleContext* context) override
        {
          //MITK_DEBUG << "BoneSegmentationActivator::Load";
          // Registering BoneSegmentation as MicroService
          m_BoneSegmentation = itk::SmartPointer<mitk::BoneSegmentation>(new BoneSegmentation());

          us::ServiceProperties _BoneSegmentationProps;
          _BoneSegmentationProps["Name"] = std::string("BoneSegmentation");
          //_BoneSegmentationProps["service.ranking"] = int(0);

          m_BoneSegmentationRegistration = context->RegisterService<mitk::IDeepLearningSegmentation>(m_BoneSegmentation.GetPointer(), _BoneSegmentationProps);
        }

        void Unload(us::ModuleContext*) override
        {
          //MITK_DEBUG("BoneSegmentationActivator") << "BoneSegmentationActivator::Unload";
          //MITK_DEBUG("BoneSegmentationActivator") << "m_BoneSegmentation GetReferenceCount " << m_BoneSegmentation->GetReferenceCount();
          m_BoneSegmentationRegistration.Unregister();
          m_BoneSegmentation->Delete();
          //MITK_DEBUG("BoneSegmentationActivator") << "m_BoneSegmentation GetReferenceCount " << m_BoneSegmentation->GetReferenceCount();
        }

        ~BoneSegmentationActivator() override
        {
        }

    private:
        itk::SmartPointer<mitk::BoneSegmentation> m_BoneSegmentation;
        us::ServiceRegistration<BoneSegmentation> m_BoneSegmentationRegistration;
    };
}

US_EXPORT_MODULE_ACTIVATOR(mitk::BoneSegmentationActivator)
#endif
