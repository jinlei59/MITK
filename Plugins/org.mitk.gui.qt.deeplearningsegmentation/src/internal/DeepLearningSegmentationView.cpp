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
#include<QToolButton>
#include<QComboBox>
#include<QFileDialog>

// mitk image
#include <mitkImage.h>
#include <mitkIOUtil.h>

#include <usModule.h>
#include <usServiceTracker.h>
#include <usModuleRegistry.h>
#include <usGetModuleContext.h>
#include <usModuleInitialization.h>
#include <usModuleResource.h>
#include <usModuleResourceStream.h>
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

  m_Controls.m_ImageSelector->SetDataStorage(GetDataStorage());
  m_Controls.m_ImageSelector->SetPredicate(GetImagePredicate());

  m_Controls.buttonPerformImageProcessing->setEnabled(false);

    connect(this->m_Controls.m_ImageSelector,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &DeepLearningSegmentationView::OnImageSelectorChanged);
  connect(m_Controls.buttonLoadTrainedNetwork, &QPushButton::clicked, this, &DeepLearningSegmentationView::DoLoadTrainedNet);
  connect(m_Controls.buttonPerformImageProcessing, &QPushButton::clicked, this, &DeepLearningSegmentationView::DoImageProcessing);

}

void DeepLearningSegmentationView::CreateSegmentationMethodsSelection() 
{
  auto *context = us::GetModuleContext();
  mitk::IDeepLearningSegmentation::ForceLoadModule();
  auto segmentationServiceRefs = context->GetServiceReferences<mitk::IDeepLearningSegmentation>();
  QHBoxLayout *layout = new QHBoxLayout;
  for (auto ref : segmentationServiceRefs)
  {
    auto nameProperty = ref.GetProperty("Name");
    QString name = QString::fromUtf8(nameProperty.ToString().c_str());
    mitk::IDeepLearningSegmentation *segmentationService = dynamic_cast<mitk::IDeepLearningSegmentation *>(
      context->GetService<mitk::IDeepLearningSegmentation>(ref));

    QToolButton *button = new QToolButton();
    button->setAccessibleName(name);
    m_ButtonServiceMap[button] = segmentationService;
    connect(button, &QToolButton::clicked, this, [=]() { this->MethodSelectionChanged(button); });

    //Set icon for button
    auto resource = ref.GetModule()->GetResource("icon.svg");
    if(resource.IsValid())
    {
        //create bytestram from resource
      us::ModuleResourceStream rs(resource, std::ios_base::binary);
      rs.seekg(0, std::ios::end);
      int filesize = rs.tellg();
      rs.seekg(0, ios::beg);
      char *output = new char[filesize];
      rs.read(output, filesize);
      //do Qt things
      QByteArray byt(output, filesize);
      QPixmap pixmap;
      pixmap.loadFromData(byt);
      QIcon ButtonIcon(pixmap);
      button->setIcon(ButtonIcon);
    }
    button->setIconSize(QSize(40, 40));
    button->setCheckable(true);
    button->setChecked(false);
    layout->addWidget(button);
  }
  m_Controls.m_SegmentationMethods->setLayout(layout);
}

void DeepLearningSegmentationView::MethodSelectionChanged(QToolButton *button) 
{
  DisableOtherButtons(button);
  if (button->isChecked())
  {
    QString serviceNameQString = button->accessibleName();
    std::string serviceName = serviceNameQString.toStdString();
    MITK_INFO << "set active Service " << serviceName;
    m_ActiveService = m_ButtonServiceMap[button];
  }
  else
  {
    MITK_INFO << "no Service active";
    m_ActiveService = NULL;
  }
}

void DeepLearningSegmentationView::DisableOtherButtons(QToolButton *button)
{
  for (auto const entry : m_ButtonServiceMap)
  {
    if (entry.first != button)
    {
      entry.first->setChecked(false);
    }
  }
}

void DeepLearningSegmentationView::OnImageSelectorChanged()
{
  auto selectedImageNode = m_Controls.m_ImageSelector->GetSelectedNode();
  if (selectedImageNode != m_selectedImageNode)
  {
    m_selectedImageNode = selectedImageNode;
    if (m_selectedImageNode.IsNotNull())
    {
      m_Controls.labelImageWarning->setVisible(false);
      if (m_TrainedNet != "")
      {
        m_Controls.buttonPerformImageProcessing->setEnabled(true);
      }
      return;
    }
    m_Controls.labelImageWarning->setText("Please select an image!");
    m_Controls.labelImageWarning->setVisible(true);
    m_Controls.buttonPerformImageProcessing->setEnabled(false);
  }
}

void DeepLearningSegmentationView::DoLoadTrainedNet()
{
  QString tempPath = QString::fromStdString(mitk::IOUtil::GetTempPathA());
  QString pretrainedNetResourcesPath =
    QFileDialog::getOpenFileName(nullptr, tr("Open File"), tempPath, tr("Images (*.pth.tar)"));

  m_TrainedNet = pretrainedNetResourcesPath.toStdString();

  if (m_TrainedNet != "")
  {
    m_Controls.labelWarning_2->setVisible(false);
    if (m_selectedImageNode.IsNotNull())
    {
      m_Controls.buttonPerformImageProcessing->setEnabled(true);
    }
  }
}

void DeepLearningSegmentationView::DoImageProcessing()
{
  m_ActiveService->DoSegmentation();
}

mitk::NodePredicateBase::Pointer DeepLearningSegmentationView::GetImagePredicate()
{
  auto isImage = mitk::NodePredicateDataType::New("Image");
  auto hasBinaryProperty = mitk::NodePredicateProperty::New("binary", mitk::BoolProperty::New(true));
  auto isNotBinary = mitk::NodePredicateNot::New(hasBinaryProperty);
  auto isNotBinaryImage = mitk::NodePredicateAnd::New(isImage, isNotBinary);
  auto hasHelperObjectProperty = mitk::NodePredicateProperty::New("helper object", nullptr);
  auto isNoHelperObject = mitk::NodePredicateNot::New(hasHelperObjectProperty);
  auto isNoHelperObjectPredicate = isNoHelperObject.GetPointer();

  auto isImageForImageStatistics = mitk::NodePredicateAnd::New(isNotBinaryImage, isNoHelperObjectPredicate);
  return isImageForImageStatistics.GetPointer();
}
