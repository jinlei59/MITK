/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef mitkIDeepLearningSegmentation_h
#define mitkIDeepLearningSegmentation_h

#include<MitkDeepLearningSegmentationExports.h>
//for microservices
#include <mitkServiceInterface.h>

namespace mitk
{
    class MITKDEEPLEARNINGSEGMENTATION_EXPORT IDeepLearningSegmentation
    {
	public:
      virtual void DoSegmentation() = 0;
      static std::string ForceLoadModule();
    protected:

    };
}

MITK_DECLARE_SERVICE_INTERFACE(mitk::IDeepLearningSegmentation, "org.mitk.services.IDeepLearningSegmentation")

#endif
