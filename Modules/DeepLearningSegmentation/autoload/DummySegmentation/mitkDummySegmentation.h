/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef mitkDummySegmentation_h
#define mitkDummySegmentation_h

#include "mitkIDeepLearningSegmentation.h"
#include <itkLightObject.h>

namespace mitk
{
  ///
  /// implementation of the IDeepLearningSegmentation
  class DummySegmentation: public itk::LightObject, public mitk::IDeepLearningSegmentation
  {
  public:
      DummySegmentation();

      ~DummySegmentation() override;

      void DoSegmentation() override;
  protected:

  private:

  };
}
#endif
