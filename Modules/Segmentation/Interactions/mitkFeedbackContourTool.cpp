/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkFeedbackContourTool.h"
#include "mitkToolManager.h"

#include "mitkColorProperty.h"
#include "mitkProperties.h"
#include "mitkStringProperty.h"

#include "mitkBaseRenderer.h"
#include "mitkDataStorage.h"

#include "mitkRenderingManager.h"

#include "mitkAbstractTransformGeometry.h"

mitk::FeedbackContourTool::FeedbackContourTool(const char *type) : SegTool2D(type), m_FeedbackContourVisible(false)
{
  m_FeedbackContourNode = DataNode::New();
  m_FeedbackContourNode->SetProperty("name", StringProperty::New("One of FeedbackContourTool's feedback nodes"));
  m_FeedbackContourNode->SetProperty("visible", BoolProperty::New(true));
  m_FeedbackContourNode->SetProperty("helper object", BoolProperty::New(true));
  m_FeedbackContourNode->SetProperty("layer", IntProperty::New(1000));
  m_FeedbackContourNode->SetProperty("contour.project-onto-plane", BoolProperty::New(false));
  m_FeedbackContourNode->SetProperty("contour.width", FloatProperty::New(1.0));
  // set to max short, max int doesn't work, max value somewhere hardcoded?
  m_FeedbackContourNode->SetProperty("layer", IntProperty::New(std::numeric_limits<short>::max()));
  m_FeedbackContourNode->SetProperty("fixedLayer", BoolProperty::New(true));
  this->InitializeFeedbackContour(true);
  SetFeedbackContourColorDefault();
}

mitk::FeedbackContourTool::~FeedbackContourTool()
{
}

void mitk::FeedbackContourTool::SetFeedbackContourColor(float r, float g, float b)
{
  m_FeedbackContourNode->SetProperty("contour.color", ColorProperty::New(r, g, b));
}

void mitk::FeedbackContourTool::SetFeedbackContourColor(const Color& color)
{
  this->SetFeedbackContourColor(color[0], color[1], color[2]);
}

void mitk::FeedbackContourTool::SetFeedbackContourColorDefault()
{
  this->SetFeedbackContourColor(0.0, 1.0, 0.0);
}

void mitk::FeedbackContourTool::SetFeedbackContourWidth(float width)
{
  m_FeedbackContourNode->SetFloatProperty("contour.width", width);
}

void mitk::FeedbackContourTool::Deactivated()
{
  Superclass::Deactivated();
  DataStorage *storage = this->GetToolManager()->GetDataStorage();
  if (storage && m_FeedbackContourNode.IsNotNull())
  {
    storage->Remove(m_FeedbackContourNode);
    m_FeedbackContour->Clear();
    SetFeedbackContourVisible(false);
  }
}

void mitk::FeedbackContourTool::Activated()
{
  Superclass::Activated();
  this->InitializeFeedbackContour(true);
  this->SetFeedbackContourVisible(true);
}

const mitk::ContourModel *mitk::FeedbackContourTool::GetFeedbackContour() const
{
  return m_FeedbackContour;
}

void mitk::FeedbackContourTool::InitializeFeedbackContour(bool isClosed)
{
  m_FeedbackContour = ContourModel::New();
  m_FeedbackContour->SetClosed(isClosed);

  auto workingSeg = this->GetWorkingData();

  if (nullptr != workingSeg)
  {
    m_FeedbackContour->Expand(workingSeg->GetTimeSteps());

    auto contourTimeGeometry = workingSeg->GetTimeGeometry()->Clone();
    contourTimeGeometry->ReplaceTimeStepGeometries(m_FeedbackContour->GetGeometry());
    m_FeedbackContour->SetTimeGeometry(contourTimeGeometry);

    for (unsigned int t = 0; t < m_FeedbackContour->GetTimeSteps(); ++t)
    {
      m_FeedbackContour->SetClosed(isClosed, t);
    }
  }

  m_FeedbackContourNode->SetData(m_FeedbackContour);
}

void mitk::FeedbackContourTool::ClearsCurrentFeedbackContour(bool isClosed)
{
  if (!m_FeedbackContour->GetTimeGeometry()->IsValidTimePoint(this->GetLastTimePointTriggered()))
  {
    MITK_WARN << "Cannot clear feedback contour at current time step. Feedback contour is in invalid state as its time geometry does not support current selected time point. Invalid time point: " << this->GetLastTimePointTriggered();
    return;
  }

  auto feedbackTimeStep = m_FeedbackContour->GetTimeGeometry()->TimePointToTimeStep(this->GetLastTimePointTriggered());
  m_FeedbackContour->Clear(feedbackTimeStep);
  m_FeedbackContour->SetClosed(isClosed, feedbackTimeStep);
}

void mitk::FeedbackContourTool::UpdateCurrentFeedbackContour(const ContourModel* sourceModel, TimeStepType sourceTimeStep)
{
  if (nullptr == sourceModel)
    return;

  if (!m_FeedbackContour->GetTimeGeometry()->IsValidTimePoint(this->GetLastTimePointTriggered()))
  {
    MITK_WARN << "Cannot update feedback contour. Feedback contour is in invalid state as its time geometry does not support current selected time point. Invalid time point: "<<this->GetLastTimePointTriggered();
    return;
  }

  auto feedbackTimeStep = m_FeedbackContour->GetTimeGeometry()->TimePointToTimeStep(this->GetLastTimePointTriggered());
  this->UpdateFeedbackContour(sourceModel, feedbackTimeStep, sourceTimeStep);
}

void mitk::FeedbackContourTool::UpdateFeedbackContour(const ContourModel* sourceModel, TimeStepType feedbackTimeStep, TimeStepType sourceTimeStep)
{
  if (nullptr == sourceModel)
    return;

  if (!sourceModel->GetTimeGeometry()->IsValidTimeStep(sourceTimeStep))
  {
    MITK_WARN << "Cannot update feedback contour. Source contour time geometry does not support passed time step. Invalid time step: " << sourceTimeStep;
    return;
  }

  if (!m_FeedbackContour->GetTimeGeometry()->IsValidTimeStep(feedbackTimeStep))
  {
    MITK_WARN << "Cannot update feedback contour. Feedback contour time geometry does not support passed time step. Invalid time step: "<<feedbackTimeStep;
    return;
  }

  m_FeedbackContour->UpdateContour(sourceModel, feedbackTimeStep, sourceTimeStep);
}

void mitk::FeedbackContourTool::AddVertexToCurrentFeedbackContour(const Point3D& point)
{
  if (!m_FeedbackContour->GetTimeGeometry()->IsValidTimePoint(this->GetLastTimePointTriggered()))
  {
    MITK_WARN << "Cannot add vertex to feedback contour. Feedback contour is in invalid state as its time geometry does not support current selected time point. Invalid time point: " << this->GetLastTimePointTriggered();
    return;
  }

  auto feedbackTimeStep = m_FeedbackContour->GetTimeGeometry()->TimePointToTimeStep(this->GetLastTimePointTriggered());
  this->AddVertexToFeedbackContour(point, feedbackTimeStep);
};

/** Adds a vertex to the feedback contour for the passed time step. If time step is invalid, nothing will be added.*/
void mitk::FeedbackContourTool::AddVertexToFeedbackContour(const Point3D& point, TimeStepType feedbackTimeStep)
{
  if (!m_FeedbackContour->GetTimeGeometry()->IsValidTimeStep(feedbackTimeStep))
  {
    MITK_WARN << "Cannot add vertex to feedback contour. Feedback contour time geometry does not support passed time step. Invalid time step: " << feedbackTimeStep;
    return;
  }

  m_FeedbackContour->AddVertex(point, feedbackTimeStep);
}


void mitk::FeedbackContourTool::SetFeedbackContourVisible(bool visible)
{
  if (m_FeedbackContourVisible == visible)
    return; // nothing changed

  if (DataStorage *storage = this->GetToolManager()->GetDataStorage())
  {
    if (visible)
    {
      // Add the feedback contour node as a derived node of the first working data.
      // If there is no working data, the node is added at the top level.
      storage->Add(m_FeedbackContourNode, this->GetWorkingDataNode());
    }
    else
    {
      storage->Remove(m_FeedbackContourNode);
    }
  }

  m_FeedbackContourVisible = visible;
}

mitk::ContourModel::Pointer mitk::FeedbackContourTool::ProjectContourTo2DSlice(const Image *slice,
                                                                               const ContourModel *contourIn3D)
{
  return mitk::ContourModelUtils::ProjectContourTo2DSlice(slice, contourIn3D);
}

mitk::ContourModel::Pointer mitk::FeedbackContourTool::BackProjectContourFrom2DSlice(const BaseGeometry *sliceGeometry,
                                                                                     const ContourModel *contourIn2D)
{
  return mitk::ContourModelUtils::BackProjectContourFrom2DSlice(sliceGeometry, contourIn2D);
}

mitk::Image::Pointer mitk::FeedbackContourTool::GenerateSliceWithContourUpdate(const MultiLabelSegmentation* workingSeg, const PlaneGeometry* sliceGeometry,
  const ContourModel* contour, MultiLabelSegmentation::LabelValueType labelValue, TimePointType timePoint, bool addMode)
{

  if (!workingSeg || !sliceGeometry)
    return nullptr;

  const auto* abstractTransformGeometry(
    dynamic_cast<const AbstractTransformGeometry*>(sliceGeometry));
  if (nullptr != abstractTransformGeometry)
  {
    MITK_ERROR << "GenerateSliceWithContourUpdate does not support non planar geometries." << std::endl;
    return nullptr;
  }

  const auto groupIndex = workingSeg->GetGroupIndexOfLabel(labelValue);
  const auto groupImage = workingSeg->GetGroupImage(groupIndex);
  auto resultSlice = this->GetAffectedImageSliceAs2DImageByTimePoint(sliceGeometry, groupImage, timePoint)->Clone();

  const auto activeLabelValue = addMode ? labelValue : MultiLabelSegmentation::UNLABELED_VALUE;

  if (resultSlice.IsNull())
  {
    MITK_ERROR << "Unable to extract slice." << std::endl;
    return nullptr;
  }

  ContourModel::Pointer projectedContour = FeedbackContourTool::ProjectContourTo2DSlice(
    resultSlice, contour);

  if (projectedContour.IsNull())
    return nullptr;

  auto contourTimeStep = contour->GetTimeGeometry()->TimePointToTimeStep(timePoint);

  Image::Pointer adaptedSlice = resultSlice->Clone();

  ContourModelUtils::FillContourInSlice2(projectedContour, contourTimeStep, adaptedSlice, activeLabelValue);
  TransferLabelContentAtTimeStep(adaptedSlice.GetPointer(), resultSlice.GetPointer(),
    workingSeg->GetConstLabelsByValue(workingSeg->GetLabelValuesByGroup(groupIndex)),
    0, MultiLabelSegmentation::UNLABELED_VALUE,MultiLabelSegmentation::UNLABELED_VALUE,
    false, {{labelValue, labelValue}});

  return resultSlice;
}

void mitk::FeedbackContourTool::WriteBackFeedbackContourAsSegmentationResult(const InteractionPositionEvent* positionEvent, MultiLabelSegmentation::LabelValueType labelValue, bool addMode, bool setInvisibleAfterSuccess)
{
  if (!positionEvent)
    return;

  auto workingSeg = this->GetWorkingData();
  const auto planeGeometry = positionEvent->GetSender()->GetCurrentWorldPlaneGeometry();

  if (!workingSeg || !planeGeometry)
    return;

  const auto feedbackContour = FeedbackContourTool::GetFeedbackContour();
  auto contourTimePoint = positionEvent->GetSender()->GetTime();

  auto slice = this->GenerateSliceWithContourUpdate(workingSeg, planeGeometry, feedbackContour, labelValue, contourTimePoint, addMode);

  if (slice.IsNull())
  {
    MITK_ERROR << "Unable to update slice." << std::endl;
    return;
  }

  this->WriteBackSegmentationResult(positionEvent, slice);

  if (setInvisibleAfterSuccess)
  {
    this->SetFeedbackContourVisible(false);
  }
}
