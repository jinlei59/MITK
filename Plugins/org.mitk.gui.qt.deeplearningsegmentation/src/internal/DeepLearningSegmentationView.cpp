/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "DeepLearningSegmentationView.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>
#include <usModule.h>
#include <usServiceTracker.h>
#include <usModuleRegistry.h>
#include <usGetModuleContext.h>
#include <usModuleInitialization.h>
#include <mitkStandardFileLocations.h>

#include<mitkIDeepLearningSegmentation.h>

US_INITIALIZE_MODULE

const std::string DeepLearningSegmentationView::VIEW_ID = "org.mitk.views.deeplearningsegmentationview";

void DeepLearningSegmentationView::SetFocus()
{
  m_Controls.buttonPerformImageProcessing->setFocus();
}

void DeepLearningSegmentationView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  CreateSegmentationMethodsSelection();
  connect(m_Controls.buttonPerformImageProcessing, &QPushButton::clicked, this, &DeepLearningSegmentationView::DoImageProcessing);
}

void DeepLearningSegmentationView::CreateSegmentationMethodsSelection() 
{
  auto *context = us::GetModuleContext();

  auto segmentationServiceRefs = context->GetServiceReferences<mitk::IDeepLearningSegmentation>();
  QVBoxLayout *vbox = new QVBoxLayout;
  for (auto ref : segmentationServiceRefs)
  {
    auto nameProperty = ref.GetProperty("Name");
    std::string name = nameProperty.ToString();
    mitk::IDeepLearningSegmentation *segmentationService = dynamic_cast<mitk::IDeepLearningSegmentation *>(
      context->GetService<mitk::IDeepLearningSegmentation>(ref));

    std::string buttonText = "&" + name;
    QPushButton *toggleButton = new QPushButton(tr(buttonText.c_str()));
    toggleButton->setCheckable(true);
    toggleButton->setChecked(true);
    vbox->addWidget(toggleButton);
  }
  m_Controls.m_SegmentationMethods->setLayout(vbox);
}

void DeepLearningSegmentationView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      m_Controls.labelWarning->setVisible(false);
      m_Controls.buttonPerformImageProcessing->setEnabled(true);
      return;
    }
  }

  m_Controls.labelWarning->setVisible(true);
  m_Controls.buttonPerformImageProcessing->setEnabled(false);
}

void DeepLearningSegmentationView::DoImageProcessing()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  mitk::DataNode *node = nodes.front();

  if (!node)
  {
    // Nothing selected. Inform the user and return
    QMessageBox::information(nullptr, "Template", "Please load and select an image before starting image processing.");
    return;
  }

  // here we have a valid mitk::DataNode

  // a node itself is not very useful, we need its data item (the image)
  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    mitk::Image *image = dynamic_cast<mitk::Image *>(data);
    if (image)
    {
      std::stringstream message;
      std::string name;
      message << "Performing image processing for image ";
      if (node->GetName(name))
      {
        // a property called "name" was found for this DataNode
        message << "'" << name << "'";
      }
      message << ".";
      MITK_INFO << message.str();

      // actually do something here...
    }
  }
}
