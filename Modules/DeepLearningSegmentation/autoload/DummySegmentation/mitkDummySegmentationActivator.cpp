/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef mitkDummySegmentationActivator_h
#define mitkDummySegmentationActivator_h

// Microservices
#include <usModuleActivator.h>
#include "usModuleContext.h"
#include "mitkDummySegmentation.h"
#include <usServiceRegistration.h>

namespace mitk
{
    class DummySegmentationActivator : public us::ModuleActivator
    {
    public:

        void Load(us::ModuleContext* context) override
        {
          //MITK_DEBUG << "DummySegmentationActivator::Load";
          // Registering DummySegmentation as MicroService
          m_DummySegmentation = itk::SmartPointer<mitk::DummySegmentation>(new DummySegmentation());

          us::ServiceProperties _DummySegmentationProps;
          _DummySegmentationProps["Name"] = std::string("DummySegmentation");
          //_DummySegmentationProps["service.ranking"] = int(0);

          m_DummySegmentationRegistration = context->RegisterService<mitk::IDeepLearningSegmentation>(m_DummySegmentation.GetPointer(), _DummySegmentationProps);
        }

        void Unload(us::ModuleContext*) override
        {
          //MITK_DEBUG("DummySegmentationActivator") << "DummySegmentationActivator::Unload";
          //MITK_DEBUG("DummySegmentationActivator") << "m_DummySegmentation GetReferenceCount " << m_DummySegmentation->GetReferenceCount();
          m_DummySegmentationRegistration.Unregister();
          m_DummySegmentation->Delete();
          //MITK_DEBUG("DummySegmentationActivator") << "m_DummySegmentation GetReferenceCount " << m_DummySegmentation->GetReferenceCount();
        }

        ~DummySegmentationActivator() override
        {
        }

    private:
        itk::SmartPointer<mitk::DummySegmentation> m_DummySegmentation;
        us::ServiceRegistration<DummySegmentation> m_DummySegmentationRegistration;
    };
}

US_EXPORT_MODULE_ACTIVATOR(mitk::DummySegmentationActivator)
#endif
