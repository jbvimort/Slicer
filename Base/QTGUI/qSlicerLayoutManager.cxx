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

// Qt includes
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QVariant>

// MRMLWidgets includes
#include <qMRMLSliceWidget.h>
#include <qMRMLSliceControllerWidget.h>

// SlicerQt includes
#include "qSlicerLayoutManager.h"
#include "qMRMLVRView.h"
#include "qSlicerApplication.h"
#ifdef Slicer_USE_PYTHONQT_WITH_TCL
#include "qSlicerPythonManager.h"
#endif

// qMRMLSlicer
#include <qMRMLLayoutManager_p.h>

// MRMLDisplayableManager includes
#include <vtkMRMLSliceViewDisplayableManagerFactory.h>
#include <vtkMRMLThreeDViewDisplayableManagerFactory.h>

// MRML includes
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLVRViewNode.h>

// VTK includes
#include <vtkInteractorObserver.h>
#include <vtkCornerAnnotation.h>


//------------------------------------------------------------------------------
qMRMLLayoutVRViewFactory::qMRMLLayoutVRViewFactory(QObject* parent)
  : qMRMLLayoutViewFactory(parent)
{
}

//------------------------------------------------------------------------------
QString qMRMLLayoutVRViewFactory::viewClassName()const
{
  return "vtkMRMLVRViewNode";
}

//------------------------------------------------------------------------------
QWidget* qMRMLLayoutVRViewFactory
::createViewFromNode(vtkMRMLAbstractViewNode* viewNode)
{
  if (!viewNode || !this->layoutManager() || !this->layoutManager()->viewport())
  {
    Q_ASSERT(viewNode);
    return 0;
  }

  // There must be a unique VRWidget per node
  Q_ASSERT(!this->viewWidget(viewNode));

  qMRMLVRView* VRWidget = new qMRMLVRView(this->layoutManager()->viewport());
  VRWidget->setObjectName(QString("VRWidget%1").arg(viewNode->GetLayoutLabel()));
  //VRWidget->setViewLabel(viewNode->GetLayoutLabel());
  VRWidget->setMRMLScene(this->mrmlScene());
  vtkMRMLVRViewNode* vrViewNode = vtkMRMLVRViewNode::SafeDownCast(viewNode);
  VRWidget->setMRMLVRViewNode(vrViewNode);
  return VRWidget;
}

//-----------------------------------------------------------------------------
class qSlicerLayoutManagerPrivate: public qMRMLLayoutManagerPrivate
{
  Q_DECLARE_PUBLIC(qSlicerLayoutManager);
public:
  qSlicerLayoutManagerPrivate(qSlicerLayoutManager& object);
  typedef qMRMLLayoutManagerPrivate Superclass;

  void init();

  qMRMLVRView* vrView(vtkMRMLVRViewNode* node)const;

public:
  QString            ScriptedDisplayableManagerDirectory;
};

// --------------------------------------------------------------------------
qSlicerLayoutManagerPrivate::qSlicerLayoutManagerPrivate(qSlicerLayoutManager& object)
  : qMRMLLayoutManagerPrivate(object)
{
}

// --------------------------------------------------------------------------
void qSlicerLayoutManagerPrivate::init()
{
  this->Superclass::init();
  Q_Q(qSlicerLayoutManager);
  qMRMLLayoutVRViewFactory* vrViewFactory =
   new qMRMLLayoutVRViewFactory;
  q->registerViewFactory(vrViewFactory);
}

//------------------------------------------------------------------------------
qMRMLVRView* qSlicerLayoutManagerPrivate::vrView(vtkMRMLVRViewNode* node)const
{
  Q_Q(const qMRMLLayoutManager);
  return qobject_cast<qMRMLVRView*>(
    q->mrmlViewFactory("vtkMRMLVRViewNode")->viewWidget(node));
}

//------------------------------------------------------------------------------
// qSlicerLayoutManager methods

// -----------------------------------------------------------------------------
qSlicerLayoutManager::qSlicerLayoutManager(QWidget* widget)
  : qMRMLLayoutManager(new qSlicerLayoutManagerPrivate(*this), widget, widget)
{
  Q_D(qSlicerLayoutManager);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerLayoutManager::setScriptedDisplayableManagerDirectory(
  const QString& scriptedDisplayableManagerDirectory)
{
#ifdef Slicer_USE_PYTHONQT
  if (qSlicerCoreApplication::testAttribute(
        qSlicerCoreApplication::AA_DisablePython))
    {
    return;
    }
  Q_D(qSlicerLayoutManager);

  Q_ASSERT(QFileInfo(scriptedDisplayableManagerDirectory).isDir());
  d->ScriptedDisplayableManagerDirectory = scriptedDisplayableManagerDirectory;
  // Disable for now as we don't have any displayable managers and
  // loading the python file on Windows 64b in Debug crashes.
  //vtkMRMLSliceViewDisplayableManagerFactory* sliceFactory
  //  = vtkMRMLSliceViewDisplayableManagerFactory::GetInstance();
  //sliceFactory->RegisterDisplayableManager(
  //  QFileInfo(QDir(scriptedDisplayableManagerDirectory),
  //            "vtkScriptedExampleDisplayableManager.py")
  //    .absoluteFilePath().toLatin1());
  //vtkMRMLThreeDViewDisplayableManagerFactory* threeDFactory
  //  = vtkMRMLThreeDViewDisplayableManagerFactory::GetInstance();
  //threeDFactory->RegisterDisplayableManager(
  //  QFileInfo(QDir(scriptedDisplayableManagerDirectory),
  //            "vtkScriptedExampleDisplayableManager.py")
  //    .absoluteFilePath().toLatin1());
#else
  Q_UNUSED(scriptedDisplayableManagerDirectory);
#endif
}

//------------------------------------------------------------------------------
void qSlicerLayoutManager::setCurrentModule(const QString& moduleName)
{
  emit this->selectModule(moduleName);
}

//------------------------------------------------------------------------------
qMRMLVRView* qSlicerLayoutManager::vrView(int id)const
{
  return qobject_cast<qMRMLVRView*>(
    this->mrmlViewFactory("vtkMRMLVRViewNode")->viewWidget(id));
}