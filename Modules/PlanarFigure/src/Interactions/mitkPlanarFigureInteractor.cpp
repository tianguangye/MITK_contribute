/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#define PLANARFIGUREINTERACTOR_DBG MITK_DEBUG("PlanarFigureInteractor") << __LINE__ << ": "

#include "mitkPlanarFigureInteractor.h"
#include "mitkPlanarBezierCurve.h"
#include "mitkPlanarCircle.h"
#include "mitkPlanarFigure.h"
#include "mitkPlanarPolygon.h"

#include "mitkInteractionPositionEvent.h"
#include "mitkInternalEvent.h"

#include "mitkBaseRenderer.h"
#include "mitkRenderingManager.h"

#include "mitkAbstractTransformGeometry.h"
#include "mitkPlaneGeometry.h"

namespace mitk
{
  itkEventMacroDefinition(PlanarFigureEvent, itk::AnyEvent);
  itkEventMacroDefinition(StartPlacementPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(EndPlacementPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(SelectPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(StartInteractionPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(EndInteractionPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(StartHoverPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(EndHoverPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(ContextMenuPlanarFigureEvent, PlanarFigureEvent);
  itkEventMacroDefinition(PointMovedPlanarFigureEvent, PlanarFigureEvent);
}

mitk::PlanarFigureInteractor::PlanarFigureInteractor()
  : DataInteractor()
  , m_Precision(6.5)
  , m_MinimumPointDistance(25.0)
  , m_IsHovering(false)
{
}

mitk::PlanarFigureInteractor::~PlanarFigureInteractor()
{
}

void mitk::PlanarFigureInteractor::ConnectActionsAndFunctions()
{
  CONNECT_CONDITION("figure_is_on_current_slice", CheckFigureOnRenderingGeometry);
  CONNECT_CONDITION("figure_is_placed", CheckFigurePlaced);
  CONNECT_CONDITION("minimal_figure_is_finished", CheckMinimalFigureFinished);
  CONNECT_CONDITION("hovering_above_figure", CheckFigureHovering);
  CONNECT_CONDITION("hovering_above_point", CheckControlPointHovering);
  CONNECT_CONDITION("figure_is_selected", CheckSelection);
  CONNECT_CONDITION("point_is_valid", CheckPointValidity);
  CONNECT_CONDITION("figure_is_finished", CheckFigureFinished);
  CONNECT_CONDITION("reset_on_point_select_needed", CheckResetOnPointSelect);
  CONNECT_CONDITION("points_can_be_added_or_removed", CheckFigureIsExtendable);
  CONNECT_CONDITION("figure_can_be_deleted", CheckFigureIsDeletable);
  CONNECT_CONDITION("figure_is_editable", CheckFigureIsEditable);
  CONNECT_CONDITION("continuous_points_mode", CheckContinuousPointsMode);

  CONNECT_FUNCTION("finalize_figure", FinalizeFigure);
  CONNECT_FUNCTION("hide_preview_point", HidePreviewPoint)
  CONNECT_FUNCTION("hide_control_points", HideControlPoints)
  CONNECT_FUNCTION("set_preview_point_position", SetPreviewPointPosition)
  CONNECT_FUNCTION("move_current_point", MoveCurrentPoint);
  CONNECT_FUNCTION("deselect_point", DeselectPoint);
  CONNECT_FUNCTION("add_new_point", AddPoint);
  CONNECT_FUNCTION("add_initial_point", AddInitialPoint);
  CONNECT_FUNCTION("remove_selected_point", RemoveSelectedPoint);
  CONNECT_FUNCTION("request_context_menu", RequestContextMenu);
  CONNECT_FUNCTION("select_figure", SelectFigure);
  CONNECT_FUNCTION("select_point", SelectPoint);
  CONNECT_FUNCTION("end_interaction", EndInteraction);
  CONNECT_FUNCTION("start_hovering", StartHovering)
  CONNECT_FUNCTION("end_hovering", EndHovering);
  CONNECT_FUNCTION("delete_figure", DeleteFigure);
  CONNECT_FUNCTION("reset_on_point_select", PerformPointResetOnSelect);
}

bool mitk::PlanarFigureInteractor::CheckContinuousPointsMode(const InteractionEvent*)
{
  return m_ContinuousPointsMode;
}

bool mitk::PlanarFigureInteractor::CheckFigurePlaced(const InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  bool isFigureFinished = false;
  planarFigure->GetPropertyList()->GetBoolProperty("initiallyplaced", isFigureFinished);

  return planarFigure->IsPlaced() && isFigureFinished;
}

void mitk::PlanarFigureInteractor::MoveCurrentPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  bool isEditable = true;
  GetDataNode()->GetBoolProperty("planarfigure.iseditable", isEditable);

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return;
  }

  // Extract point in 2D world coordinates (relative to PlaneGeometry of PlanarFigure)
  Point2D point2D;
  if (!this->TransformPositionEventToPoint2D(positionEvent, planarFigureGeometry, point2D) || !isEditable)
  {
    return;
  }

  planarFigure->InvokeEvent(StartInteractionPlanarFigureEvent());

  // check if the control points shall be hidden during interaction
  bool hidecontrolpointsduringinteraction = false;
  GetDataNode()->GetBoolProperty("planarfigure.hidecontrolpointsduringinteraction", hidecontrolpointsduringinteraction);

  // hide the control points if necessary
  // interactionEvent->GetSender()->GetDataStorage()->BlockNodeModifiedEvents( true );
  GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", !hidecontrolpointsduringinteraction);
  // interactionEvent->GetSender()->GetDataStorage()->BlockNodeModifiedEvents( false );

  // Move current control point to this point
  planarFigure->SetCurrentControlPoint(point2D);

  // Re-evaluate features
  planarFigure->EvaluateFeatures();

  // Update rendered scene
  RenderingManager::GetInstance()->RequestUpdateAll();

  planarFigure->InvokeEvent(PointMovedPlanarFigureEvent());
}

void mitk::PlanarFigureInteractor::FinalizeFigure(StateMachineAction *, InteractionEvent *)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  planarFigure->Modified();
  planarFigure->DeselectControlPoint();
  planarFigure->RemoveLastControlPoint();
  planarFigure->SetProperty("initiallyplaced", mitk::BoolProperty::New(true));
  GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", true);
  GetDataNode()->Modified();
  planarFigure->InvokeEvent(EndPlacementPlanarFigureEvent());
  planarFigure->InvokeEvent(EndInteractionPlanarFigureEvent());

  // Shape might change when figure is finalized, e.g., smoothing of subdivision polygon
  planarFigure->EvaluateFeatures();

  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::EndInteraction(StateMachineAction *, InteractionEvent *)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", true);
  planarFigure->Modified();
  planarFigure->InvokeEvent(EndInteractionPlanarFigureEvent());
  RenderingManager::GetInstance()->RequestUpdateAll();
}

bool mitk::PlanarFigureInteractor::FilterEvents(InteractionEvent *interactionEvent, mitk::DataNode * /*dataNode*/)
{
  if (interactionEvent->GetSender() == nullptr)
    return false;
  if (interactionEvent->GetSender()->GetMapperID() == BaseRenderer::Standard3D)
    return false;

  return true;
}

void mitk::PlanarFigureInteractor::EndHovering(StateMachineAction *, InteractionEvent *)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  planarFigure->ResetPreviewContolPoint();

  // Invoke end-hover event once the mouse is exiting the figure area
  m_IsHovering = false;
  planarFigure->InvokeEvent(EndHoverPlanarFigureEvent());

  // Set bool property to indicate that planar figure is no longer in "hovering" mode
  GetDataNode()->SetBoolProperty("planarfigure.ishovering", false);

  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::DeleteFigure(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  interactionEvent->GetSender()->GetDataStorage()->Remove(GetDataNode());
  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::PerformPointResetOnSelect(StateMachineAction *, InteractionEvent *)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  planarFigure->ResetOnPointSelect();
}

bool mitk::PlanarFigureInteractor::CheckMinimalFigureFinished(const InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  return planarFigure->GetNumberOfControlPoints() >= planarFigure->GetMinimumNumberOfControlPoints();
}

bool mitk::PlanarFigureInteractor::CheckFigureFinished(const InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  return planarFigure->GetNumberOfControlPoints() >= planarFigure->GetMaximumNumberOfControlPoints();
}

bool mitk::PlanarFigureInteractor::CheckFigureIsExtendable(const InteractionEvent * /*interactionEvent*/)
{
  bool isExtendable(false);
  GetDataNode()->GetBoolProperty("planarfigure.isextendable", isExtendable);

  return isExtendable;
}

bool mitk::PlanarFigureInteractor::CheckFigureIsDeletable(const InteractionEvent * /*interactionEvent*/)
{
  bool isDeletable(true);
  GetDataNode()->GetBoolProperty("planarfigure.isdeletable", isDeletable);

  return isDeletable;
}

bool mitk::PlanarFigureInteractor::CheckFigureIsEditable(const InteractionEvent * /*interactionEvent*/)
{
  bool isEditable(true);
  GetDataNode()->GetBoolProperty("planarfigure.iseditable", isEditable);

  return isEditable;
}

void mitk::PlanarFigureInteractor::DeselectPoint(StateMachineAction *, InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  const bool wasSelected = planarFigure->DeselectControlPoint();
  if (wasSelected)
  {
    // Issue event so that listeners may update themselves
    planarFigure->Modified();
    planarFigure->InvokeEvent(EndInteractionPlanarFigureEvent());

    GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", true);
    GetDataNode()->Modified();
  }
}

void mitk::PlanarFigureInteractor::AddPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  /*
  * Added check for "initiallyplaced" due to bug 13097:
  *
  * There are two possible cases in which a point can be inserted into a PlanarPolygon:
  *
  * 1. The figure is currently drawn -> the point will be appended at the end of the figure
  * 2. A point is inserted at a userdefined position after the initial placement of the figure is finished
  *
  * In the second case we need to determine the proper insertion index. In the first case the index always has
  * to be -1 so that the point is appended to the end.
  *
  * These changes are necessary because of a macOS specific issue: If a users draws a PlanarPolygon then the
  * next point to be added moves according to the mouse position. If then the user left clicks in order to add
  * a point one would assume the last move position is identical to the left click position. This is actually the
  * case for windows and linux but somehow NOT for mac. Because of the insertion logic of a new point in the
  * PlanarFigure then for mac the wrong current selected point is determined.
  *
  * With this check here this problem can be avoided. However a redesign of the insertion logic should be considered
  */

  const DataNode::Pointer node = this->GetDataNode();
  const BaseData::Pointer data = node->GetData();

  bool isFigureFinished = false;
  data->GetPropertyList()->GetBoolProperty("initiallyplaced", isFigureFinished);

  bool selected = false;
  bool isEditable = true;

  node->GetBoolProperty("selected", selected);
  node->GetBoolProperty("planarfigure.iseditable", isEditable);

  if (!selected || !isEditable)
  {
    return;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(data.GetPointer());
  if (nullptr == planarFigure)
  {
    return;
  }

  // We can't derive a new control point from a polyline of a Bezier curve
  // as all control points contribute to each polyline point.
  if (dynamic_cast<PlanarBezierCurve *>(planarFigure) != nullptr && isFigureFinished)
    return;

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return;
  }

  // If the planarFigure already has reached the maximum number
  if (planarFigure->GetNumberOfControlPoints() >= planarFigure->GetMaximumNumberOfControlPoints())
  {
    return;
  }

  // Extract point in 2D world coordinates (relative to PlaneGeometry of
  // PlanarFigure)
  Point2D point2D, projectedPoint;
  if (!this->TransformPositionEventToPoint2D(positionEvent, planarFigureGeometry, point2D))
  {
    return;
  }

  // TODO: check segment of polyline we clicked in
  int nextIndex = -1;

  // We only need to check which position to insert the control point
  // when interacting with a PlanarPolygon. For all other types
  // new control points will always be appended

  const mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  const PlaneGeometry *projectionPlane = renderer->GetCurrentWorldPlaneGeometry();

  if (dynamic_cast<mitk::PlanarPolygon *>(planarFigure) && isFigureFinished)
  {
    nextIndex =
      this->IsPositionOverFigure(positionEvent, planarFigure, planarFigureGeometry, projectionPlane, projectedPoint);
  }

  // Add point as new control point

  if (planarFigure->IsPreviewControlPointVisible())
  {
    point2D = planarFigure->GetPreviewControlPoint();
  }

  planarFigure->AddControlPoint(point2D, planarFigure->GetControlPointForPolylinePoint(nextIndex, 0));

  if (planarFigure->IsPreviewControlPointVisible())
  {
    planarFigure->SelectControlPoint(nextIndex);
    planarFigure->ResetPreviewContolPoint();
  }

  // Re-evaluate features
  planarFigure->EvaluateFeatures();
  // this->LogPrintPlanarFigureQuantities( planarFigure );

  // Update rendered scene
  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::AddInitialPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));

  // Invoke event to notify listeners that placement of this PF starts now
  planarFigure->InvokeEvent(StartPlacementPlanarFigureEvent());

  // Use PlaneGeometry of the renderer clicked on for this PlanarFigure
  auto *planeGeometry = const_cast<mitk::PlaneGeometry *>(
    dynamic_cast<const mitk::PlaneGeometry *>(renderer->GetSliceNavigationController()->GetCurrentPlaneGeometry()));
  if (planeGeometry != nullptr && abstractTransformGeometry == nullptr)
  {
    planarFigure->SetPlaneGeometry(planeGeometry);
  }
  else
  {
    return;
  }

  // Extract point in 2D world coordinates (relative to PlaneGeometry of
  // PlanarFigure)
  Point2D point2D;
  if (!this->TransformPositionEventToPoint2D(positionEvent, planeGeometry, point2D))
  {
    return;
  }

  // Place PlanarFigure at this point
  planarFigure->PlaceFigure(point2D);

  // Re-evaluate features
  planarFigure->EvaluateFeatures();
  // this->LogPrintPlanarFigureQuantities( planarFigure );

  // Set a bool property indicating that the figure has been placed in
  // the current RenderWindow. This is required so that the same render
  // window can be re-aligned to the PlaneGeometry of the PlanarFigure later
  // on in an application.
  GetDataNode()->SetBoolProperty("PlanarFigureInitializedWindow", true, renderer);

  // Update rendered scene
  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::StartHovering(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  if (!m_IsHovering)
  {
    // Invoke hover event once when the mouse is entering the figure area
    m_IsHovering = true;
    planarFigure->InvokeEvent(StartHoverPlanarFigureEvent());

    // Set bool property to indicate that planar figure is currently in "hovering" mode
    GetDataNode()->SetBoolProperty("planarfigure.ishovering", true);

    RenderingManager::GetInstance()->RequestUpdateAll();
  }
}

void mitk::PlanarFigureInteractor::SetPreviewPointPosition(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  const mitk::BaseRenderer *renderer = interactionEvent->GetSender();

  planarFigure->DeselectControlPoint();

  mitk::Point2D pointProjectedOntoLine = positionEvent->GetPointerPositionOnScreen();

  bool selected(false);
  bool isExtendable(false);
  bool isEditable(true);
  GetDataNode()->GetBoolProperty("selected", selected);
  GetDataNode()->GetBoolProperty("planarfigure.isextendable", isExtendable);
  GetDataNode()->GetBoolProperty("planarfigure.iseditable", isEditable);

  if (selected && isExtendable && isEditable)
  {
    renderer->DisplayToPlane(pointProjectedOntoLine, pointProjectedOntoLine);
    planarFigure->SetPreviewControlPoint(pointProjectedOntoLine);
  }

  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PlanarFigureInteractor::HideControlPoints(StateMachineAction *, InteractionEvent * /*interactionEvent*/)
{
  GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", false);
}

void mitk::PlanarFigureInteractor::HidePreviewPoint(StateMachineAction *, InteractionEvent *)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  planarFigure->ResetPreviewContolPoint();
  RenderingManager::GetInstance()->RequestUpdateAll();
}

bool mitk::PlanarFigureInteractor::CheckFigureHovering(const InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return false;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return false;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return false;
  }

  const mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  const PlaneGeometry *projectionPlane = renderer->GetCurrentWorldPlaneGeometry();

  mitk::Point2D pointProjectedOntoLine;
  int previousControlPoint = this->IsPositionOverFigure(
    positionEvent, planarFigure, planarFigureGeometry, projectionPlane, pointProjectedOntoLine);

  bool isHovering = (previousControlPoint != -1);
  return isHovering;
}

bool mitk::PlanarFigureInteractor::CheckControlPointHovering(const InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return false;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return false;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return false;
  }

  const mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  const PlaneGeometry *projectionPlane = renderer->GetCurrentWorldPlaneGeometry();

  int pointIndex = -1;
  pointIndex = mitk::PlanarFigureInteractor::IsPositionInsideMarker(
    positionEvent, planarFigure, planarFigureGeometry, projectionPlane, renderer);

  return pointIndex >= 0;
}

bool mitk::PlanarFigureInteractor::CheckSelection(const InteractionEvent * /*interactionEvent*/)
{
  bool selected = false;
  GetDataNode()->GetBoolProperty("selected", selected);

  return selected;
}

void mitk::PlanarFigureInteractor::SelectFigure(StateMachineAction *, InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  planarFigure->InvokeEvent(SelectPlanarFigureEvent());
}

void mitk::PlanarFigureInteractor::SelectPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return;
  }

  const mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  const PlaneGeometry *projectionPlane = renderer->GetCurrentWorldPlaneGeometry();

  const int pointIndex = mitk::PlanarFigureInteractor::IsPositionInsideMarker(
    positionEvent, planarFigure, planarFigureGeometry, projectionPlane, renderer);

  if (pointIndex >= 0)
  {
    // If mouse is above control point, mark it as selected
    planarFigure->SelectControlPoint(pointIndex);
  }
  else
  {
    planarFigure->DeselectControlPoint();
  }
}

bool mitk::PlanarFigureInteractor::CheckPointValidity(const InteractionEvent *interactionEvent)
{
  // Check if the distance of the current point to the previously set point in display coordinates
  // is sufficient (if a previous point exists)

  auto positionEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return false;
  }

  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  return IsMousePositionAcceptableAsNewControlPoint(positionEvent, planarFigure);
}

void mitk::PlanarFigureInteractor::RemoveSelectedPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  const int selectedControlPoint = planarFigure->GetSelectedControlPoint();
  planarFigure->RemoveControlPoint(selectedControlPoint);

  // Re-evaluate features
  planarFigure->EvaluateFeatures();
  planarFigure->Modified();

  GetDataNode()->SetBoolProperty("planarfigure.drawcontrolpoints", true);
  planarFigure->InvokeEvent(EndInteractionPlanarFigureEvent());
  RenderingManager::GetInstance()->RequestUpdateAll();

  mitk::BaseRenderer *renderer = interactionEvent->GetSender();
  HandleEvent(mitk::InternalEvent::New(renderer, this, "Dummy-Event"), GetDataNode());
}

void mitk::PlanarFigureInteractor::RequestContextMenu(StateMachineAction *, InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return;
  }

  bool selected = false;
  GetDataNode()->GetBoolProperty("selected", selected);

  // no need to invoke this if the figure is already selected
  if (!selected)
  {
    planarFigure->InvokeEvent(SelectPlanarFigureEvent());
  }

  planarFigure->InvokeEvent(ContextMenuPlanarFigureEvent());
}

bool mitk::PlanarFigureInteractor::CheckResetOnPointSelect(const InteractionEvent * /*interactionEvent*/)
{
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  bool isEditable = true;
  GetDataNode()->GetBoolProperty("planarfigure.iseditable", isEditable);

  // Reset the PlanarFigure if required
  return isEditable && planarFigure->ResetOnPointSelectNeeded();
}

bool mitk::PlanarFigureInteractor::CheckFigureOnRenderingGeometry(const InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return false;
  }

  const mitk::Point3D worldPoint3D = positionEvent->GetPositionInWorld();
  auto planarFigure = dynamic_cast<PlanarFigure*>(GetDataNode()->GetData());
  if (nullptr == planarFigure)
  {
    return false;
  }

  auto planarFigureGeometry = planarFigure->GetPlaneGeometry();
  if (nullptr == planarFigureGeometry)
  {
    return false;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(0));
  if (nullptr != abstractTransformGeometry)
  {
    return false;
  }

  const double planeThickness = planarFigureGeometry->GetExtentInMM(2);
  return planarFigureGeometry->Distance(worldPoint3D) <= planeThickness;
}

void mitk::PlanarFigureInteractor::SetPrecision(mitk::ScalarType precision)
{
  m_Precision = precision;
}

void mitk::PlanarFigureInteractor::SetMinimumPointDistance(ScalarType minimumDistance)
{
  m_MinimumPointDistance = minimumDistance * minimumDistance; // Used squared in distance calculations.
}

void mitk::PlanarFigureInteractor::EnableContinuousPointsMode()
{
  m_ContinuousPointsMode = true;
}

bool mitk::PlanarFigureInteractor::TransformPositionEventToPoint2D(const InteractionPositionEvent *positionEvent,
                                                                   const PlaneGeometry *planarFigureGeometry,
                                                                   Point2D &point2D)
{
  if (nullptr == positionEvent || nullptr == planarFigureGeometry)
  {
    return false;
  }

  const mitk::Point3D worldPoint3D = positionEvent->GetPositionInWorld();

  // TODO: proper handling of distance tolerance
  if (planarFigureGeometry->Distance(worldPoint3D) > 0.1)
  {
    return false;
  }

  // Project point onto plane of this PlanarFigure
  planarFigureGeometry->Map(worldPoint3D, point2D);
  return true;
}

bool mitk::PlanarFigureInteractor::TransformObjectToDisplay(const mitk::Point2D &point2D,
                                                            mitk::Point2D &displayPoint,
                                                            const mitk::PlaneGeometry *objectGeometry,
                                                            const mitk::PlaneGeometry *rendererGeometry,
                                                            const mitk::BaseRenderer *renderer) const
{
  if (nullptr == objectGeometry || nullptr == rendererGeometry || nullptr == renderer)
  {
    return false;
  }

  mitk::Point3D point3D;

  // Map circle point from local 2D geometry into 3D world space
  objectGeometry->Map(point2D, point3D);

  const double planeThickness = objectGeometry->GetExtentInMM(2);

  // TODO: proper handling of distance tolerance
  if (rendererGeometry->Distance(point3D) < planeThickness / 3.0)
  {
    // Project 3D world point onto display geometry
    renderer->WorldToDisplay(point3D, displayPoint);
    return true;
  }

  return false;
}

bool mitk::PlanarFigureInteractor::IsPointNearLine(const mitk::Point2D &point,
                                                   const mitk::Point2D &startPoint,
                                                   const mitk::Point2D &endPoint,
                                                   mitk::Point2D &projectedPoint) const
{
  mitk::Vector2D n1 = endPoint - startPoint;
  n1.Normalize();

  // Determine dot products between line vector and startpoint-point / endpoint-point vectors
  const double l1 = n1 * (point - startPoint);
  const double l2 = -n1 * (point - endPoint);

  // Determine projection of specified point onto line defined by start / end point
  const mitk::Point2D crossPoint = startPoint + n1 * l1;
  projectedPoint = crossPoint;

  const float dist1 = crossPoint.SquaredEuclideanDistanceTo(point);
  const float dist2 = endPoint.SquaredEuclideanDistanceTo(point);
  const float dist3 = startPoint.SquaredEuclideanDistanceTo(point);

  // Point is inside encompassing rectangle IF
  // - its distance to its projected point is small enough
  // - it is not further outside of the line than the defined tolerance
  if (((dist1 < 20.0) && (l1 > 0.0) && (l2 > 0.0)) || dist2 < 20.0 || dist3 < 20.0)
  {
    return true;
  }

  return false;
}

int mitk::PlanarFigureInteractor::IsPositionOverFigure(const InteractionPositionEvent *positionEvent,
                                                       PlanarFigure *planarFigure,
                                                       const PlaneGeometry *planarFigureGeometry,
                                                       const PlaneGeometry *rendererGeometry,
                                                       Point2D &pointProjectedOntoLine) const
{
  if (nullptr == positionEvent || nullptr == planarFigure || nullptr == planarFigureGeometry
    || nullptr == rendererGeometry)
  {
    return -1;
  }

  mitk::Point2D displayPosition = positionEvent->GetPointerPositionOnScreen();

  // Iterate over all polylines of planar figure, and check if
  // any one is close to the current display position
  Point2D polyLinePoint;
  Point2D firstPolyLinePoint;
  Point2D previousPolyLinePoint;
  for (unsigned short loop = 0; loop < planarFigure->GetPolyLinesSize(); ++loop)
  {
    const auto& polyLine = planarFigure->GetPolyLine(loop);

    bool firstPoint(true);
    for (auto it = polyLine.begin(); it != polyLine.end(); ++it)
    {
      // Get plane coordinates of this point of polyline (if possible)
      if (!this->TransformObjectToDisplay(
            *it, polyLinePoint, planarFigureGeometry, rendererGeometry, positionEvent->GetSender()))
      {
        break; // Poly line invalid (not on current 2D plane) --> skip it
      }

      if (firstPoint)
      {
        firstPolyLinePoint = polyLinePoint;
        firstPoint = false;
      }
      else if (this->IsPointNearLine(displayPosition, previousPolyLinePoint, polyLinePoint, pointProjectedOntoLine))
      {
        // Point is close enough to line segment --> Return index of the segment
        return std::distance(polyLine.begin(), it);
      }
      previousPolyLinePoint = polyLinePoint;
    }

    // For closed figures, also check last line segment
    if (planarFigure->IsClosed() &&
        this->IsPointNearLine(displayPosition, polyLinePoint, firstPolyLinePoint, pointProjectedOntoLine))
    {
      return 0; // Return index of first control point
    }
  }
  return -1;
}

int mitk::PlanarFigureInteractor::IsPositionInsideMarker(const InteractionPositionEvent *positionEvent,
                                                         const PlanarFigure *planarFigure,
                                                         const PlaneGeometry *planarFigureGeometry,
                                                         const PlaneGeometry *rendererGeometry,
                                                         const BaseRenderer *renderer) const
{
  if (nullptr == positionEvent || nullptr == planarFigure || nullptr == planarFigureGeometry
    || nullptr == rendererGeometry || nullptr == renderer)
  {
    return -1;
  }

  const mitk::Point2D displayPosition = positionEvent->GetPointerPositionOnScreen();

  // Iterate over all control points of planar figure, and check if
  // any one is close to the current display position
  mitk::Point2D displayControlPoint;

  const int numberOfControlPoints = planarFigure->GetNumberOfControlPoints();
  for (int i = 0; i < numberOfControlPoints; i++)
  {
    if (this->TransformObjectToDisplay(
          planarFigure->GetControlPoint(i), displayControlPoint, planarFigureGeometry, rendererGeometry, renderer))
    {
      // TODO: variable size of markers
      if (displayPosition.SquaredEuclideanDistanceTo(displayControlPoint) < 20.0)
      {
        return i;
      }
    }
  }

  return -1;
}

void mitk::PlanarFigureInteractor::LogPrintPlanarFigureQuantities(const PlanarFigure *planarFigure)
{
  if (nullptr == planarFigure)
  {
    MITK_INFO << "PlanarFigure invalid.";
  }

  MITK_INFO << "PlanarFigure: " << planarFigure->GetNameOfClass();
  for (unsigned int i = 0; i < planarFigure->GetNumberOfFeatures(); ++i)
  {
    MITK_INFO << "* " << planarFigure->GetFeatureName(i) << ": " << planarFigure->GetQuantity(i) << " "
              << planarFigure->GetFeatureUnit(i);
  }
}

bool mitk::PlanarFigureInteractor::IsMousePositionAcceptableAsNewControlPoint(
  const mitk::InteractionPositionEvent *positionEvent, const PlanarFigure *planarFigure)
{
  if (nullptr == positionEvent || nullptr == planarFigure)
  {
    return false;

  }

  const BaseRenderer *renderer = positionEvent->GetSender();
  if (nullptr == renderer)
  {
    return false;
  }

  // Get the timestep to support 3D+t
  const int timeStep(renderer->GetTimeStep(planarFigure));

  bool tooClose(false);

  auto planarFigureGeometry = dynamic_cast<mitk::PlaneGeometry*>(planarFigure->GetGeometry(timeStep));
  if (nullptr == planarFigureGeometry)
  {
    return false;
  }

  auto abstractTransformGeometry = dynamic_cast<AbstractTransformGeometry*>(planarFigure->GetGeometry(timeStep));
  if (nullptr != abstractTransformGeometry)
  {
    return false;
  }

  Point2D point2D;
  // Get the point2D from the positionEvent
  if (!this->TransformPositionEventToPoint2D(positionEvent, planarFigureGeometry, point2D))
  {
    return false;
  }

  // apply the controlPoint constraints of the planarFigure to get the
  // coordinates that would actually be used.
  const Point2D correctedPoint = const_cast<PlanarFigure *>(planarFigure)->ApplyControlPointConstraints(0, point2D);

  // map the 2D coordinates of the new point to world-coordinates
  // and transform those to display-coordinates
  mitk::Point3D newPoint3D;
  planarFigureGeometry->Map(correctedPoint, newPoint3D);
  mitk::Point2D newDisplayPosition;
  renderer->WorldToDisplay(newPoint3D, newDisplayPosition);

  const int selectedControlPoint = planarFigure->GetSelectedControlPoint();
  const int numControlPoints = static_cast<int>(planarFigure->GetNumberOfControlPoints());

  // For performance reasons we only check the distance to the previous
  // control point in continuous points mode.
  const int startIndex = m_ContinuousPointsMode
    ? numControlPoints - 2
    : 0;

  for (int i = startIndex; i < numControlPoints; ++i)
  {
    if (i != selectedControlPoint)
    {
      // Try to convert previous point to current display coordinates
      mitk::Point3D previousPoint3D;
      // map the 2D coordinates of the control-point to world-coordinates
      planarFigureGeometry->Map(planarFigure->GetControlPoint(i), previousPoint3D);

      if (renderer->GetCurrentWorldPlaneGeometry()->Distance(previousPoint3D) < 0.1) // ugly, but assert makes this work
      {
        mitk::Point2D previousDisplayPosition;
        // transform the world-coordinates into display-coordinates
        renderer->WorldToDisplay(previousPoint3D, previousDisplayPosition);

        // Calculate the distance. We use display-coordinates here to make
        // the check independent of the zoom-level of the rendering scene.
        const double a = newDisplayPosition[0] - previousDisplayPosition[0];
        const double b = newDisplayPosition[1] - previousDisplayPosition[1];

        // If point is to close, do not set a new point
        tooClose = (a * a + b * b < m_MinimumPointDistance);
      }
      if (tooClose)
        return false; // abort loop early
    }
  }

  return !tooClose; // default
}

void mitk::PlanarFigureInteractor::ConfigurationChanged()
{
  const mitk::PropertyList::Pointer properties = GetAttributes();

  std::string precision = "";
  if (properties->GetStringProperty("precision", precision))
  {
    m_Precision = atof(precision.c_str());
  }
  else
  {
    m_Precision = (ScalarType)6.5;
  }

  std::string minPointDistance = "";
  if (properties->GetStringProperty("minPointDistance", minPointDistance))
  {
    m_MinimumPointDistance = atof(minPointDistance.c_str());
  }
  else
  {
    m_MinimumPointDistance = (ScalarType)25.0;
  }
}
