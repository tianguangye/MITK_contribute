/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkPointSetDataInteractor.h"
#include "mitkMouseMoveEvent.h"

#include "mitkInteractionConst.h" // TODO: refactor file
#include "mitkInternalEvent.h"
#include "mitkOperationEvent.h"
#include "mitkRenderingManager.h"
#include <mitkPointOperation.h>
//
#include "mitkBaseRenderer.h"
#include "mitkDispatcher.h"

#include "mitkUndoController.h"

void mitk::PointSetDataInteractor::ConnectActionsAndFunctions()
{
  // Condition which is evaluated before transition is taken
  // following actions in the statemachine are only executed if it returns TRUE
  CONNECT_CONDITION("isoverpoint", CheckSelection);
  CONNECT_FUNCTION("addpoint", AddPoint);
  CONNECT_FUNCTION("selectpoint", SelectPoint);
  CONNECT_FUNCTION("unselect", UnSelectPointAtPosition);
  CONNECT_FUNCTION("unselectAll", UnSelectAll);
  CONNECT_FUNCTION("initMove", InitMove);
  CONNECT_FUNCTION("movePoint", MovePoint);
  CONNECT_FUNCTION("finishMovement", FinishMove);
  CONNECT_FUNCTION("removePoint", RemovePoint);
  CONNECT_FUNCTION("keyDelete", KeyDelete);
}

void mitk::PointSetDataInteractor::AddPoint(StateMachineAction *stateMachineAction, InteractionEvent *interactionEvent)
{
  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  // disallow adding of new points if maximum number of points is reached
  if (m_MaxNumberOfPoints > 1 && m_PointSet->GetSize(timeStep) >= m_MaxNumberOfPoints)
  {
    return;
  }
  // To add a point the minimal information is the position, this method accepts all InteractionsPositionEvents
  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    mitk::Point3D itkPoint = positionEvent->GetPositionInWorld();

    if (m_Bounds.IsNotNull())
    {
      if (!m_Bounds->IsInside(itkPoint))
        return; // Disallow adding new points outside of the required bounds.
    }

    this->UnselectAll(timeStep, timeInMs);

    int lastPosition = 0;
    mitk::PointSet::PointsIterator it, end;
    it = m_PointSet->Begin(timeStep);
    end = m_PointSet->End(timeStep);
    while (it != end)
    {
      if (!m_PointSet->IndexExists(lastPosition, timeStep))
        break;
      ++it;
      ++lastPosition;
    }

    // Insert a Point to the PointSet
    // 2) Create the Operation inserting the point
	if (m_PointSet->IsEmpty()) { lastPosition = 0; }
    auto *doOp = new mitk::PointOperation(OpINSERT, timeInMs, itkPoint, lastPosition);

    // 3) If Undo is enabled, also create the inverse Operation
    if (m_UndoEnabled)
    {
      auto *undoOp = new mitk::PointOperation(OpREMOVE, timeInMs, itkPoint, lastPosition);
      // 4) Do and Undo Operations are combined in an Operation event which also contains the target of the operations
      // (here m_PointSet)
      OperationEvent *operationEvent = new OperationEvent(m_PointSet, doOp, undoOp, "Add point");
      // 5) Store the Operation in the UndoController
      OperationEvent::IncCurrObjectEventId();
      m_UndoController->SetOperationEvent(operationEvent);
    }

    // 6) Execute the Operation performs the actual insertion of the point into the PointSet
    m_PointSet->ExecuteOperation(doOp);

    // 7) If Undo is not enabled the Do-Operation is to be dropped to prevent memory leaks.
    if (!m_UndoEnabled)
      delete doOp;

    RenderingManager::GetInstance()->RequestUpdateAll();

    // Check if points form a closed contour now, if so fire an InternalEvent
    IsClosedContour(stateMachineAction, interactionEvent);

    if (m_MaxNumberOfPoints > 0 && m_PointSet->GetSize(timeStep) >= m_MaxNumberOfPoints)
    {
      // Signal that DataNode is fully filled
      this->NotifyResultReady();
      // Send internal event that can be used by StateMachines to switch in a different state
      InternalEvent::Pointer event = InternalEvent::New(nullptr, this, "MaximalNumberOfPoints");
      positionEvent->GetSender()->GetDispatcher()->QueueEvent(event.GetPointer());
    }
  }
}

void mitk::PointSetDataInteractor::SelectPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    Point3D point = positionEvent->GetPositionInWorld();
    // iterate over point set and check if it contains a point close enough to the pointer to be selected
    int index = GetPointIndexByPosition(point, timeStep);
    if (index != -1)
    {
      // first deselect the other points
      // undoable deselect of all points in the DataList
      this->UnselectAll(timeStep, timeInMs);

      auto *doOp = new mitk::PointOperation(OpSELECTPOINT, timeInMs, point, index);

      /*if (m_UndoEnabled)
      {
        PointOperation* undoOp = new mitk::PointOperation(OpDESELECTPOINT,timeInMs,point, index);
        OperationEvent *operationEvent = new OperationEvent(m_PointSet, doOp, undoOp, "Select Point");
        OperationEvent::IncCurrObjectEventId();
        m_UndoController->SetOperationEvent(operationEvent);
      }*/

      // execute the Operation
      m_PointSet->ExecuteOperation(doOp);

      if (!m_UndoEnabled)
        delete doOp;

      RenderingManager::GetInstance()->RequestUpdateAll();
    }
  }
}

mitk::PointSetDataInteractor::PointSetDataInteractor()
  : m_MaxNumberOfPoints(0),
    m_SelectionAccuracy(3.5),
    m_IsMovementEnabled(true),
    m_IsRemovalEnabled(true)
{
}

mitk::PointSetDataInteractor::~PointSetDataInteractor()
{
}

void mitk::PointSetDataInteractor::EnableMovement(bool enabled)
{
  m_IsMovementEnabled = enabled;
}

void mitk::PointSetDataInteractor::EnableRemoval(bool enabled)
{
  m_IsRemovalEnabled = enabled;
}

void mitk::PointSetDataInteractor::SetBounds(BaseGeometry* geometry)
{
  m_Bounds = geometry != nullptr
    ? geometry->Clone()
    : nullptr;
}

void mitk::PointSetDataInteractor::RemovePoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  if (!m_IsRemovalEnabled)
    return;

  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    mitk::Point3D itkPoint = positionEvent->GetPositionInWorld();

    // search the point in the list
    int position = m_PointSet->SearchPoint(itkPoint, m_SelectionAccuracy, timeStep);
    if (position >= 0) // found a point
    {
      PointSet::PointType pt = m_PointSet->GetPoint(position, timeStep);
      itkPoint[0] = pt[0];
      itkPoint[1] = pt[1];
      itkPoint[2] = pt[2];

      auto *doOp = new mitk::PointOperation(OpREMOVE, timeInMs, itkPoint, position);
      if (m_UndoEnabled) // write to UndoMechanism
      {
        auto *undoOp = new mitk::PointOperation(OpINSERT, timeInMs, itkPoint, position);
        OperationEvent *operationEvent = new OperationEvent(m_PointSet, doOp, undoOp, "Remove point");
        mitk::OperationEvent::IncCurrObjectEventId();
        m_UndoController->SetOperationEvent(operationEvent);
      }
      // execute the Operation
      m_PointSet->ExecuteOperation(doOp);

      if (!m_UndoEnabled)
        delete doOp;

      /*now select the point "position-1",
      and if it is the first in list,
      then continue at the last in list*/
      // only then a select of a point is possible!
      if (m_PointSet->GetSize(timeStep) > 0)
      {
        this->SelectPoint(m_PointSet->Begin(timeStep)->Index(), timeStep, timeInMs);
      }
    }
    RenderingManager::GetInstance()->RequestUpdateAll();
  }
}

void mitk::PointSetDataInteractor::IsClosedContour(StateMachineAction *, InteractionEvent *interactionEvent)
{
  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    Point3D point = positionEvent->GetPositionInWorld();
    // iterate over point set and check if it contains a point close enough to the pointer to be selected
    if (GetPointIndexByPosition(point, timeStep) != -1 && m_PointSet->GetSize(timeStep) >= 3)
    {
      InternalEvent::Pointer event = InternalEvent::New(nullptr, this, "ClosedContour");
      positionEvent->GetSender()->GetDispatcher()->QueueEvent(event.GetPointer());
    }
  }
}

void mitk::PointSetDataInteractor::MovePoint(StateMachineAction *stateMachineAction, InteractionEvent *interactionEvent)
{
  if (!m_IsMovementEnabled)
    return;

  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();
  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    IsClosedContour(stateMachineAction, interactionEvent);

    // search the elements in the list that are selected then calculate the
    // vector, because only with the vector we can move several elements in
    // the same direction
    //   newPoint - lastPoint = vector
    // then move all selected and set the lastPoint = newPoint.
    // then add all vectors to a summeryVector (to be able to calculate the
    // startpoint for undoOperation)
    auto newPoint = positionEvent->GetPositionInWorld();
    mitk::Vector3D dirVector = newPoint - m_LastPoint;

    // sum up all Movement for Undo in FinishMovement
    m_SumVec = m_SumVec + dirVector;

    mitk::PointSet::PointsIterator it, end;
    it = m_PointSet->Begin(timeStep);
    end = m_PointSet->End(timeStep);
    while (it != end)
    {
      int position = it->Index();
      if (m_PointSet->GetSelectInfo(position, timeStep)) // if selected
      {
        auto pt = m_PointSet->GetPoint(position, timeStep);
        auto resultPoint = pt + dirVector;

        if (m_Bounds.IsNotNull())
          resultPoint = m_Bounds->ClampPoint(resultPoint);

        auto *doOp = new mitk::PointOperation(OpMOVE, timeInMs, resultPoint, position);
        // execute the Operation
        // here no undo is stored, because the movement-steps aren't interesting.
        // only the start and the end is interisting to store for undo.
        m_PointSet->ExecuteOperation(doOp);
        delete doOp;
      }
      ++it;
    }
    m_LastPoint = newPoint; // for calculation of the direction vector
    // Update the display
    RenderingManager::GetInstance()->RequestUpdateAll();
    IsClosedContour(stateMachineAction, interactionEvent);
  }
}

void mitk::PointSetDataInteractor::UnSelectPointAtPosition(StateMachineAction *, InteractionEvent *interactionEvent)
{
  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    Point3D point = positionEvent->GetPositionInWorld();
    // iterate over point set and check if it contains a point close enough to the pointer to be selected
    int index = GetPointIndexByPosition(point, timeStep);
    // here it is ensured that we don't switch from one point being selected to another one being selected,
    // without accepting the unselect of the current point
    if (index != -1)
    {
      auto *doOp = new mitk::PointOperation(OpDESELECTPOINT, timeInMs, point, index);

      /*if (m_UndoEnabled)
      {
        PointOperation* undoOp = new mitk::PointOperation(OpSELECTPOINT,timeInMs, point, index);
        OperationEvent *operationEvent = new OperationEvent(m_PointSet, doOp, undoOp, "Unselect Point");
        OperationEvent::IncCurrObjectEventId();
        m_UndoController->SetOperationEvent(operationEvent);
      }*/

      m_PointSet->ExecuteOperation(doOp);

      if (!m_UndoEnabled)
        delete doOp;

      RenderingManager::GetInstance()->RequestUpdateAll();
    }
  }
}

void mitk::PointSetDataInteractor::UnSelectAll(mitk::StateMachineAction *, mitk::InteractionEvent *interactionEvent)
{
  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    Point3D positioninWorld = positionEvent->GetPositionInWorld();
    PointSet::PointsContainer::Iterator it, end;

    PointSet::DataType *itkPointSet = m_PointSet->GetPointSet(timeStep);

    end = itkPointSet->GetPoints()->End();

    for (it = itkPointSet->GetPoints()->Begin(); it != end; it++)
    {
      int position = it->Index();
      // then declare an operation which unselects this point;
      // UndoOperation as well!
      if (m_PointSet->GetSelectInfo(position, timeStep))
      {
        float distance = sqrt(positioninWorld.SquaredEuclideanDistanceTo(m_PointSet->GetPoint(position, timeStep)));
        if (distance > m_SelectionAccuracy)
        {
          mitk::Point3D noPoint;
          noPoint.Fill(0);
          auto *doOp = new mitk::PointOperation(OpDESELECTPOINT, timeInMs, noPoint, position);

          /*if ( m_UndoEnabled )
          {
            mitk::PointOperation* undoOp = new mitk::PointOperation(OpSELECTPOINT, timeInMs,  noPoint, position);
            OperationEvent *operationEvent = new OperationEvent( m_PointSet, doOp, undoOp, "Unselect Point" );
            OperationEvent::IncCurrObjectEventId();
            m_UndoController->SetOperationEvent( operationEvent );
          }*/

          m_PointSet->ExecuteOperation(doOp);

          if (!m_UndoEnabled)
            delete doOp;
        }
      }
    }
  }
  else
  {
    this->UnselectAll(timeStep, timeInMs);
  }

  RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::PointSetDataInteractor::UpdatePointSet(mitk::StateMachineAction *, mitk::InteractionEvent *)
{
  auto *pointSet = dynamic_cast<mitk::PointSet *>(this->GetDataNode()->GetData());
  if (pointSet == nullptr)
  {
    MITK_ERROR << "PointSetDataInteractor:: No valid point set .";
    return;
  }

  m_PointSet = pointSet;
}

void mitk::PointSetDataInteractor::Abort(StateMachineAction *, InteractionEvent *interactionEvent)
{
  InternalEvent::Pointer event = InternalEvent::New(nullptr, this, IntDeactivateMe);
  interactionEvent->GetSender()->GetDispatcher()->QueueEvent(event.GetPointer());
}

void mitk::PointSetDataInteractor::KeyDelete(StateMachineAction*, InteractionEvent* interactionEvent)
{
  auto renderer = interactionEvent->GetSender();
  auto t = renderer->GetTimeStep(m_PointSet);
  auto id = m_PointSet->SearchSelectedPoint(t);

  if (id == -1)
    return;

  auto point = m_PointSet->GetPoint(id, t);
  Point2D displayPoint;
  renderer->WorldToDisplay(point, displayPoint);

  auto event = InteractionPositionEvent::New(nullptr, displayPoint);
  event->SetSender(renderer);

  this->RemovePoint(nullptr, event.GetPointer());
}

/*
 * Check whether the DataNode contains a pointset, if not create one and add it.
 */
void mitk::PointSetDataInteractor::DataNodeChanged()
{
  if (GetDataNode() != nullptr)
  {
    auto *points = dynamic_cast<PointSet *>(GetDataNode()->GetData());
    if (points == nullptr)
    {
      m_PointSet = PointSet::New();
      GetDataNode()->SetData(m_PointSet);
    }
    else
    {
      m_PointSet = points;
    }
    // load config file parameter: maximal number of points
    mitk::PropertyList::Pointer properties = GetAttributes();

    if (properties.IsNull())
      return;

    std::string strNumber;
    if (properties->GetStringProperty("MaxPoints", strNumber))
    {
      m_MaxNumberOfPoints = atoi(strNumber.c_str());
    }
  }

  Superclass::DataNodeChanged();
}

void mitk::PointSetDataInteractor::InitMove(StateMachineAction *, InteractionEvent *interactionEvent)
{
  if (!m_IsMovementEnabled)
    return;

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);

  if (positionEvent == nullptr)
    return;

  // start of the movement is stored to calculate the undo coordinate
  // in FinishMovement
  m_LastPoint = positionEvent->GetPositionInWorld();

  // initialize a value to calculate the movement through all
  // MouseMoveEvents from MouseClick to MouseRelease
  m_SumVec.Fill(0);

  GetDataNode()->SetProperty("contourcolor", ColorProperty::New(1.0, 1.0, 1.0));
}

void mitk::PointSetDataInteractor::FinishMove(StateMachineAction *, InteractionEvent *interactionEvent)
{
  if (!m_IsMovementEnabled)
    return;

  unsigned int timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
  ScalarType timeInMs = interactionEvent->GetSender()->GetTime();

  auto *positionEvent = dynamic_cast<InteractionPositionEvent *>(interactionEvent);

  if (positionEvent != nullptr)
  {
    // finish the movement:
    // the final point is m_LastPoint
    // m_SumVec stores the movement in a vector
    // the operation would not be necessary, but we need it for the undo Operation.
    // m_LastPoint is for the Operation
    // the point for undoOperation calculates from all selected
    // elements (point) - m_SumVec

    // search all selected elements and move them with undo-functionality.

    mitk::PointSet::PointsIterator it, end;
    it = m_PointSet->Begin(timeStep);
    end = m_PointSet->End(timeStep);
    while (it != end)
    {
      int position = it->Index();
      if (m_PointSet->GetSelectInfo(position, timeStep)) // if selected
      {
        PointSet::PointType pt = m_PointSet->GetPoint(position, timeStep);
        auto *doOp = new mitk::PointOperation(OpMOVE, timeInMs, pt, position);

        if (m_UndoEnabled) //&& (posEvent->GetType() == mitk::Type_MouseButtonRelease)
        {
          // set the undo-operation, so the final position is undo-able
          // calculate the old Position from the already moved position - m_SumVec
          mitk::Point3D undoPoint = (pt - m_SumVec);
          auto *undoOp = new mitk::PointOperation(OpMOVE, timeInMs, undoPoint, position);
          OperationEvent *operationEvent = new OperationEvent(m_PointSet, doOp, undoOp, "Move point");
          OperationEvent::IncCurrObjectEventId();
          m_UndoController->SetOperationEvent(operationEvent);
        }
        // execute the Operation
        m_PointSet->ExecuteOperation(doOp);

        if (!m_UndoEnabled)
          delete doOp;
      }
      ++it;
    }

    // Update the display
    RenderingManager::GetInstance()->RequestUpdateAll();
  }
  else
  {
    return;
  }
  this->NotifyResultReady();
}

void mitk::PointSetDataInteractor::SetAccuracy(float accuracy)
{
  m_SelectionAccuracy = accuracy;
}

void mitk::PointSetDataInteractor::SetMaxPoints(unsigned int maxNumber)
{
  m_MaxNumberOfPoints = maxNumber;
}

int mitk::PointSetDataInteractor::GetPointIndexByPosition(Point3D position, unsigned int time, float accuracy)
{
  // Iterate over point set and check if any point is close enough to the pointer to be selected.
  // Choose the closest one of these candidates.

  auto *points = dynamic_cast<PointSet *>(GetDataNode()->GetData());
  int index = -1;
  if (points == nullptr)
  {
    return index;
  }

  if (points->GetPointSet(time) == nullptr)
    return -1;

  PointSet::PointsContainer *pointsContainer = points->GetPointSet(time)->GetPoints();

  if (accuracy == -1)
    accuracy = m_SelectionAccuracy;

  const auto end = pointsContainer->End();
  float minDistance = accuracy;
  float distance;

  for (auto it = pointsContainer->Begin(); it != end; ++it)
  {
    distance = sqrtf(position.SquaredEuclideanDistanceTo(points->GetPoint(it->Index(), time)));
    if (distance < minDistance)
    {
      minDistance = distance;
      index = it->Index();
    }
  }
  return index;
}

bool mitk::PointSetDataInteractor::CheckSelection(const mitk::InteractionEvent *interactionEvent)
{
  const auto *positionEvent = dynamic_cast<const InteractionPositionEvent *>(interactionEvent);
  if (positionEvent != nullptr)
  {
    const auto timeStep = interactionEvent->GetSender()->GetTimeStep(GetDataNode()->GetData());
    Point3D point = positionEvent->GetPositionInWorld();
    // iterate over point set and check if it contains a point close enough to the pointer to be selected
    int index = GetPointIndexByPosition(point, timeStep);
    if (index != -1)
      return true;
  }
  return false;
}

void mitk::PointSetDataInteractor::UnselectAll(unsigned int timeStep, ScalarType timeInMs)
{
  auto *pointSet = dynamic_cast<mitk::PointSet *>(GetDataNode()->GetData());
  if (pointSet == nullptr)
  {
    return;
  }

  mitk::PointSet::DataType *itkPointSet = pointSet->GetPointSet(timeStep);
  if (itkPointSet == nullptr)
  {
    return;
  }

  mitk::PointSet::PointsContainer::Iterator it, end;
  end = itkPointSet->GetPoints()->End();

  for (it = itkPointSet->GetPoints()->Begin(); it != end; it++)
  {
    int position = it->Index();
    PointSet::PointDataType pointData = {0, false, PTUNDEFINED};
    itkPointSet->GetPointData(position, &pointData);

    // then declare an operation which unselects this point;
    // UndoOperation as well!
    if (pointData.selected)
    {
      mitk::Point3D noPoint;
      noPoint.Fill(0);
      auto *doOp = new mitk::PointOperation(OpDESELECTPOINT, timeInMs, noPoint, position);

      /*if ( m_UndoEnabled )
      {
        mitk::PointOperation *undoOp =
            new mitk::PointOperation(OpSELECTPOINT, timeInMs, noPoint, position);
        OperationEvent *operationEvent = new OperationEvent( pointSet, doOp, undoOp, "Unselect Point" );
        OperationEvent::IncCurrObjectEventId();
        m_UndoController->SetOperationEvent( operationEvent );
      }*/

      pointSet->ExecuteOperation(doOp);

      if (!m_UndoEnabled)
        delete doOp;
    }
  }
}

void mitk::PointSetDataInteractor::SelectPoint(int position, unsigned int timeStep, ScalarType timeInMS)
{
  auto *pointSet = dynamic_cast<mitk::PointSet *>(this->GetDataNode()->GetData());

  // if List is empty, then no selection of a point can be done!
  if ((pointSet == nullptr) || (pointSet->GetSize(timeStep) <= 0))
  {
    return;
  }

  // dummyPoint... not needed anyway
  mitk::Point3D noPoint;
  noPoint.Fill(0);

  auto *doOp = new mitk::PointOperation(OpSELECTPOINT, timeInMS, noPoint, position);

  /*if ( m_UndoEnabled )
  {
    mitk::PointOperation* undoOp = new mitk::PointOperation(OpDESELECTPOINT,timeInMS, noPoint, position);

    OperationEvent *operationEvent = new OperationEvent(pointSet, doOp, undoOp, "Select Point");
    OperationEvent::IncCurrObjectEventId();
    m_UndoController->SetOperationEvent(operationEvent);
  }*/

  pointSet->ExecuteOperation(doOp);

  if (!m_UndoEnabled)
    delete doOp;
}
