/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include"SegmentationWorker.h"
#include <usModule.h>
#include <usServiceTracker.h>
#include <usModuleRegistry.h>
#include <usGetModuleContext.h>
#include<mitkIPythonService.h>
#include <usModuleInitialization.h>
#include <mitkStandardFileLocations.h>
#include <QMessageBox.h>

void SegmentationWorker::DoWork(mitk::IDeepLearningSegmentation *service)
{
  MITK_INFO << "In worker";
  service->DoSegmentation();
}
