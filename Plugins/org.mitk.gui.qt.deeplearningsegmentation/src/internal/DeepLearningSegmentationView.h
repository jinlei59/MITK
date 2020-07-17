/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/


#ifndef DeepLearningSegmentationView_h
#define DeepLearningSegmentationView_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>
#include <QToolButton>
#include <QThread>

#include "ui_DeepLearningSegmentationViewControls.h"

#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateNot.h>
#include<mitkIDeepLearningSegmentation.h>

#include"SegmentationWorker.h"

/**
  \brief DeepLearningSegmentationView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class DeepLearningSegmentationView : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  
signals:
  void Operate(mitk::IDeepLearningSegmentation *service);

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  void CreateSegmentationMethodsSelection();

  virtual void SetFocus() override;

  /// \brief called by QmitkFunctionality when DataManager's selection has changed
  void OnImageSelectorChanged();

  /// \brief Called when the user clicks the GUI button
  void DoImageProcessing();

  mitk::NodePredicateBase::Pointer GetImagePredicate();

  void DoLoadTrainedNet();

  void MethodSelectionChanged(QToolButton *button);

  void DisableOtherButtons(QToolButton *button);

  Ui::DeepLearningSegmentationViewControls m_Controls;

  mitk::DataNode::Pointer m_selectedImageNode = nullptr;
  std::string m_TrainedNet;
  std::map<QToolButton*, mitk::IDeepLearningSegmentation*> m_ButtonServiceMap;
  mitk::IDeepLearningSegmentation *m_ActiveService;

  QThread *m_SegmentationThread;
  SegmentationWorker *m_Worker;
};

#endif // DeepLearningSegmentationView_h
