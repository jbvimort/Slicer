/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Need to be included before qMRMLVRView_p
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRCamera.h>

#include "qMRMLVRView_p.h"

// Qt includes
#include <QCoreApplication>
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>

// CTK includes
#include <ctkAxesWidget.h>
#include <ctkPimpl.h>

// qMRML includes
#include "qMRMLColors.h"
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "vtkSlicerConfigure.h" // For Slicer_USE_OpenVR

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractDisplayableManager.h>
#include <vtkMRMLDisplayableManagerGroup.h>
#include <vtkMRMLVRViewDisplayableManagerFactory.h>
#include <vtkThreeDViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLVRViewNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLCameraNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkCullerCollection.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include "qSlicerApplication.h"

//--------------------------------------------------------------------------
// qMRMLVRViewPrivate methods

//---------------------------------------------------------------------------
qMRMLVRViewPrivate::qMRMLVRViewPrivate(qMRMLVRView& object)
  : q_ptr(&object)
{
  this->DisplayableManagerGroup = 0;
  this->MRMLScene = 0;
  this->MRMLVRViewNode = 0;

//#ifdef Slicer_USE_OpenVR
  this->Renderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
  this->RenderWindow = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
  this->Interactor = vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();
  this->Camera = vtkSmartPointer<vtkOpenVRCamera>::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->RenderWindow->SetInteractor(this->Interactor);
  this->Renderer->SetActiveCamera(this->Camera);
//#endif

  //this->FO = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
}

//---------------------------------------------------------------------------
qMRMLVRViewPrivate::~qMRMLVRViewPrivate()
{
  if (this->DisplayableManagerGroup)
    {
    this->DisplayableManagerGroup->Delete();
    }
}

//---------------------------------------------------------------------------
void qMRMLVRViewPrivate::init()
{
  Q_Q(qMRMLVRView);
  q->setRenderEnabled(this->MRMLScene != 0);

  q->setLayout(new QVBoxLayout);
  q->layout()->setMargin(0);
  q->layout()->setSpacing(0);

  QPushButton* startVRButton = new QPushButton("Start VR");
  q->layout()->addWidget(startVRButton);
  QObject::connect(startVRButton, SIGNAL(clicked()), this, SLOT(startVR()));


  QPushButton* stopVRbutton = new QPushButton("Stop VR");
  q->layout()->addWidget(stopVRbutton);
  QObject::connect(stopVRbutton, SIGNAL(clicked()), this, SLOT(stopVR()));

  //vtkNew<vtkThreeDViewInteractorStyle> interactorStyle;
  //q->interactor()->SetInteractorStyle(interactorStyle.GetPointer());

  // Set default background color
  //q->setBackgroundColor(QColor::fromRgbF(
  //  vtkMRMLVRViewNode::defaultBackgroundColor()[0],
  //  vtkMRMLVRViewNode::defaultBackgroundColor()[1],
  //  vtkMRMLVRViewNode::defaultBackgroundColor()[2]));

  //q->setGradientBackground(true);

  //// Hide orientation widget
  //q->setOrientationWidgetVisible(false);

  //q->setZoomFactor(0.05);

  //q->setPitchDirection(ctkVTKRenderView::PitchUp);
  //q->setRollDirection(ctkVTKRenderView::RollRight);
  //q->setYawDirection(ctkVTKRenderView::YawLeft);

  this->initDisplayableManagers();
}

//----------------------------------------------------------------------------
void qMRMLVRViewPrivate::startVR()
{
  qDebug() << "startVR";
  this->VRLoopTimer.start(0);
}

//----------------------------------------------------------------------------
void qMRMLVRViewPrivate::stopVR()
{
  qDebug() << "stopVR";
  this->VRLoopTimer.stop();
}

//----------------------------------------------------------------------------
CTK_SET_CPP(qMRMLVRView, bool, setRenderEnabled, RenderEnabled);
CTK_GET_CPP(qMRMLVRView, bool, renderEnabled, RenderEnabled);

//----------------------------------------------------------------------------
CTK_GET_CPP(qMRMLVRView, vtkOpenVRRenderer*, renderer, Renderer);

//----------------------------------------------------------------------------
CTK_GET_CPP(qMRMLVRView, vtkOpenVRRenderWindow*, renderWindow, RenderWindow);

//----------------------------------------------------------------------------
CTK_GET_CPP(qMRMLVRView, vtkOpenVRRenderWindowInteractor*, interactor, Interactor);

//---------------------------------------------------------------------------
void qMRMLVRViewPrivate::initDisplayableManagers()
{
  Q_Q(qMRMLVRView);
  vtkMRMLVRViewDisplayableManagerFactory* factory
    = vtkMRMLVRViewDisplayableManagerFactory::GetInstance();

  this->RenderWindow->SetMultiSamples(0);

  QStringList displayableManagers;
  displayableManagers //<< "vtkMRMLCameraDisplayableManager"
                      //<< "vtkMRMLViewDisplayableManager"
                      << "vtkMRMLModelDisplayableManager"
                      << "vtkMRMLThreeDReformatDisplayableManager"
                      << "vtkMRMLCrosshairDisplayableManager3D"
                      << "vtkMRMLOrientationMarkerDisplayableManager"
                      << "vtkMRMLRulerDisplayableManager"
	  ;
  foreach(const QString& displayableManager, displayableManagers)
    {
    if(!factory->IsDisplayableManagerRegistered(displayableManager.toLatin1()))
      {
      factory->RegisterDisplayableManager(displayableManager.toLatin1());
      }
    }

  this->DisplayableManagerGroup
    = factory->InstantiateDisplayableManagers(q->renderer());

  qDebug() << "this->DisplayableManagerGroup" << this->DisplayableManagerGroup->GetDisplayableManagerCount();

  // Observe displayable manager group to catch RequestRender events

  //this->qvtkConnect(this->DisplayableManagerGroup, vtkCommand::UpdateEvent,
  //                  q, SLOT(scheduleRender()));

  ///CONFIGURATION OF THE OPENVR ENVIRONEMENT

  this->Renderer->RemoveCuller(this->Renderer->GetCullers()->GetLastItem());
  this->Renderer->SetBackground(0.7, 0.7, 0.7);

  // create 4 lights for even lighting
  {
    vtkNew<vtkLight> light;
    light->SetPosition(0.0, 1.0, 0.0);
    light->SetIntensity(1.0);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light.GetPointer());
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(0.8, -0.2, 0.0);
    light->SetIntensity(0.8);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light.GetPointer());
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(-0.3, -0.2, 0.7);
    light->SetIntensity(0.6);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light.GetPointer());
  }
  {
    vtkNew<vtkLight> light;
    light->SetPosition(-0.3, -0.2, -0.7);
    light->SetIntensity(0.4);
    light->SetLightTypeToSceneLight();
    this->Renderer->AddLight(light.GetPointer());
  }

  this->Renderer->SetBackground(0, 0, 1);

  vtkRenderWindow* refRenderWindow = qSlicerApplication::application()->layoutManager()->threeDWidget(0)->threeDView()->renderWindow();
  this->RenderWindow->InitializeViewFromCamera(refRenderWindow->GetRenderers()->GetFirstRenderer()->GetActiveCamera());

  this->Renderer->SetGradientBackground(1);
  this->Renderer->SetBackground2(0.7, 0.7, 0.7);
  this->Renderer->SetBackground(0, 0, 1);

  this->RenderWindow->Initialize();
  if (!this->RenderWindow->GetHMD())
    {
    qWarning() << "Failed to initialize OpenVR RenderWindow";
    return;
    }
  QObject::connect(&this->VRLoopTimer, SIGNAL(timeout()), this, SLOT(doOpenVR()));
}

//---------------------------------------------------------------------------
void qMRMLVRViewPrivate::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_Q(qMRMLVRView);
  if (newScene == this->MRMLScene)
    {
    return;
    }

  this->qvtkReconnect(
    this->MRMLScene, newScene,
    vtkMRMLScene::StartBatchProcessEvent, this, SLOT(onSceneStartProcessing()));

  this->qvtkReconnect(
    this->MRMLScene, newScene,
    vtkMRMLScene::EndBatchProcessEvent, this, SLOT(onSceneEndProcessing()));

  this->MRMLScene = newScene;
  q->setRenderEnabled(
    this->MRMLScene != 0 && !this->MRMLScene->IsBatchProcessing());
}

//---------------------------------------------------------------------------
//vtkMRMLCameraNode* qMRMLVRViewPrivate::lookUpMRMLCameraNode(vtkMRMLVRViewNode* viewNode)
//{
//  Q_ASSERT(viewNode);
//
//  QString viewNodeID(viewNode->GetID());
//  Q_ASSERT(!viewNodeID.isEmpty());
//
//  std::vector<vtkMRMLNode*> cameraNodes;
//  int cameraNodeCount = this->MRMLScene->GetNodesByClass("vtkMRMLCameraNode", cameraNodes);
//  for (int n=0; n < cameraNodeCount; n++)
//    {
//    vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(cameraNodes[n]);
//    Q_ASSERT(cameraNode);
//    QString activeTag(cameraNode->GetActiveTag());
//    if (activeTag == viewNodeID)
//      {
//      Q_ASSERT(this->MRMLScene->GetNodeByID(cameraNode->GetID()));
//      return cameraNode;
//      }
//    }
//  return 0;
//}

// --------------------------------------------------------------------------
void qMRMLVRViewPrivate::onSceneStartProcessing()
{
  //qDebug() << "qMRMLVRViewPrivate::onSceneStartProcessing";
  Q_Q(qMRMLVRView);
  q->setRenderEnabled(false);
}

//
// --------------------------------------------------------------------------
void qMRMLVRViewPrivate::onSceneEndProcessing()
{
  //qDebug() << "qMRMLVRViewPrivate::onSceneImportedEvent";
  Q_Q(qMRMLVRView);
  q->setRenderEnabled(true);
}

// --------------------------------------------------------------------------
void qMRMLVRViewPrivate::updateWidgetFromMRML()
{
  Q_Q(qMRMLVRView);
  if (!this->MRMLVRViewNode)
    {
    return;
    }
  //q->setAnimationIntervalMs(this->MRMLVRViewNode->GetAnimationMs());
  //q->setPitchRollYawIncrement(this->MRMLVRViewNode->GetRotateDegrees());
  //q->setSpinIncrement(this->MRMLVRViewNode->GetSpinDegrees());
  //q->setRockIncrement(this->MRMLVRViewNode->GetRockCount());
  //q->setRockLength(this->MRMLVRViewNode->GetRockLength());

  //q->setSpinEnabled(this->MRMLVRViewNode->GetAnimationMode() == vtkMRMLVRViewNode::Spin);
  //q->setRockEnabled(this->MRMLVRViewNode->GetAnimationMode() == vtkMRMLVRViewNode::Rock);

  //q->setUseDepthPeeling(this->MRMLVRViewNode->GetUseDepthPeeling() != 0);
  //q->setFPSVisible(this->MRMLVRViewNode->GetFPSVisible() != 0);
}

// --------------------------------------------------------------------------
void qMRMLVRViewPrivate::doOpenVR()
{
  //vtkRenderWindow* srcRenderWindow = qSlicerApplication::application()->layoutManager()->threeDWidget(0)->threeDView()->renderWindow();

  //this->FO->SaveCurrentBindingsAndBuffers(GL_FRAMEBUFFER);

  //qDebug() << "doOpenVR";
  //this->RenderWindow->MakeCurrent();
  this->Interactor->DoOneEvent(this->RenderWindow, this->Renderer);

  //srcRenderWindow->MakeCurrent();
  //this->FO->RestorePreviousBindingsAndBuffers(GL_FRAMEBUFFER);
}

// --------------------------------------------------------------------------
// qMRMLVRView methods

// --------------------------------------------------------------------------
qMRMLVRView::qMRMLVRView(QWidget* _parent) : Superclass(_parent)
  , d_ptr(new qMRMLVRViewPrivate(*this))
{
  Q_D(qMRMLVRView);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLVRView::~qMRMLVRView()
{
}

//------------------------------------------------------------------------------
void qMRMLVRView::addDisplayableManager(const QString& displayableManagerName)
{
  Q_D(qMRMLVRView);
  vtkSmartPointer<vtkMRMLAbstractDisplayableManager> displayableManager;
  displayableManager.TakeReference(
    vtkMRMLDisplayableManagerGroup::InstantiateDisplayableManager(
      displayableManagerName.toLatin1()));
  d->DisplayableManagerGroup->AddDisplayableManager(displayableManager);
}



//------------------------------------------------------------------------------
//void qMRMLVRView::rotateToViewAxis(unsigned int axisId)
//{
//  vtkVRViewInteractorStyle* style =
//    vtkVRViewInteractorStyle::SafeDownCast(this->interactorStyle());
//  if (!style)
//    {
//    qCritical() << "qMRMLVRView::rotateToViewAxis: no valid interactor style.";
//    return;
//    }
//
//  vtkMRMLCameraNode* cam = style->GetCameraNode();
//  if (!cam)
//    {
//    qCritical() << "qMRMLVRView::rotateToViewAxis: can not retrieve camera node.";
//    return;
//    }
//
//  switch (axisId)
//    {
//  case 0:
//    cam->RotateTo(vtkMRMLCameraNode::Left);
//  case 1:
//    cam->RotateTo(vtkMRMLCameraNode::Right);
//  case 2:
//    cam->RotateTo(vtkMRMLCameraNode::Posterior);
//  case 3:
//    cam->RotateTo(vtkMRMLCameraNode::Anterior);
//  case 4:
//    cam->RotateTo(vtkMRMLCameraNode::Inferior);
//  case 5:
//    cam->RotateTo(vtkMRMLCameraNode::Superior);
//  default:
//    qWarning() << "qMRMLVRView::rotateToViewAxis: " << axisId
//               << " is not a valid axis id (0 to 5 : "
//               << "-X, +X, -Y, +Y, -Z, +Z).";
//    }
//}

//------------------------------------------------------------------------------
//void qMRMLVRView::rotateToViewAxis(const std::string& axisLabel)
//{
//  Q_D(qMRMLVRView);
//  if (!d->MRMLVRViewNode)
//    {
//    qCritical() << "qMRMLVRView::rotateToViewAxis: no valid view node.";
//    return;
//    }
//
//  for (int i = 0; i < vtkMRMLAbstractViewNode::AxisLabelsCount; ++i)
//    {
//    if (axisLabel == std::string(d->MRMLVRViewNode->GetAxisLabel(i)))
//      {
//      this->rotateToViewAxis(i);
//      return;
//      }
//    }
//  qWarning() << "qMRMLVRView::rotateToViewAxis: " << QString(axisLabel.c_str())
//              << "is not a valid axis label.";
//}

//------------------------------------------------------------------------------
//void qMRMLVRView
//::resetCamera(bool resetRotation, bool resetTranslation, bool resetDistance)
//{
//  vtkVRViewInteractorStyle* style =
//    vtkVRViewInteractorStyle::SafeDownCast(this->interactorStyle());
//  if (!style)
//    {
//    qCritical() << "qMRMLVRView::resetCamera: no valid interactor style.";
//    return;
//    }
//
//  vtkMRMLCameraNode* cam = style->GetCameraNode();
//  if (!cam)
//    {
//    qCritical() << "qMRMLVRView::resetCamera: can not retrieve camera node.";
//    return;
//    }
//
//  cam->Reset(resetRotation, resetTranslation, resetDistance, this->renderer());
//}

//------------------------------------------------------------------------------
void qMRMLVRView::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLVRView);
  d->setMRMLScene(newScene);

  if (d->MRMLVRViewNode && newScene != d->MRMLVRViewNode->GetScene())
    {
    this->setMRMLVRViewNode(0);
    }
}

//---------------------------------------------------------------------------
void qMRMLVRView::setMRMLVRViewNode(vtkMRMLVRViewNode* newViewNode)
{
  Q_D(qMRMLVRView);
  if (d->MRMLVRViewNode == newViewNode)
    {
    return;
    }


  d->qvtkReconnect(
    d->MRMLVRViewNode, newViewNode,
    vtkCommand::ModifiedEvent, d, SLOT(updateWidgetFromMRML()));

  d->MRMLVRViewNode = newViewNode;
  d->DisplayableManagerGroup->SetMRMLDisplayableNode(newViewNode);

  d->updateWidgetFromMRML();
  // Enable/disable widget
  this->setEnabled(newViewNode != 0);
}

//---------------------------------------------------------------------------
vtkMRMLVRViewNode* qMRMLVRView::mrmlVRViewNode()const
{
  Q_D(const qMRMLVRView);
  return d->MRMLVRViewNode;
}

// --------------------------------------------------------------------------
//void qMRMLVRView::lookFromViewAxis(const ctkAxesWidget::Axis& axis)
//{
//  Q_D(qMRMLVRView);
//  if (!d->MRMLVRViewNode)
//    {
//    qCritical() << "qMRMLVRView::lookFromViewAxis: no valid view node.";
//    return;
//    }
//  double fov = d->MRMLVRViewNode->GetFieldOfView();
//  Q_ASSERT(fov >= 0.0);
//  this->lookFromAxis(axis, fov);
//}

// --------------------------------------------------------------------------
//void qMRMLVRView::resetFocalPoint()
//{
//  Q_D(qMRMLVRView);
//
//  bool savedBoxVisibile = true;
//  bool savedAxisLabelVisible = true;
//  if (d->MRMLVRViewNode)
//    {
//    // Save current visiblity state of Box and AxisLabel
//    savedBoxVisibile = d->MRMLVRViewNode->GetBoxVisible();
//    savedAxisLabelVisible = d->MRMLVRViewNode->GetAxisLabelsVisible();
//
//    int wasModifying = d->MRMLVRViewNode->StartModify();
//    // Hide Box and AxisLabel so they don't get taken into account when computing
//    // the view boundaries
//    d->MRMLVRViewNode->SetBoxVisible(0);
//    d->MRMLVRViewNode->SetAxisLabelsVisible(0);
//    d->MRMLVRViewNode->EndModify(wasModifying);
//    }
//  // Superclass resets the camera.
//  this->Superclass::resetFocalPoint();
//
//  if (d->MRMLVRViewNode)
//    {
//    // Restore visibility state
//    int wasModifying = d->MRMLVRViewNode->StartModify();
//    d->MRMLVRViewNode->SetBoxVisible(savedBoxVisibile);
//    d->MRMLVRViewNode->SetAxisLabelsVisible(savedAxisLabelVisible);
//    d->MRMLVRViewNode->EndModify(wasModifying);
//    // Inform the displayable manager that the view is reset, so it can
//    // update the box/labels bounds.
//    d->MRMLVRViewNode->InvokeEvent(vtkMRMLVRViewNode::ResetFocalPointRequestedEvent);
//    }
//
//  if (this->renderer())
//    {
//    this->renderer()->ResetCameraClippingRange();
//    }
//}

//------------------------------------------------------------------------------
void qMRMLVRView::getDisplayableManagers(vtkCollection *displayableManagers)
{
  Q_D(qMRMLVRView);

  if (!displayableManagers || !d->DisplayableManagerGroup)
    {
    return;
    }
  int num = d->DisplayableManagerGroup->GetDisplayableManagerCount();
  for (int n = 0; n < num; n++)
    {
    displayableManagers->AddItem(d->DisplayableManagerGroup->GetNthDisplayableManager(n));
    }
}
