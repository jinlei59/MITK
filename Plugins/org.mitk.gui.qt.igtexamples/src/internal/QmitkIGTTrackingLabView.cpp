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


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "QmitkIGTTrackingLabView.h"
#include "QmitkStdMultiWidget.h"

#include <QmitkNDIConfigurationWidget.h>
#include <QmitkFiducialRegistrationWidget.h>
#include <QmitkUpdateTimerWidget.h>
#include <QmitkToolSelectionWidget.h>
#include <QmitkToolTrackingStatusWidget.h>


#include <mitkCone.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>

#include <vtkConeSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>

// Qt
#include <QMessageBox>
#include <QIcon>


const std::string QmitkIGTTrackingLabView::VIEW_ID = "org.mitk.views.igttrackinglab";

QmitkIGTTrackingLabView::QmitkIGTTrackingLabView()
: QmitkFunctionality()
,m_Source(NULL)
,m_PermanentRegistrationFilter(NULL)
,m_Visualizer(NULL)
,m_VirtualView(NULL)
,m_PSRecordingPointSet(NULL)
,m_RegistrationTrackingFiducialsName("Tracking Fiducials")
,m_RegistrationImageFiducialsName("Image Fiducials")
,m_PointSetRecordingDataNodeName("Recorded Points")
,m_PointSetRecording(false)
,m_ImageFiducialsDataNode(NULL)
,m_TrackerFiducialsDataNode(NULL)
,m_PermanentRegistrationSourcePoints(NULL)
{

  //[-1;0;0] for WOLF_6D bronchoscope
  m_DirectionOfProjectionVector[0]=0;
  m_DirectionOfProjectionVector[1]=0;
  m_DirectionOfProjectionVector[2]=-1;}

QmitkIGTTrackingLabView::~QmitkIGTTrackingLabView()
{
}

void QmitkIGTTrackingLabView::CreateQtPartControl( QWidget *parent )
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi( parent );

  m_ToolBox = m_Controls.m_ToolBox;

  this->CreateBundleWidgets( parent );
  this->CreateConnections();
}


void QmitkIGTTrackingLabView::CreateBundleWidgets( QWidget* parent )
{
  //initialize configuration widget
  //m_NDIConfigWidget = m_Controls.m_NDIConfigWidget;
  //m_NDIConfigWidget->SetToolTypes(QStringList () << "Instrument" << "Fiducial" << "Skinmarker" << "Unknown" );

  //initialize registration widget
  m_RegistrationWidget = m_Controls.m_RegistrationWidget;
  m_RegistrationWidget->HideStaticRegistrationRadioButton(true);
  m_RegistrationWidget->HideContinousRegistrationRadioButton(true);
  m_RegistrationWidget->HideUseICPRegistrationCheckbox(true);

  //create widget for pointset recording
  m_Controls.m_PointSetRecordingLayout->addWidget(this->CreatePointSetRecordingWidget(parent));

  //initialize virtual view tab
  m_VirtualViewToolSelectionWidget = m_Controls.m_VirtualViewToolSelectionWidget;
  m_VirtualViewToolSelectionWidget->SetCheckboxtText("Enable Virtual Camera");

  //initialize tracking status widget
  m_ToolStatusWidget = m_Controls.m_ToolStatusWidget;

  //initialize update timer
  m_RenderingTimerWidget = m_Controls.m_RenderingTimerWidget;
  m_RenderingTimerWidget->SetPurposeLabelText(QString("Navigation"));
  m_RenderingTimerWidget->SetTimerInterval( 50 );  // set rendering timer at 20Hz (updating every 50msec)
}


void QmitkIGTTrackingLabView::CreateConnections()
{
  m_Timer = new QTimer(this);
  connect(m_Timer, SIGNAL(timeout()), this, SLOT(UpdateTimer()));

  connect( m_ToolBox, SIGNAL(currentChanged(int)), this, SLOT(OnToolBoxCurrentChanged(int)) );

  connect( m_Controls.m_UsePermanentRegistrationToggle, SIGNAL(toggled(bool)), this, SLOT(OnPermanentRegistration(bool)) );

  //connect( m_NDIConfigWidget, SIGNAL(Connected()), m_RenderingTimerWidget, SLOT(EnableWidget()) );
  /*connect( m_NDIConfigWidget, SIGNAL(Disconnected()), this, SLOT(OnTrackerDisconnected()) );
  connect( m_NDIConfigWidget, SIGNAL(Connected()), this, SLOT(OnSetupNavigation()) );
  connect( m_NDIConfigWidget, SIGNAL(SignalToolNameChanged(int, QString)), this, SLOT(OnChangeToolName(int, QString)) );
  connect( m_NDIConfigWidget, SIGNAL(SignalLoadTool(int, mitk::DataNode::Pointer)), this, SLOT(OnToolLoaded(int, mitk::DataNode::Pointer)) );
  connect( m_NDIConfigWidget, SIGNAL(ToolsAdded(QStringList)), this, SLOT(OnToolsAdded(QStringList)) );
  connect( m_NDIConfigWidget, SIGNAL(RepresentationChanged( int ,mitk::Surface::Pointer )), this, SLOT(ChangeToolRepresentation( int, mitk::Surface::Pointer )));*/
  connect( m_Controls.m_TrackingDeviceSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), m_RenderingTimerWidget, SLOT(EnableWidget()) );
  connect( m_Controls.m_TrackingDeviceSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupNavigation()) );

  connect( m_Controls.m_UseAsPointerButton, SIGNAL(clicked()), this, SLOT(OnInstrumentSelected()) );
  connect( m_Controls.m_UseAsObjectmarkerButton, SIGNAL(clicked()), this, SLOT(OnObjectmarkerSelected()) );




  connect( m_RegistrationWidget, SIGNAL(AddedTrackingFiducial()), this, SLOT(OnAddRegistrationTrackingFiducial()) );
  connect( m_RegistrationWidget, SIGNAL(PerformFiducialRegistration()), this, SLOT(OnRegisterFiducials()) );

  connect( m_RenderingTimerWidget, SIGNAL(Started()), this, SLOT(OnStartNavigation()) );
  connect( m_RenderingTimerWidget, SIGNAL(Stopped()), this, SLOT(OnStopNavigation()) );

  //TODO
  //connect( m_VirtualViewToolSelectionWidget, SIGNAL(SignalUseTool(int, bool)), this, SLOT(OnVirtualCamera(int, bool)));

  //initialize Combo Box
  m_Controls.m_ObjectComboBox->SetDataStorage(this->GetDataStorage());
  m_Controls.m_ObjectComboBox->SetAutoSelectNewItems(true);
  //m_Controls->m_ObjectComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));


}

void QmitkIGTTrackingLabView::UpdateTimer()
  {
  m_PermanentRegistrationFilter->Update();
  }


void QmitkIGTTrackingLabView::OnAddRegistrationTrackingFiducial()
{
  /*
  mitk::DataStorage* ds = this->GetDefaultDataStorage(); // check if DataStorage available
  if(ds == NULL)
    throw std::invalid_argument("DataStorage is not available");

  if (m_FiducialRegistrationFilter.IsNull())
  {
    std::string message( "IGT Pipeline is not ready. Please 'Start Navigation' before adding points");
    QMessageBox::warning(NULL, "Adding Fiducials not possible", message.c_str());
    return;
  }

  if (m_FiducialRegistrationFilter->GetNumberOfOutputs() < 1 || m_FiducialRegistrationFilter->GetNumberOfInputs() < 1)
  {
    std::string message("There are no tracking instruments! Please add an instrument first!");
    QMessageBox::warning(NULL, "Adding Fiducials not possible", message.c_str());
    return;
  }

  if (m_FiducialRegistrationFilter->GetInput()->IsDataValid() == false)
  {
    std::string message("instrument can currently not be tracked. Please make sure that the instrument is visible to the tracker");
    QMessageBox::warning(NULL, "Adding Fiducials not possible", message.c_str());
    return;
  }

  */
  mitk::NavigationData::Pointer nd = m_InstrumentNavigationData;

  if( nd.IsNull() || !nd->IsDataValid())
    QMessageBox::warning( 0, "Invalid tracking data", "Navigation data is not available or invalid!", QMessageBox::Ok );

  // in case the tracker fiducials datanode has been renamed or removed
  //if(trackerFiducialsPS.IsNull())
  //{
  //  mitk::DataNode::Pointer trackerFiducialsDN = mitk::DataNode::New();
  //  trackerFiducialsDN->SetName(m_RegistrationTrackingFiducialsName);
  //  trackerFiducialsPS = mitk::PointSet::New();
  //  trackerFiducialsDN->SetData(trackerFiducialsPS);
  //  m_RegistrationWidget->SetTrackerFiducialsNode(trackerFiducialsDN);
  //}



  if(m_TrackerFiducialsDataNode.IsNotNull() && m_TrackerFiducialsDataNode->GetData() != NULL)
  {
    mitk::PointSet::Pointer ps = dynamic_cast<mitk::PointSet*>(m_TrackerFiducialsDataNode->GetData());
    ps->InsertPoint(ps->GetSize(), nd->GetPosition());
  }
  else
    QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access Tracker Fiducials. Adding fiducial not possible!");


}

void QmitkIGTTrackingLabView::OnInstrumentSelected()
{
  if (m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().IsNotNull())
    {
    m_InstrumentNavigationData = m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedToolID());
    }
  else
    {
    m_Controls.m_PointerNameLabel->setText("<not available>");
    return;
    }

  if (m_InstrumentNavigationData.IsNotNull())
    {
    m_Controls.m_PointerNameLabel->setText(m_InstrumentNavigationData->GetName());
    }
  else
    {
    m_Controls.m_PointerNameLabel->setText("<not available>");
    }
}

void QmitkIGTTrackingLabView::OnObjectmarkerSelected()
{

if (m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().IsNotNull())
    {
    m_ObjectmarkerNavigationData = m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedToolID());
    }
  else
    {
    m_Controls.m_ObjectmarkerNameLabel->setText("<not available>");
    return;
    }

  if (m_ObjectmarkerNavigationData.IsNotNull())
    {
    m_Controls.m_ObjectmarkerNameLabel->setText(m_ObjectmarkerNavigationData->GetName());
    }
  else
    {
    m_Controls.m_ObjectmarkerNameLabel->setText("<not available>");
    }
}

void QmitkIGTTrackingLabView::OnSetupNavigation()
{
  MITK_INFO << "SetupNavigationCalled";
  if(m_Source.IsNotNull())
    if(m_Source->IsTracking())
      return;

  mitk::DataStorage* ds = this->GetDefaultDataStorage();
  if(ds == NULL)
  {
    QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "can not access DataStorage. Navigation not possible");
    return;
  }

  // Building up the filter pipeline
  try
  {
    this->SetupIGTPipeline();

  }
  catch(std::exception& e)
  {
    QMessageBox::warning(NULL, QString("IGTSurfaceTracker: Error"), QString("Error while building the IGT-Pipeline: %1").arg(e.what()));
    this->DestroyIGTPipeline(); // destroy the pipeline if building is incomplete
    return;
  }
  catch(...)
  {
    QMessageBox::warning(NULL, QString("IGTSurfaceTracker: Error"), QString("Error while building the IGT-Pipeline"));
    this->DestroyIGTPipeline();
    return;
  }
}

void QmitkIGTTrackingLabView::SetupIGTPipeline()
{
  this->InitializeRegistration(); //initializes the registration widget

  /*
  mitk::DataStorage* ds = this->GetDefaultDataStorage(); // check if DataStorage is available
  if(ds == NULL)
    throw std::invalid_argument("DataStorage is not available");

  //Get selected source
  m_Source = dynamic_cast<mitk::TrackingDeviceSource*>(m_Controls.m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().GetPointer());
  if (m_Source.IsNull()) {mitkThrow() << "Error: no tracking device";}


  this->InitializeFilters(); // initialize all needed filters


  for (unsigned int i=0; i < m_Source->GetNumberOfOutputs(); ++i)
  {
    m_FiducialRegistrationFilter->SetInput(i, m_Source->GetOutput(i)); // set input for registration filter
    m_Visualizer->SetInput(i, m_FiducialRegistrationFilter->GetOutput(i)); // set input for visualization filter
  }

  for(unsigned int i= 0; i < m_Visualizer->GetNumberOfOutputs(); ++i)
  {
    const char* toolName = m_Source->GetOutput(i)->GetName();

    mitk::DataNode::Pointer representation = this->CreateInstrumentVisualization(this->GetDefaultDataStorage(), toolName);
    m_PSRecToolSelectionComboBox->addItem(QString(toolName));

    m_VirtualViewToolSelectionWidget->AddToolName(QString(toolName));

    m_Visualizer->SetRepresentationObject(i, representation->GetData());

  }

  if(m_Source->GetTrackingDevice()->GetToolCount() > 0)
    m_RenderingTimerWidget->setEnabled(true);

  mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_ALL);
  this->GlobalReinit();

  */

}

void QmitkIGTTrackingLabView::InitializeFilters()
{
  //3. Virtual Camera
  //m_VirtualView = mitk::CameraVisualization::New(); // filter to update the vtk camera according to the reference navigation data
  //m_VirtualView->SetRenderer(mitk::BaseRenderer::GetInstance(this->GetActiveStdMultiWidget()->mitkWidget4->GetRenderWindow()));

  /*mitk::Vector3D viewUpInToolCoordinatesVector;
  viewUpInToolCoordinatesVector[0]=1;
  viewUpInToolCoordinatesVector[1]=0;
  viewUpInToolCoordinatesVector[2]=0;

  m_VirtualView->SetDirectionOfProjectionInToolCoordinates(m_DirectionOfProjectionVector);
  m_VirtualView->SetFocalLength(5000.0);
  m_VirtualView->SetViewUpInToolCoordinates(viewUpInToolCoordinatesVector);*/
}

void QmitkIGTTrackingLabView::OnRegisterFiducials( )
{
  /* filter pipeline can only be build, if source and visualization filters exist */
  /*
  if (m_Source.IsNull() || m_Visualizer.IsNull() || m_FiducialRegistrationFilter.IsNull())
  {
    QMessageBox::warning(NULL, "Registration not possible", "Navigation pipeline is not ready. Please (re)start the navigation");
    return;
  }
  if (m_Source->IsTracking() == false)
  {
    QMessageBox::warning(NULL, "Registration not possible", "Registration only possible if navigation is running");
    return;
  }
  */

  /* retrieve fiducials from data storage */
  mitk::DataStorage* ds = this->GetDefaultDataStorage();


  mitk::PointSet::Pointer imageFiducials = dynamic_cast<mitk::PointSet*>(m_ImageFiducialsDataNode->GetData());
  mitk::PointSet::Pointer trackerFiducials = dynamic_cast<mitk::PointSet*>(m_TrackerFiducialsDataNode->GetData());

  //mitk::PointSet::Pointer imageFiducials = ds->GetNamedObject<mitk::PointSet>(m_RegistrationImageFiducialsName.c_str());
  //mitk::PointSet::Pointer trackerFiducials = ds->GetNamedObject<mitk::PointSet>(m_RegistrationTrackingFiducialsName.c_str());
  if (imageFiducials.IsNull() || trackerFiducials.IsNull())
  {
    QMessageBox::warning(NULL, "Registration not possible", "Fiducial data objects not found. \n"
      "Please set 3 or more fiducials in the image and with the tracking system.\n\n"
      "Registration is not possible");
    return;
  }

  unsigned int minFiducialCount = 3; // \Todo: move to view option
  if ((imageFiducials->GetSize() < minFiducialCount) || (trackerFiducials->GetSize() < minFiducialCount) || (imageFiducials->GetSize() != trackerFiducials->GetSize()))
  {
    QMessageBox::warning(NULL, "Registration not possible", QString("Not enough fiducial pairs found. At least %1 fiducial must "
      "exist for the image and the tracking system respectively.\n"
      "Currently, %2 fiducials exist for the image, %3 fiducials exist for the tracking system").arg(minFiducialCount).arg(imageFiducials->GetSize()).arg(trackerFiducials->GetSize()));
    return;
  }

  //convert point sets to vtk poly data

  vtkSmartPointer<vtkPoints> sourcePoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> targetPoints = vtkSmartPointer<vtkPoints>::New();

  for (int i=0; i<imageFiducials->GetSize(); i++)
    {
    double point[3] = {imageFiducials->GetPoint(i)[0],imageFiducials->GetPoint(i)[1],imageFiducials->GetPoint(i)[2]};
    sourcePoints->InsertNextPoint(point);

    double point_targets[3] = {trackerFiducials->GetPoint(i)[0],trackerFiducials->GetPoint(i)[1],trackerFiducials->GetPoint(i)[2]};
    targetPoints->InsertNextPoint(point_targets);
    }

  //compute transform
  vtkSmartPointer<vtkLandmarkTransform> transform = vtkSmartPointer<vtkLandmarkTransform>::New();
  transform->SetSourceLandmarks(sourcePoints);
  transform->SetTargetLandmarks(targetPoints);
  transform->Modified();
  transform->Update();

  //convert from vtk to itk data types
  itk::Matrix<float,3,3> rotationFloat = itk::Matrix<float,3,3>();
  itk::Vector<float,3> translationFloat = itk::Vector<float,3>();

  vtkSmartPointer<vtkMatrix4x4> m = transform->GetMatrix();
  for(int k=0; k<3; k++) for(int l=0; l<3; l++)
  {
    rotationFloat[k][l] = m->GetElement(k,l);

  }
  for(int k=0; k<3; k++)
  {
    translationFloat[k] = m->GetElement(k,3);
  }

  //create new transform object
  mitk::AffineTransform3D::Pointer newTransform = mitk::AffineTransform3D::New();
  newTransform->SetMatrix(rotationFloat);
  newTransform->SetOffset(translationFloat);

  m_Controls.m_ObjectComboBox->GetSelectedNode()->GetData()->GetGeometry()->SetIndexToWorldTransform(newTransform);

/*
    if (m_FiducialRegistrationFilter.IsNotNull() && m_FiducialRegistrationFilter->IsInitialized()) // update registration quality display
    {
      QString registrationQuality = QString("%0: FRE is %1mm (Std.Dev. %2), \n"
        "RMS error is %3mm,\n"
        "Minimum registration error (best fitting landmark) is  %4mm,\n"
        "Maximum registration error (worst fitting landmark) is %5mm.")
        .arg("Fiducial Registration")
        .arg(m_FiducialRegistrationFilter->GetFRE(), 3, 'f', 3)
        .arg(m_FiducialRegistrationFilter->GetFREStdDev(), 3, 'f', 3)
        .arg(m_FiducialRegistrationFilter->GetRMSError(), 3, 'f', 3)
        .arg(m_FiducialRegistrationFilter->GetMinError(), 3, 'f', 3)
        .arg(m_FiducialRegistrationFilter->GetMaxError(), 3, 'f', 3);
      m_RegistrationWidget->SetQualityDisplayText(registrationQuality);
    }
    */







  //trackerFiducials->Clear();
  //this->GlobalReinit();
}


void QmitkIGTTrackingLabView::OnTrackerDisconnected()
{
  m_RenderingTimerWidget->DisableWidget();
  this->DestroyInstrumentVisualization(this->GetDefaultDataStorage(), m_NDIConfigWidget->GetTracker());
}


mitk::DataNode::Pointer QmitkIGTTrackingLabView::CreateInstrumentVisualization(mitk::DataStorage* ds, const char* toolName)
{
    //const char* toolName = tracker->GetTool(i)->GetToolName();
    mitk::DataNode::Pointer toolRepresentationNode;
    toolRepresentationNode = ds->GetNamedNode(toolName); // check if node with same name already exists

    if(toolRepresentationNode.IsNotNull())
      ds->Remove(toolRepresentationNode); // remove old node with same name

    toolRepresentationNode = this->CreateConeRepresentation( toolName );
  //  m_Visualizer->SetRepresentationObject(i, toolRepresentationNode->GetData());

    ds->Add(toolRepresentationNode); // adds node to data storage

    return toolRepresentationNode;
}


mitk::DataNode::Pointer QmitkIGTTrackingLabView::CreateConeRepresentation( const char* label )
{

  //new data
  mitk::Cone::Pointer activeToolData = mitk::Cone::New();
  vtkConeSource* vtkData = vtkConeSource::New();

  vtkData->SetRadius(7.5);
  vtkData->SetHeight(15.0);
  vtkData->SetDirection(m_DirectionOfProjectionVector[0],m_DirectionOfProjectionVector[1],m_DirectionOfProjectionVector[2]);
  vtkData->SetCenter(0.0, 0.0, 7.5);
  vtkData->SetResolution(20);
  vtkData->CappingOn();
  vtkData->Update();
  activeToolData->SetVtkPolyData(vtkData->GetOutput());
  vtkData->Delete();

  //new node
  mitk::DataNode::Pointer coneNode = mitk::DataNode::New();
  coneNode->SetData(activeToolData);
  coneNode->GetPropertyList()->SetProperty("name", mitk::StringProperty::New( label ));
  coneNode->GetPropertyList()->SetProperty("layer", mitk::IntProperty::New(0));
  coneNode->GetPropertyList()->SetProperty("visible", mitk::BoolProperty::New(true));
  coneNode->SetColor(1.0,0.0,0.0);
  coneNode->SetOpacity(0.85);
  coneNode->Modified();

  return coneNode;
}

void QmitkIGTTrackingLabView::DestroyIGTPipeline()
{
  if(m_Source.IsNotNull())
  {
    m_Source->StopTracking();
    m_Source->Disconnect();
    m_Source = NULL;
  }
  m_PermanentRegistrationFilter = NULL;
  m_Visualizer = NULL;
  m_VirtualView = NULL;
}

void QmitkIGTTrackingLabView::OnChangeToolName(int index, QString name)
{
    if(m_Source.IsNull())
      return;

    mitk::DataStorage* ds = this->GetDefaultDataStorage();
    if(ds == NULL)
    {
      QMessageBox::warning(NULL,"DataStorage Access Error", "Could not access DataStorage. Tool Name can not be changed!");
      return;
    }

    mitk::NavigationData::Pointer tempND = m_Source->GetOutput(index);
    if(tempND.IsNull())
      return;

    const char* oldName = tempND->GetName();

    mitk::DataNode::Pointer tempNode = ds->GetNamedNode(oldName);

    if(tempNode.IsNotNull())
    {
      tempNode->SetName(name.toStdString().c_str());
      tempND->SetName(name.toStdString().c_str());
    }
    else
      QMessageBox::warning(NULL, "Rename Tool Error", "Couldn't find the corresponding tool for changing it's name!");
}

void QmitkIGTTrackingLabView::OnToolLoaded(int index, mitk::DataNode::Pointer toolNode)
{
  if(m_Source.IsNull() || m_Visualizer.IsNull())
    return;

  mitk::DataStorage* ds = this->GetDefaultDataStorage();
  if(ds == NULL)
  {
    QMessageBox::warning(NULL,"DataStorage Access Error", "Could not access DataStorage. Loaded tool representation can not be shown!");
    return;
  }

  mitk::NavigationData::Pointer tempND = m_Source->GetOutput(index);
  if(tempND.IsNull())
    return;

  // try to find DataNode for tool in DataStorage
  const char* toolName = tempND->GetName();
  mitk::DataNode::Pointer tempNode = ds->GetNamedNode(toolName);

  if(tempNode.IsNull())
  {
    tempNode = mitk::DataNode::New();  // create new node, if none was found
    ds->Add(tempNode);
  }

  tempNode->SetData(toolNode->GetData());
  tempNode->SetName(toolNode->GetName());

  m_PSRecToolSelectionComboBox->setItemText(index,toolNode->GetName().c_str());

  m_VirtualViewToolSelectionWidget->ChangeToolName(index, QString(toolNode->GetName().c_str()));

  m_Visualizer->SetRepresentationObject(index, tempNode->GetData());
  m_Visualizer->Update();

  tempNode->Modified();
  this->GlobalReinit();
}

void QmitkIGTTrackingLabView::OnStartNavigation()
{
MITK_INFO << "1";

  if(m_Source.IsNull())
  {
    QMessageBox::warning(NULL, "IGTTrackingLab: Error", "can not access tracking source. Navigation not possible");
    return;
  }

  MITK_INFO << "2";
  if(!m_Source->IsTracking())
  {
    m_Source->StartTracking();

    try
    {
      //m_RenderingTimerWidget->GetTimerInterval();
      this->StartContinuousUpdate(); // start tracker with set interval
      MITK_INFO << "3";
      for(unsigned int i = 0; i < m_Source->GetNumberOfOutputs(); i++)  // add navigation data to bundle widgets
      {
        m_ToolStatusWidget->AddNavigationData(dynamic_cast<mitk::NavigationData*>(m_Source->GetOutputs().at(i).GetPointer()));
      }

      m_ToolStatusWidget->ShowStatusLabels(); // show status for every tool if ND is valid or not
      //m_IGTPlayerWidget->setEnabled(true);
      MITK_INFO << "4";
    }
    catch(...)
    {
      MITK_INFO << "Excpetion";
      //m_IGTPlayerWidget->setDisabled(true);
      this->StopContinuousUpdate();
      this->DestroyIGTPipeline();
      return;

    }

    MITK_INFO << "5";
  }
}


void QmitkIGTTrackingLabView::StopContinuousUpdate()
{
  if (this->m_RenderingTimerWidget->GetUpdateTimer() != NULL)
  {
    m_RenderingTimerWidget->StopTimer();
    disconnect( (QTimer*) m_RenderingTimerWidget->GetUpdateTimer(), SIGNAL(timeout()), this, SLOT(RenderScene()) ); // disconnect timer from RenderScene() method
  }

  if(m_PointSetRecordPushButton)
    m_PointSetRecordPushButton->setDisabled(true);
}

void QmitkIGTTrackingLabView::RenderScene( )
{
  try
  {
    if (m_Visualizer.IsNull() || this->GetActiveStdMultiWidget() == NULL)
      return;
    try
    {
      if(m_Source.IsNotNull() && m_Source->IsTracking())
       m_ToolStatusWidget->Refresh();



      if(m_VirtualViewToolSelectionWidget->IsSelectedToolActivated())
      {
        m_VirtualView->Update();
        mitk::Point3D p = m_Visualizer->GetOutput(m_VirtualViewToolSelectionWidget->GetCurrentSelectedIndex())->GetPosition();
        this->GetActiveStdMultiWidget()->MoveCrossToPosition(p);
      }

      mitk::NavigationData::Pointer permRegTool = this->m_ObjectmarkerNavigationData;



      if(m_PointSetRecording && m_PSRecordingPointSet.IsNotNull())
      {
        int size = m_PSRecordingPointSet->GetSize();
        mitk::NavigationData::Pointer nd= m_Visualizer->GetOutput(m_PSRecToolSelectionComboBox->currentIndex());

        if(size > 0)
        {
          mitk::Point3D p = m_PSRecordingPointSet->GetPoint(size-1);
          if(p.EuclideanDistanceTo(nd->GetPosition()) > (double) m_PSRecordingSpinBox->value())
            m_PSRecordingPointSet->InsertPoint(size, nd->GetPosition());
        }
        else
          m_PSRecordingPointSet->InsertPoint(size, nd->GetPosition());
      }
    }
    catch(std::exception& e)
    {
      MITK_WARN << "Exception during QmitkIGTTrackingLab::RenderScene():" << e.what() << "\n";
    }

    //if(m_VirtualViewCheckBox->isChecked())
    //  mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_ALL);
    ////update all Widgets
    //else

    m_Visualizer->Update();

      mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_ALL);


  }
  catch (std::exception& e)
  {
    MITK_WARN << "RenderAll exception: " << e.what() << "\n";
  }
  catch (...)
  {
    MITK_WARN << "RenderAll unknown exception\n";
  }
}

void QmitkIGTTrackingLabView::StartContinuousUpdate( )
{
  if (m_Source.IsNull() || m_Visualizer.IsNull() )
    throw std::invalid_argument("Pipeline is not set up correctly");

  if (m_RenderingTimerWidget->GetUpdateTimer() == NULL)
    return;

  else
  {
    connect( (QTimer*) m_RenderingTimerWidget->GetUpdateTimer(), SIGNAL(timeout()), this, SLOT(RenderScene()) ); // connect update timer to RenderScene() method
  }

  if(m_PointSetRecordPushButton)
    m_PointSetRecordPushButton->setEnabled(true);
}



void QmitkIGTTrackingLabView::OnStopNavigation()
{
  if(m_Source.IsNull())
  {
    QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "can not access tracking source. Navigation not possible");
    return;
  }
  if(m_Source->IsTracking())
  {
    m_Source->StopTracking();
    this->StopContinuousUpdate();
    m_ToolStatusWidget->RemoveStatusLabels();

    m_NDIConfigWidget->EnableAddToolsButton(true);
  }
}

void QmitkIGTTrackingLabView::InitializeRegistration()
{
  mitk::DataStorage* ds = this->GetDefaultDataStorage();
  if( ds == NULL )
    return;


  m_RegistrationWidget->SetMultiWidget(this->GetActiveStdMultiWidget()); // passing multiwidget to pointsetwidget

  if(m_ImageFiducialsDataNode.IsNull())
  {
    m_ImageFiducialsDataNode = mitk::DataNode::New();
    mitk::PointSet::Pointer ifPS = mitk::PointSet::New();

    m_ImageFiducialsDataNode->SetData(ifPS);

    mitk::Color color;
    color.Set(1.0f, 0.0f, 0.0f);
    m_ImageFiducialsDataNode->SetName(m_RegistrationImageFiducialsName);
    m_ImageFiducialsDataNode->SetColor(color);
    m_ImageFiducialsDataNode->SetBoolProperty( "updateDataOnRender", false );

    ds->Add(m_ImageFiducialsDataNode);
  }
  m_RegistrationWidget->SetMultiWidget(this->GetActiveStdMultiWidget());
  m_RegistrationWidget->SetImageFiducialsNode(m_ImageFiducialsDataNode);

  if(m_TrackerFiducialsDataNode.IsNull())
  {
    m_TrackerFiducialsDataNode = mitk::DataNode::New();
    mitk::PointSet::Pointer tfPS = mitk::PointSet::New();
    m_TrackerFiducialsDataNode->SetData(tfPS);

    mitk::Color color;
    color.Set(0.0f, 1.0f, 0.0f);
    m_TrackerFiducialsDataNode->SetName(m_RegistrationTrackingFiducialsName);
    m_TrackerFiducialsDataNode->SetColor(color);
    m_TrackerFiducialsDataNode->SetBoolProperty( "updateDataOnRender", false );

    ds->Add(m_TrackerFiducialsDataNode);
  }

  m_RegistrationWidget->SetTrackerFiducialsNode(m_TrackerFiducialsDataNode);
}


void QmitkIGTTrackingLabView::OnToolBoxCurrentChanged(const int index)
{
  switch (index)
  {
  case RegistrationWidget:
    this->InitializeRegistration();
    break;

  default:
    break;
  }
}


mitk::DataNode::Pointer QmitkIGTTrackingLabView::CreateRegistrationFiducialsNode( const std::string& label, const mitk::Color& color)
{
  mitk::DataNode::Pointer fiducialsNode = mitk::DataNode::New();
  mitk::PointSet::Pointer fiducialsPointSet = mitk::PointSet::New();

  fiducialsNode->SetData(fiducialsPointSet);
  fiducialsNode->SetName( label );
  fiducialsNode->SetColor( color );
  fiducialsNode->SetBoolProperty( "updateDataOnRender", false );

  return fiducialsNode;
}


void QmitkIGTTrackingLabView::ChangeToolRepresentation( int toolID , mitk::Surface::Pointer surface )
{
  mitk::DataStorage* ds = this->GetDefaultDataStorage();
  if(ds == NULL)
  {
    QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access DataStorage. Changing tool representation not possible!");
    return;
  }

  mitk::TrackingDevice::Pointer tracker = m_NDIConfigWidget->GetTracker();
  if(tracker.IsNull())
  {
    QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access Tracker. Changing tool representation not possible!");
    return;
  }

  try
  {
    const char* name = tracker->GetTool(toolID)->GetToolName();   // get tool name by id

    mitk::DataNode::Pointer toolNode = ds->GetNamedNode(name);

    if(toolNode.IsNull())
      return;

    toolNode->SetData(surface); // change surface representation of node
    toolNode->SetColor(0.45,0.70,0.85); //light blue like old 5D sensors
    toolNode->Modified();

    m_Visualizer->SetRepresentationObject( toolID, toolNode->GetData()); // updating node with changed surface back in visualizer

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  }
  catch(std::exception& e)
  {
    QMessageBox::warning(NULL, QString("IGTSurfaceTracker: Error"), QString("Can not change tool representation!").arg(e.what()));
    return;
  }
}


QWidget* QmitkIGTTrackingLabView::CreatePointSetRecordingWidget(QWidget* parent)
{
  QWidget* pointSetRecordingWidget = new QWidget(parent);
  m_PSRecToolSelectionComboBox = new QComboBox(pointSetRecordingWidget);
  m_PSRecordingSpinBox = new QSpinBox(pointSetRecordingWidget);
  QLabel* psRecordingEpsilonDistance = new QLabel("mm (point distance)", pointSetRecordingWidget);

  // the recording button
  m_PointSetRecordPushButton = new QPushButton("Start PointSet Recording", pointSetRecordingWidget);
  m_PointSetRecordPushButton->setDisabled(true);
  m_PointSetRecordPushButton->setIcon(QIcon(":/QmitkQmitkIGTTrackingLabView/start_rec.png"));
  m_PointSetRecordPushButton->setCheckable(true);
  connect( m_PointSetRecordPushButton, SIGNAL(toggled(bool)), this, SLOT(OnPointSetRecording(bool)) );

  // distances spin
  m_PSRecordingSpinBox->setValue(1);
  m_PSRecordingSpinBox->setMinimum(1);
  m_PSRecordingSpinBox->setMaximum(20);

  QLabel* toolSelectLabel = new QLabel("Select tool for recording:", pointSetRecordingWidget);
  QGridLayout* layout = new QGridLayout(pointSetRecordingWidget);

  int row = 0;
  int col = 0;

  layout->addWidget(toolSelectLabel,row,col++,1,1,Qt::AlignRight);
  layout->addWidget(m_PSRecToolSelectionComboBox,row,col++,1,3,Qt::AlignLeft);
  col +=2;
  layout->addWidget(m_PSRecordingSpinBox,row,col++,1,1,Qt::AlignRight);
  layout->addWidget(psRecordingEpsilonDistance, row, col++,1,1,Qt::AlignLeft);

  row++;
  col=4;

  layout->addWidget(m_PointSetRecordPushButton,row,col++,1,2,Qt::AlignRight);

  return pointSetRecordingWidget;
}

void QmitkIGTTrackingLabView::OnPointSetRecording(bool record)
{
  mitk::DataStorage* ds = this->GetDefaultDataStorage();

  if(ds == NULL)
    return;

  if(record)
  {
    mitk::DataNode::Pointer psRecND = ds->GetNamedNode(m_PointSetRecordingDataNodeName);
    if(m_PSRecordingPointSet.IsNull() || psRecND.IsNull())
    {
      m_PSRecordingPointSet = NULL;
      m_PSRecordingPointSet = mitk::PointSet::New();
      mitk::DataNode::Pointer dn = mitk::DataNode::New();
      dn->SetName(m_PointSetRecordingDataNodeName);
      dn->SetColor(0.,1.,0.);
      dn->SetData(m_PSRecordingPointSet);
      ds->Add(dn);
    }
    else
      m_PSRecordingPointSet->Clear();

    m_PointSetRecording = true;
    m_PointSetRecordPushButton->setText("Stop PointSet Recording");
    m_PSRecToolSelectionComboBox->setDisabled(true);
  }

  else
  {
    m_PointSetRecording = false;
    m_PointSetRecordPushButton->setText("Start PointSet Recording");
    m_PSRecToolSelectionComboBox->setEnabled(true);
  }
}

void QmitkIGTTrackingLabView::DestroyInstrumentVisualization(mitk::DataStorage* ds, mitk::TrackingDevice::Pointer tracker)
{
  if(ds == NULL || tracker.IsNull())
    return;

  for(int i=0; i < tracker->GetToolCount(); ++i)
  {
    mitk::DataNode::Pointer dn = ds->GetNamedNode(tracker->GetTool(i)->GetToolName());

    if(dn.IsNotNull())
      ds->Remove(dn);
  }
}


void QmitkIGTTrackingLabView::GlobalReinit()
{
  // request global reiinit
  mitk::NodePredicateNot::Pointer pred = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("includeInBoundingBox", mitk::BoolProperty::New(false)));
  mitk::DataStorage::SetOfObjects::ConstPointer rs = this->GetDataStorage()->GetSubset(pred);

  // calculate bounding geometry of these nodes
  mitk::TimeSlicedGeometry::Pointer bounds = this->GetDataStorage()->ComputeBoundingGeometry3D(rs, "visible");

  // initialize the views to the bounding geometry
  mitk::RenderingManager::GetInstance()->InitializeViews(bounds);

  //global reinit end
}

void QmitkIGTTrackingLabView::OnVirtualCamera(int toolNr, bool on)
{
   if(m_VirtualView.IsNull())
    return;

   if(on)
   {
     //m_VirtualView->SetInput(m_FiducialRegistrationFilter->GetOutput(toolNr));
     this->GetActiveStdMultiWidget()->SetWidgetPlaneModeToRotation(true);
   }
   else
     this->GetActiveStdMultiWidget()->SetWidgetPlaneModeToSlicing(true);

}


void QmitkIGTTrackingLabView::OnPermanentRegistration(bool on)
{
  MITK_INFO << "Permanent registration" << on;
  if(on)
    {

    m_PermanentRegistrationFilter = mitk::NavigationDataObjectVisualizationFilter::New();

    //connect filter to source
    m_PermanentRegistrationFilter->SetInput(this->m_ObjectmarkerNavigationData);

    mitk::AffineTransform3D::Pointer initialTransform = this->m_Controls.m_ObjectComboBox->GetSelectedNode()->GetData()->GetGeometry()->GetIndexToWorldTransform();

    m_PermanentRegistrationFilter->SetRepresentationObject(0,this->m_Controls.m_ObjectComboBox->GetSelectedNode()->GetData());

    itk::Matrix<float,3,3> OffsetR = itk::Matrix<float,3,3>(this->m_ObjectmarkerNavigationData->GetOrientation().rotation_matrix_transpose());
    itk::Vector<float,3> OffsetT = itk::Vector<float,3>();
    OffsetT[0] = this->m_ObjectmarkerNavigationData->GetPosition()[0];
    OffsetT[1] = this->m_ObjectmarkerNavigationData->GetPosition()[1];
    OffsetT[2] = this->m_ObjectmarkerNavigationData->GetPosition()[2];

    MITK_INFO << "Offset R:" << OffsetR;
    MITK_INFO << "Orientation Quat:" << m_ObjectmarkerNavigationData->GetOrientation();

    OffsetR = OffsetR.GetInverse();
    OffsetT = (OffsetR * OffsetT) * -1.;


    /*itk::Vector<float,3> negativeTranslation;
    mitk::Point3D Translation = this->m_ObjectmarkerNavigationData->GetPosition();
    negativeTranslation[0] = Translation[0];
    negativeTranslation[1] = Translation[1];
    negativeTranslation[2] = Translation[2];*/



    mitk::AffineTransform3D::Pointer offset = mitk::AffineTransform3D::New();
    offset->SetMatrix(OffsetR);
    offset->SetOffset(OffsetT);

    mitk::AffineTransform3D::Pointer overall_transform = mitk::AffineTransform3D::New();
    overall_transform->SetIdentity();
    overall_transform->Compose(initialTransform);
    overall_transform->Compose(offset);

    m_PermanentRegistrationFilter->SetOffset(0,overall_transform);

    //start timer
    m_Timer->start(30);
    }
  else
    {
    //stop timer
      m_Timer->stop();
    //delete filter
    m_PermanentRegistrationFilter = NULL;
    }


}

mitk::PointSet::Pointer QmitkIGTTrackingLabView::GetVirtualPointSetFromPosition(mitk::NavigationData::Pointer navigationData)
{
  typedef itk::QuaternionRigidTransform<double> QuaternionTransformType;

  mitk::NavigationData::PositionType pointA;
  mitk::NavigationData::PositionType pointB;
  mitk::NavigationData::PositionType pointC;

  //initializing three points with position(0|0|0)
  pointA.Fill(0);
  pointB.Fill(0);
  pointC.Fill(0);

  // changing position off all points in order to make them orthogonal
  pointA[0] = 1;
  pointB[1] = 1;
  pointC[2] = 1;

  QuaternionTransformType::Pointer quatTransform = QuaternionTransformType::New();

  // orientation of NavigationData from parameter
  mitk::NavigationData::OrientationType quatIn = navigationData->GetOrientation();

  // set orientation to quaternion transform
  vnl_quaternion<double> const vnlQuatIn(quatIn.x(), quatIn.y(), quatIn.z(), quatIn.r());
  quatTransform->SetRotation(vnlQuatIn);

  // transform each point
  pointA = quatTransform->TransformPoint(pointA);
  pointB = quatTransform->TransformPoint(pointB);
  pointC = quatTransform->TransformPoint(pointC);

  // add position data from NavigationData parameter to each point
  pointA[0] += navigationData->GetPosition()[0];
  pointA[1] += navigationData->GetPosition()[1];
  pointA[2] += navigationData->GetPosition()[2];

  pointB[0] += navigationData->GetPosition()[0];
  pointB[1] += navigationData->GetPosition()[1];
  pointB[2] += navigationData->GetPosition()[2];

  pointC[0] += navigationData->GetPosition()[0];
  pointC[1] += navigationData->GetPosition()[1];
  pointC[2] += navigationData->GetPosition()[2];

  // insert points in source points pointset for the permanent registration landmark transform
  m_PermanentRegistrationSourcePoints->InsertPoint(0,pointA);
  m_PermanentRegistrationSourcePoints->InsertPoint(1,pointB);
  m_PermanentRegistrationSourcePoints->InsertPoint(2,pointC);


  return m_PermanentRegistrationSourcePoints;
}

