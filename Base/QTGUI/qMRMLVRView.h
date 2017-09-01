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

#ifndef __qMRMLVRView_h
#define __qMRMLVRView_h

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKRenderView.h>

// Qt includes
#include <QWidget>

//#include "qMRMLWidgetsExport.h"
#include "qSlicerBaseQTGUIExport.h"

class qMRMLVRViewPrivate;
class vtkMRMLScene;
class vtkMRMLVRViewNode;
class vtkCollection;
class vtkGenericOpenGLRenderWindow;
class vtkRenderWindowInteractor;

class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;
class vtkOpenVRRenderWindowInteractor;
class vtkOpenVRCamera;

/// \brief 3D view for view nodes.
/// For performance reasons, the view block refreshs when the scene is in
/// batch process state.
/// \sa qMRMLVRWidget, qMRMLVRViewControllerWidget, qMRMLSliceView
class Q_SLICER_BASE_QTGUI_EXPORT qMRMLVRView : public QWidget
{
  Q_OBJECT
public:
  /// Superclass typedef
  typedef QWidget Superclass;

  /// Constructors
  explicit qMRMLVRView(QWidget* parent = 0);
  virtual ~qMRMLVRView();

  /// Add a displayable manager to the view,
  /// the displayable manager is proper to the 3D view and is not shared
  /// with other views.
  /// If you want to register a displayable manager with all the 3D
  /// views (existing or future), you need to do it via
  /// vtkMRMLVRViewDisplayableManagerFactory::RegisterDisplayableManager()
  /// By default: vtkMRMLCameraDisplayableManager,
  /// vtkMRMLViewDisplayableManager and vtkMRMLModelDisplayableManager are
  /// already registered.
  void addDisplayableManager(const QString& displayableManager);
  Q_INVOKABLE void getDisplayableManagers(vtkCollection *displayableManagers);

  /// Get the 3D View node observed by view.
  Q_INVOKABLE vtkMRMLVRViewNode* mrmlVRViewNode()const;

  /// Returns the interactor style of the view
  //vtkInteractorObserver* interactorStyle()const;

  /// Methods to rotate/reset the camera,
  /// Can defined a view axis by its index (from 0 to 5)
  /// or its label (defined in vtkMRMLViewNode::AxisLabels)
  /// to rotate to the axis ranged in that order:
  /// -X, +X, -Y, +Y, -Z, +Z
  //Q_INVOKABLE void rotateToViewAxis(unsigned int axisId);
  //Q_INVOKABLE void rotateToViewAxis(const std::string& axisLabel);
  //Q_INVOKABLE void resetCamera(bool resetRotation = true,
  //                             bool resetTranslation = true,
  //                             bool resetDistance = true);

  /// Return if rendering is enabled
  bool renderEnabled() const;

  /// Get a reference to the associated vtkRenderer
  vtkOpenVRRenderer* renderer()const;

  /// Get underlying RenderWindow
  Q_INVOKABLE vtkOpenVRRenderWindow* renderWindow()const;

  /// Get underlying RenderWindow
  Q_INVOKABLE vtkOpenVRRenderWindowInteractor* interactor()const;

public slots:

  /// Set the MRML \a scene that should be listened for events
  /// When the scene is in batch process state, the view blocks all refresh.
  /// \sa renderEnabled
  void setMRMLScene(vtkMRMLScene* newScene);

  /// Set the current \a viewNode to observe
  void setMRMLVRViewNode(vtkMRMLVRViewNode* newViewNode);

  /// Look from a given axis, need a mrml view node to be set
  //void lookFromViewAxis(const ctkAxesWidget::Axis& axis);

  /// Reimplemented to hide items to not take into
  /// account when computing the boundaries
  //virtual void resetFocalPoint();

  /// Enable/Disable rendering
  void setRenderEnabled(bool value);


protected:
  QScopedPointer<qMRMLVRViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLVRView);
  Q_DISABLE_COPY(qMRMLVRView);
};

#endif
