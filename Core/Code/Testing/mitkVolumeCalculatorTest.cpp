/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#include "mitkVolumeCalculator.h"
#include "mitkImage.h"
#include "mitkTestingMacros.h"
#include <mitkDataNodeFactory.h>
#include <mitkStandaloneDataStorage.h>


int mitkVolumeCalculatorTest(int /*argc*/, char* argv[])
{
  MITK_TEST_BEGIN("VolumeCalculator")
  const char * filename = argv[1];
  const char * filename3D = argv[2];
  mitk::VolumeCalculator::Pointer volumeCalculator = mitk::VolumeCalculator::New();
  mitk::DataNodeFactory::Pointer nodeReader = mitk::DataNodeFactory::New();
      //*********************************************************************
      // Part I: Testing calculated volume.
      // The correct values have been manually calculated using external software.
      //*********************************************************************

      nodeReader->SetFileName(filename);
      nodeReader->Update();

      mitk::DataNode::Pointer node = nodeReader->GetOutput(0);
      mitk::Image::Pointer image = dynamic_cast<mitk::Image*>(node->GetData());
       MITK_TEST_CONDITION_REQUIRED(
         image.IsNotNull()
    , "01 Check if test image could be loaded");

       volumeCalculator->SetImage(image);
       volumeCalculator->SetThreshold(0);
       volumeCalculator->ComputeVolume();
       float volume = volumeCalculator->GetVolume();

       MITK_TEST_CONDITION_REQUIRED(
         volume == 1600
    , "02 Test Volume Result. Expected 1600 actual value " << volume);

       volumeCalculator->SetThreshold(255);
       volumeCalculator->ComputeVolume();
       volume = volumeCalculator->GetVolume();

       MITK_TEST_CONDITION_REQUIRED(
         volume == 1272.50
         , "03 Test Volume Result. Expected 1272.50 actual value " << volume);

      nodeReader->SetFileName(filename3D);
      nodeReader->Update();

      node = nodeReader->GetOutput(0);
      image = dynamic_cast<mitk::Image*>(node->GetData());

       volumeCalculator->SetImage(image);
       volumeCalculator->SetThreshold(-1023);
       volumeCalculator->ComputeVolume();
       std::vector<float> volumes = volumeCalculator->GetVolumes();

       std::vector<float>::iterator it = volumes.begin();
       for (std::vector<float>::iterator it = volumes.begin(); it != volumes.end(); it++)
       {
       MITK_TEST_CONDITION_REQUIRED(
         (*it) == 24.576f
         , "04 Test Volume Result.");
        }
  MITK_TEST_END()
}
