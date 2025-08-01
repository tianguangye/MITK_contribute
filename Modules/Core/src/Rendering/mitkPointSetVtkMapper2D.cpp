/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkPointSetVtkMapper2D.h"

// mitk includes
#include "mitkVtkPropRenderer.h"
#include <mitkDataNode.h>
#include <mitkPlaneGeometry.h>
#include <mitkPointSet.h>
#include <mitkProperties.h>

// vtk includes
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropAssembly.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <cstdlib>

namespace
{
  double GetScreenResolution(const mitk::BaseRenderer* renderer)
  {
    if (nullptr == renderer)
      return 1.0;

    mitk::Point2D pD1, pD2;
    pD1[0] = 0.0;
    pD1[1] = 0.0;
    pD2[0] = 0.0;
    pD2[1] = 1.0;

    // Calculate world coordinates of in-plane screen pixels (0, 0) and (0, 1).
    mitk::Point3D pW1, pW2;
    renderer->DisplayToWorld(pD1, pW1);
    renderer->DisplayToWorld(pD2, pW2);

    // For 2D renderers, the distance between these points is the screen resolution.
    return pW1.EuclideanDistanceTo(pW2);
  }

  bool GetColorFromProperty(const mitk::DataNode* node, const std::string& propertyName, mitk::BaseRenderer* renderer, std::array<double, 4>& color)
  {
    std::array<float, 3> propertyColor;
    bool foundColorProperty = node->GetColor(propertyColor.data(), renderer, propertyName.c_str());

    if (!foundColorProperty && renderer != nullptr)
      foundColorProperty = node->GetColor(propertyColor.data(), nullptr, propertyName.c_str());

    if (!foundColorProperty)
      return false;

    color[0] = propertyColor[0];
    color[1] = propertyColor[1];
    color[2] = propertyColor[2];
    color[3] = 1.0;

    return true;
  }
}

// constructor LocalStorage
mitk::PointSetVtkMapper2D::LocalStorage::LocalStorage()
{
  // points
  m_UnselectedPoints = vtkSmartPointer<vtkPoints>::New();
  m_SelectedPoints = vtkSmartPointer<vtkPoints>::New();
  m_ContourPoints = vtkSmartPointer<vtkPoints>::New();

  // scales
  m_UnselectedScales = vtkSmartPointer<vtkFloatArray>::New();
  m_SelectedScales = vtkSmartPointer<vtkFloatArray>::New();
  m_SelectedContourScales = vtkSmartPointer<vtkFloatArray>::New();

  // distances
  m_DistancesBetweenPoints = vtkSmartPointer<vtkFloatArray>::New();

  // lines
  m_ContourLines = vtkSmartPointer<vtkCellArray>::New();

  // glyph source (provides the different shapes)
  m_UnselectedGlyphSource2D = vtkSmartPointer<vtkGlyphSource2D>::New();
  m_SelectedGlyphSource2D = vtkSmartPointer<vtkGlyphSource2D>::New();
  m_SelectedContourGlyphSource2D = vtkSmartPointer<vtkGlyphSource2D>::New();

  // glyphs
  m_UnselectedGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();
  m_SelectedGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();
  m_SelectedContourGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();

  // polydata
  m_VtkUnselectedPointListPolyData = vtkSmartPointer<vtkPolyData>::New();
  m_VtkSelectedPointListPolyData = vtkSmartPointer<vtkPolyData>::New();
  m_VtkSelectedContourPointListPolyData = vtkSmartPointer<vtkPolyData>::New();
  m_VtkContourPolyData = vtkSmartPointer<vtkPolyData>::New();

  // actors
  m_UnselectedActor = vtkSmartPointer<vtkActor>::New();
  m_SelectedActor = vtkSmartPointer<vtkActor>::New();
  m_SelectedContourActor = vtkSmartPointer<vtkActor>::New();
  m_ContourActor = vtkSmartPointer<vtkActor>::New();

  // mappers
  m_VtkUnselectedPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_VtkSelectedPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_VtkSelectedContourPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_VtkContourPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();

  // propassembly
  m_PropAssembly = vtkSmartPointer<vtkPropAssembly>::New();
}
// destructor LocalStorage
mitk::PointSetVtkMapper2D::LocalStorage::~LocalStorage()
{
}

// input for this mapper ( = point set)
const mitk::PointSet *mitk::PointSetVtkMapper2D::GetInput() const
{
  return static_cast<const mitk::PointSet *>(GetDataNode()->GetData());
}

// constructor PointSetVtkMapper2D
mitk::PointSetVtkMapper2D::PointSetVtkMapper2D()
  : m_ShowContour(false),
    m_CloseContour(false),
    m_ShowPoints(true),
    m_ShowDistances(false),
    m_DistancesDecimalDigits(1),
    m_ShowAngles(false),
    m_ShowDistantLines(false),
    m_LineWidth(1),
    m_PointLineWidth(1),
    m_Point2DSize(6),
    m_IDShapeProperty(mitk::PointSetShapeProperty::CROSS),
    m_FillShape(false),
    m_DistanceToPlane(4.0f),
    m_FixedSizeOnScreen(false),
    m_Resolution(8),
    m_KeepShapeWhenSelected(false),
    m_ShowSelectedContour(false)
{
}

// destructor
mitk::PointSetVtkMapper2D::~PointSetVtkMapper2D()
{
}

// reset mapper so that nothing is displayed e.g. toggle visibility of the propassembly
void mitk::PointSetVtkMapper2D::ResetMapper(BaseRenderer *renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
  ls->m_PropAssembly->VisibilityOff();
}

// returns propassembly
vtkProp *mitk::PointSetVtkMapper2D::GetVtkProp(mitk::BaseRenderer *renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
  return ls->m_PropAssembly;
}

static bool makePerpendicularVector2D(const mitk::Vector2D &in, mitk::Vector2D &out)
{
  // The dot product of orthogonal vectors is zero.
  // In two dimensions the slopes of perpendicular lines are negative reciprocals.
  if ((fabs(in[0]) > 0) && ((fabs(in[0]) > fabs(in[1])) || (in[1] == 0)))
  {
    // negative reciprocal
    out[0] = -in[1] / in[0];
    out[1] = 1;
    out.Normalize();
    return true;
  }
  else if (fabs(in[1]) > 0)
  {
    out[0] = 1;
    // negative reciprocal
    out[1] = -in[0] / in[1];
    out.Normalize();
    return true;
  }
  else
    return false;
}

void mitk::PointSetVtkMapper2D::CreateVTKRenderObjects(mitk::BaseRenderer *renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);

  unsigned i = 0;

  // The vtk text actors need to be removed manually from the propassembly
  // since the same vtk text actors are not overwritten within this function,
  // but new actors are added to the propassembly each time this function is executed.
  // Thus, the actors from the last call must be removed in the beginning.
  for (i = 0; i < ls->m_VtkTextLabelActors.size(); i++)
  {
    if (ls->m_PropAssembly->GetParts()->IsItemPresent(ls->m_VtkTextLabelActors.at(i)))
      ls->m_PropAssembly->RemovePart(ls->m_VtkTextLabelActors.at(i));
  }

  for (i = 0; i < ls->m_VtkTextDistanceActors.size(); i++)
  {
    if (ls->m_PropAssembly->GetParts()->IsItemPresent(ls->m_VtkTextDistanceActors.at(i)))
      ls->m_PropAssembly->RemovePart(ls->m_VtkTextDistanceActors.at(i));
  }

  for (i = 0; i < ls->m_VtkTextAngleActors.size(); i++)
  {
    if (ls->m_PropAssembly->GetParts()->IsItemPresent(ls->m_VtkTextAngleActors.at(i)))
      ls->m_PropAssembly->RemovePart(ls->m_VtkTextAngleActors.at(i));
  }

  // initialize polydata here, otherwise we have update problems when
  // executing this function again
  ls->m_VtkUnselectedPointListPolyData = vtkSmartPointer<vtkPolyData>::New();
  ls->m_VtkSelectedPointListPolyData = vtkSmartPointer<vtkPolyData>::New();
  ls->m_VtkContourPolyData = vtkSmartPointer<vtkPolyData>::New();

  // get input point set and update the PointSet
  mitk::PointSet::Pointer input = const_cast<mitk::PointSet *>(this->GetInput());

  // only update the input data, if the property tells us to
  bool update = true;
  this->GetDataNode()->GetBoolProperty("updateDataOnRender", update);
  if (update == true)
    input->Update();

  int timestep = this->GetTimestep();
  mitk::PointSet::DataType::Pointer itkPointSet = input->GetPointSet(timestep);

  if (itkPointSet.GetPointer() == nullptr)
  {
    ls->m_PropAssembly->VisibilityOff();
    return;
  }

  // iterator for point set
  mitk::PointSet::PointsContainer::Iterator pointsIter = itkPointSet->GetPoints()->Begin();

  // PointDataContainer has additional information to each point, e.g. whether
  // it is selected or not
  mitk::PointSet::PointDataContainer::Iterator pointDataIter;
  pointDataIter = itkPointSet->GetPointData()->Begin();

  // check if the list for the PointDataContainer is the same size as the PointsContainer.
  // If not, then the points were inserted manually and can not be visualized according to the PointData
  // (selected/unselected)
  bool pointDataBroken = (itkPointSet->GetPointData()->Size() != itkPointSet->GetPoints()->Size());

  if (itkPointSet->GetPointData()->size() == 0 || pointDataBroken)
  {
    ls->m_PropAssembly->VisibilityOff();
    return;
  }

  ls->m_PropAssembly->VisibilityOn();

  // empty point sets, cellarrays, scalars
  ls->m_UnselectedPoints->Reset();
  ls->m_SelectedPoints->Reset();

  ls->m_ContourPoints->Reset();
  ls->m_ContourLines->Reset();

  ls->m_UnselectedScales->Reset();
  ls->m_SelectedScales->Reset();
  ls->m_SelectedContourScales->Reset();

  ls->m_DistancesBetweenPoints->Reset();

  ls->m_VtkTextLabelActors.clear();
  ls->m_VtkTextDistanceActors.clear();
  ls->m_VtkTextAngleActors.clear();

  ls->m_UnselectedScales->SetNumberOfComponents(3);
  ls->m_SelectedScales->SetNumberOfComponents(3);
  ls->m_SelectedContourScales->SetNumberOfComponents(3);

  int NumberContourPoints = 0;
  bool pointsOnSameSideOfPlane = false;

  const int text2dDistance = 10;

  // initialize points with a random start value

  // current point in point set
  itk::Point<ScalarType> point = pointsIter->Value();

  mitk::Point3D p = point;     // currently visited point
  mitk::Point3D lastP = point; // last visited point (predecessor in point set of "point")
  mitk::Vector3D vec;          // p - lastP
  mitk::Vector3D lastVec;      // lastP - point before lastP
  vec.Fill(0.0);
  lastVec.Fill(0.0);

  mitk::Point2D pt2d;
  pt2d[0] = point[0]; // projected_p in display coordinates
  pt2d[1] = point[1];
  mitk::Point2D lastPt2d = pt2d;    // last projected_p in display coordinates (predecessor in point set of "pt2d")
  mitk::Point2D preLastPt2d = pt2d; // projected_p in display coordinates before lastPt2

  const mitk::PlaneGeometry *geo2D = renderer->GetCurrentWorldPlaneGeometry();
  double resolution = GetScreenResolution(renderer);

  vtkLinearTransform *dataNodeTransform = input->GetGeometry()->GetVtkTransform();

  int count = 0;

  for (pointsIter = itkPointSet->GetPoints()->Begin(); pointsIter != itkPointSet->GetPoints()->End(); pointsIter++)
  {
    lastP = p;              // valid for number of points count > 0
    preLastPt2d = lastPt2d; // valid only for count > 1
    lastPt2d = pt2d;        // valid for number of points count > 0

    lastVec = vec; // valid only for counter > 1

    // get current point in point set
    point = pointsIter->Value();

    // transform point
    {
      float vtkp[3];
      itk2vtk(point, vtkp);
      dataNodeTransform->TransformPoint(vtkp, vtkp);
      vtk2itk(vtkp, point);
    }

    p[0] = point[0];
    p[1] = point[1];
    p[2] = point[2];

    renderer->WorldToDisplay(p, pt2d);

    vec = p - lastP; // valid only for counter > 0

    // compute distance to current plane
    float dist = geo2D->Distance(point);
    // measure distance in screen pixel units if requested
    if (m_FixedSizeOnScreen)
    {
      dist /= resolution;
    }

    // draw markers on slices a certain distance away from the points
    // location according to the tolerance threshold (m_DistanceToPlane)
    if (dist < m_DistanceToPlane)
    {
      // is point selected or not?
      if (pointDataIter->Value().selected)
      {
        ls->m_SelectedPoints->InsertNextPoint(point[0], point[1], point[2]);
        // point is scaled according to its distance to the plane
        ls->m_SelectedScales->InsertNextTuple3(
            std::max(0.0f, m_Point2DSize - (2 * dist)), 0, 0);
        ls->m_SelectedContourScales->InsertNextTuple3(
          std::max(0.0f, m_Point2DSize + 0.125f - (2 * dist)), 0, 0);
      }
      else
      {
        ls->m_UnselectedPoints->InsertNextPoint(point[0], point[1], point[2]);
        // point is scaled according to its distance to the plane
        ls->m_UnselectedScales->InsertNextTuple3(
            std::max(0.0f, m_Point2DSize - (2 * dist)), 0, 0);
      }

      //---- LABEL -----//
      // paint label for each point if available
      if (dynamic_cast<mitk::StringProperty *>(this->GetDataNode()->GetProperty("label")) != nullptr)
      {
        const char *pointLabel =
          dynamic_cast<mitk::StringProperty *>(this->GetDataNode()->GetProperty("label"))->GetValue();
        std::string l = pointLabel;
        if (input->GetSize() > 1)
        {
          std::stringstream ss;
          ss << pointsIter->Index();
          l.append(ss.str());
        }

        ls->m_VtkTextActor = vtkSmartPointer<vtkTextActor>::New();

        ls->m_VtkTextActor->SetDisplayPosition(pt2d[0] + text2dDistance, pt2d[1] + text2dDistance);
        ls->m_VtkTextActor->SetInput(l.c_str());
        ls->m_VtkTextActor->GetTextProperty()->SetOpacity(100);

        float unselectedColor[4] = {1.0, 1.0, 0.0, 1.0};

        // check if there is a color property
        GetDataNode()->GetColor(unselectedColor);

        ls->m_VtkTextActor->GetTextProperty()->SetColor(unselectedColor[0], unselectedColor[1], unselectedColor[2]);

        ls->m_VtkTextLabelActors.push_back(ls->m_VtkTextActor);
      }
    }

    // draw contour, distance text and angle text in render window

    // lines between points, which intersect the current plane, are drawn
    if (m_ShowContour && count > 0)
    {
      ScalarType distance = renderer->GetCurrentWorldPlaneGeometry()->SignedDistance(point);
      ScalarType lastDistance = renderer->GetCurrentWorldPlaneGeometry()->SignedDistance(lastP);

      pointsOnSameSideOfPlane = (distance * lastDistance) > 0.5;

      // Points must be on different side of plane in order to draw a contour.
      // If "show distant lines" is enabled this condition is disregarded.
      if (!pointsOnSameSideOfPlane || m_ShowDistantLines)
      {
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();

        ls->m_ContourPoints->InsertNextPoint(lastP[0], lastP[1], lastP[2]);
        line->GetPointIds()->SetId(0, NumberContourPoints);
        NumberContourPoints++;

        ls->m_ContourPoints->InsertNextPoint(point[0], point[1], point[2]);
        line->GetPointIds()->SetId(1, NumberContourPoints);
        NumberContourPoints++;

        ls->m_ContourLines->InsertNextCell(line);

        if (m_ShowDistances) // calculate and print distance between adjacent points
        {
          float distancePoints = point.EuclideanDistanceTo(lastP);

          std::stringstream buffer;
          buffer << std::fixed << std::setprecision(m_DistancesDecimalDigits) << distancePoints << " mm";

          // compute desired display position of text
          Vector2D vec2d = pt2d - lastPt2d;
          makePerpendicularVector2D(vec2d,
                                    vec2d); // text is rendered within text2dDistance perpendicular to current line
          Vector2D pos2d = (lastPt2d.GetVectorFromOrigin() + pt2d.GetVectorFromOrigin()) * 0.5 + vec2d * text2dDistance;

          ls->m_VtkTextActor = vtkSmartPointer<vtkTextActor>::New();

          ls->m_VtkTextActor->SetDisplayPosition(pos2d[0], pos2d[1]);
          ls->m_VtkTextActor->SetInput(buffer.str().c_str());
          ls->m_VtkTextActor->GetTextProperty()->SetColor(0.0, 1.0, 0.0);

          ls->m_VtkTextDistanceActors.push_back(ls->m_VtkTextActor);
        }

        if (m_ShowAngles && count > 1) // calculate and print angle between connected lines
        {
          std::stringstream buffer;
          buffer << angle(vec.GetVnlVector(), -lastVec.GetVnlVector()) * 180 / vnl_math::pi << "°";

          // compute desired display position of text
          Vector2D vec2d = pt2d - lastPt2d; // first arm enclosing the angle
          vec2d.Normalize();
          Vector2D lastVec2d = lastPt2d - preLastPt2d; // second arm enclosing the angle
          lastVec2d.Normalize();
          vec2d = vec2d - lastVec2d; // vector connecting both arms
          vec2d.Normalize();

          // middle between two vectors that enclose the angle
          Vector2D pos2d = lastPt2d.GetVectorFromOrigin() + vec2d * text2dDistance * text2dDistance;

          ls->m_VtkTextActor = vtkSmartPointer<vtkTextActor>::New();

          ls->m_VtkTextActor->SetDisplayPosition(pos2d[0], pos2d[1]);
          ls->m_VtkTextActor->SetInput(buffer.str().c_str());
          ls->m_VtkTextActor->GetTextProperty()->SetColor(0.0, 1.0, 0.0);

          ls->m_VtkTextAngleActors.push_back(ls->m_VtkTextActor);
        }
      }
    }

    if (pointDataIter != itkPointSet->GetPointData()->End())
    {
      pointDataIter++;
      count++;
    }
  }

  // add each single text actor to the assembly
  for (i = 0; i < ls->m_VtkTextLabelActors.size(); i++)
  {
    ls->m_PropAssembly->AddPart(ls->m_VtkTextLabelActors.at(i));
  }

  for (i = 0; i < ls->m_VtkTextDistanceActors.size(); i++)
  {
    ls->m_PropAssembly->AddPart(ls->m_VtkTextDistanceActors.at(i));
  }

  for (i = 0; i < ls->m_VtkTextAngleActors.size(); i++)
  {
    ls->m_PropAssembly->AddPart(ls->m_VtkTextAngleActors.at(i));
  }

  //---- CONTOUR -----//

  // create lines between the points which intersect the plane
  if (m_ShowContour)
  {
    // draw line between first and last point which is rendered
    if (m_CloseContour && NumberContourPoints > 1)
    {
      vtkSmartPointer<vtkLine> closingLine = vtkSmartPointer<vtkLine>::New();
      closingLine->GetPointIds()->SetId(0, 0);                       // index of first point
      closingLine->GetPointIds()->SetId(1, NumberContourPoints - 1); // index of last point
      ls->m_ContourLines->InsertNextCell(closingLine);
    }

    ls->m_VtkContourPolyData->SetPoints(ls->m_ContourPoints);
    ls->m_VtkContourPolyData->SetLines(ls->m_ContourLines);

    ls->m_VtkContourPolyDataMapper->SetInputData(ls->m_VtkContourPolyData);
    ls->m_ContourActor->SetMapper(ls->m_VtkContourPolyDataMapper);
    ls->m_ContourActor->GetProperty()->SetLineWidth(m_LineWidth);

    ls->m_PropAssembly->AddPart(ls->m_ContourActor);
  }

  // the point set must be transformed in order to obtain the appropriate glyph orientation
  // according to the current view
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkMatrix4x4> a, b = vtkSmartPointer<vtkMatrix4x4>::New();

  a = geo2D->GetVtkTransform()->GetMatrix();
  b->DeepCopy(a);

  // delete transformation from matrix, only take orientation
  b->SetElement(3, 3, 1);
  b->SetElement(2, 3, 0);
  b->SetElement(1, 3, 0);
  b->SetElement(0, 3, 0);
  b->SetElement(3, 2, 0);
  b->SetElement(3, 1, 0);
  b->SetElement(3, 0, 0);

  Vector3D spacing = geo2D->GetSpacing();

  // If you find a way to simplify the following, feel free to change!
  b->SetElement(0, 0, b->GetElement(0, 0) / spacing[0]);
  b->SetElement(1, 0, b->GetElement(1, 0) / spacing[0]);
  b->SetElement(2, 0, b->GetElement(2, 0) / spacing[0]);
  b->SetElement(1, 1, b->GetElement(1, 1) / spacing[1]);
  b->SetElement(2, 1, b->GetElement(2, 1) / spacing[1]);

  b->SetElement(0, 2, b->GetElement(0, 2) / spacing[2]);
  b->SetElement(1, 2, b->GetElement(1, 2) / spacing[2]);
  b->SetElement(2, 2, b->GetElement(2, 2) / spacing[2]);

  transform->SetMatrix(b);

  //---- UNSELECTED POINTS  -----//

  // apply properties to glyph
  ls->m_UnselectedGlyphSource2D->SetGlyphType(m_IDShapeProperty);
  ls->m_UnselectedGlyphSource2D->SetResolution(m_Resolution);
  ls->m_UnselectedGlyphSource2D->SetFilled(m_FillShape);

  // apply transform
  vtkSmartPointer<vtkTransformFilter> transformFilterU = vtkSmartPointer<vtkTransformFilter>::New();
  transformFilterU->SetInputConnection(ls->m_UnselectedGlyphSource2D->GetOutputPort());
  transformFilterU->SetTransform(transform);

  ls->m_VtkUnselectedPointListPolyData->SetPoints(ls->m_UnselectedPoints);
  ls->m_VtkUnselectedPointListPolyData->GetPointData()->SetVectors(ls->m_UnselectedScales);

  // apply transform of current plane to glyphs
  ls->m_UnselectedGlyph3D->SetSourceConnection(transformFilterU->GetOutputPort());
  ls->m_UnselectedGlyph3D->SetInputData(ls->m_VtkUnselectedPointListPolyData);
  ls->m_UnselectedGlyph3D->SetScaleFactor(m_FixedSizeOnScreen ? resolution : 1.0);
  ls->m_UnselectedGlyph3D->SetScaleModeToScaleByVector();
  ls->m_UnselectedGlyph3D->SetVectorModeToUseVector();

  ls->m_VtkUnselectedPolyDataMapper->SetInputConnection(ls->m_UnselectedGlyph3D->GetOutputPort());
  ls->m_UnselectedActor->SetMapper(ls->m_VtkUnselectedPolyDataMapper);
  ls->m_UnselectedActor->GetProperty()->SetLineWidth(m_PointLineWidth);

  ls->m_PropAssembly->AddPart(ls->m_UnselectedActor);

  //---- SELECTED POINTS  -----//

  if (m_KeepShapeWhenSelected)
  {
    ls->m_SelectedGlyphSource2D->SetGlyphType(m_IDShapeProperty);
    ls->m_SelectedGlyphSource2D->SetResolution(m_Resolution);
    ls->m_SelectedGlyphSource2D->SetFilled(m_FillShape);
  }
  else
  {
    ls->m_SelectedGlyphSource2D->SetGlyphTypeToDiamond();
    ls->m_SelectedGlyphSource2D->CrossOn();
    ls->m_SelectedGlyphSource2D->FilledOff();
  }

  // apply transform
  vtkSmartPointer<vtkTransformFilter> transformFilterS = vtkSmartPointer<vtkTransformFilter>::New();
  transformFilterS->SetInputConnection(ls->m_SelectedGlyphSource2D->GetOutputPort());
  transformFilterS->SetTransform(transform);

  ls->m_VtkSelectedPointListPolyData->SetPoints(ls->m_SelectedPoints);
  ls->m_VtkSelectedPointListPolyData->GetPointData()->SetVectors(ls->m_SelectedScales);

  // apply transform of current plane to glyphs
  ls->m_SelectedGlyph3D->SetSourceConnection(transformFilterS->GetOutputPort());
  ls->m_SelectedGlyph3D->SetInputData(ls->m_VtkSelectedPointListPolyData);
  ls->m_SelectedGlyph3D->SetScaleFactor(m_FixedSizeOnScreen ? resolution : 1.0);
  ls->m_SelectedGlyph3D->SetScaleModeToScaleByVector();
  ls->m_SelectedGlyph3D->SetVectorModeToUseVector();

  ls->m_VtkSelectedPolyDataMapper->SetInputConnection(ls->m_SelectedGlyph3D->GetOutputPort());
  ls->m_SelectedActor->SetMapper(ls->m_VtkSelectedPolyDataMapper);
  ls->m_SelectedActor->GetProperty()->SetLineWidth(m_PointLineWidth);

  ls->m_PropAssembly->AddPart(ls->m_SelectedActor);

  if (m_ShowSelectedContour)
  {
    ls->m_SelectedContourGlyphSource2D->SetGlyphType(ls->m_SelectedGlyphSource2D->GetGlyphType());
    ls->m_SelectedContourGlyphSource2D->SetResolution(ls->m_SelectedGlyphSource2D->GetResolution());
    ls->m_SelectedContourGlyphSource2D->FilledOff();

    // apply transform
    vtkSmartPointer<vtkTransformFilter> transformFilterSC = vtkSmartPointer<vtkTransformFilter>::New();
    transformFilterSC->SetInputConnection(ls->m_SelectedContourGlyphSource2D->GetOutputPort());
    transformFilterSC->SetTransform(transform);

    ls->m_VtkSelectedContourPointListPolyData->SetPoints(ls->m_SelectedPoints);
    ls->m_VtkSelectedContourPointListPolyData->GetPointData()->SetVectors(ls->m_SelectedContourScales);

    // apply transform of current plane to glyphs
    ls->m_SelectedContourGlyph3D->SetSourceConnection(transformFilterSC->GetOutputPort());
    ls->m_SelectedContourGlyph3D->SetInputData(ls->m_VtkSelectedContourPointListPolyData);
    ls->m_SelectedContourGlyph3D->SetScaleFactor(m_FixedSizeOnScreen ? resolution : 1.0);
    ls->m_SelectedContourGlyph3D->SetScaleModeToScaleByVector();
    ls->m_SelectedContourGlyph3D->SetVectorModeToUseVector();

    ls->m_VtkSelectedContourPolyDataMapper->SetInputConnection(ls->m_SelectedContourGlyph3D->GetOutputPort());
    ls->m_SelectedContourActor->SetMapper(ls->m_VtkSelectedContourPolyDataMapper);
    ls->m_SelectedContourActor->GetProperty()->SetLineWidth(m_PointLineWidth);

    ls->m_PropAssembly->AddPart(ls->m_SelectedContourActor);
  }
}

void mitk::PointSetVtkMapper2D::GenerateDataForRenderer(mitk::BaseRenderer *renderer)
{
  const mitk::DataNode *node = GetDataNode();
  if (node == nullptr)
    return;

  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);

  // check whether the input data has been changed
  bool needGenerateData = ls->IsGenerateDataRequired(renderer, this, GetDataNode());

  // toggle visibility
  bool visible = true;
  node->GetVisibility(visible, renderer, "visible");
  if (!visible)
  {
    ls->m_UnselectedActor->VisibilityOff();
    ls->m_SelectedActor->VisibilityOff();
    ls->m_SelectedContourActor->VisibilityOff();
    ls->m_ContourActor->VisibilityOff();
    ls->m_PropAssembly->VisibilityOff();
    return;
  }
  else
  {
    ls->m_PropAssembly->VisibilityOn();
  }

  node->GetBoolProperty("show contour", m_ShowContour, renderer);
  node->GetBoolProperty("close contour", m_CloseContour, renderer);
  node->GetBoolProperty("show points", m_ShowPoints, renderer);
  node->GetBoolProperty("show distances", m_ShowDistances, renderer);
  node->GetIntProperty("distance decimal digits", m_DistancesDecimalDigits, renderer);
  node->GetBoolProperty("show angles", m_ShowAngles, renderer);
  node->GetBoolProperty("show distant lines", m_ShowDistantLines, renderer);
  node->GetIntProperty("line width", m_LineWidth, renderer);
  node->GetIntProperty("point line width", m_PointLineWidth, renderer);
  if (!node->GetFloatProperty(
        "point 2D size", m_Point2DSize, renderer)) // re-defined to float 2015-08-13, keep a fallback
  {
    int oldPointSize = m_Point2DSize;
    if (node->GetIntProperty("point 2D size", oldPointSize, renderer))
    {
      m_Point2DSize = oldPointSize;
    }
  }
  node->GetBoolProperty("Pointset.2D.fill shape", m_FillShape, renderer);
  node->GetFloatProperty("Pointset.2D.distance to plane", m_DistanceToPlane, renderer);
  node->GetBoolProperty("Pointset.2D.fixed size on screen", m_FixedSizeOnScreen, renderer);
  node->GetIntProperty("Pointset.2D.resolution", m_Resolution, renderer);
  node->GetBoolProperty("Pointset.2D.keep shape when selected", m_KeepShapeWhenSelected, renderer);
  node->GetBoolProperty("Pointset.2D.selected.show contour", m_ShowSelectedContour, renderer);

  mitk::PointSetShapeProperty::Pointer shape =
    dynamic_cast<mitk::PointSetShapeProperty *>(this->GetDataNode()->GetProperty("Pointset.2D.shape", renderer));
  if (shape.IsNotNull())
  {
    m_IDShapeProperty = shape->GetPointSetShape();
  }

  // check for color props and use it for rendering of selected/unselected points and contour
  // due to different params in VTK (double/float) we have to convert

  float opacity = 1.0;

  GetDataNode()->GetOpacity(opacity, renderer);

  // apply color and opacity
  if (m_ShowPoints)
  {
    float unselectedColor[4];
    std::array<double, 4> selectedColor = {1.0, 0.0, 0.0, 1.0}; // red
    std::array<double, 4> selectedContourColor = { 0.125, 0.625, 0.875, 1.0 };

    ls->m_UnselectedActor->VisibilityOn();
    ls->m_SelectedActor->VisibilityOn();
    ls->m_SelectedContourActor->SetVisibility(m_ShowSelectedContour);

    // check if there is a color property
    GetDataNode()->GetColor(unselectedColor);

    GetColorFromProperty(node, "selectedcolor", renderer, selectedColor);
    GetColorFromProperty(node, "Pointset.2D.selected.contour color", renderer, selectedContourColor);

    ls->m_SelectedContourActor->GetProperty()->SetColor(selectedContourColor.data());
    ls->m_SelectedContourActor->GetProperty()->SetOpacity(opacity);

    ls->m_SelectedActor->GetProperty()->SetColor(selectedColor.data());
    ls->m_SelectedActor->GetProperty()->SetOpacity(opacity);

    ls->m_UnselectedActor->GetProperty()->SetColor(unselectedColor[0], unselectedColor[1], unselectedColor[2]);
    ls->m_UnselectedActor->GetProperty()->SetOpacity(opacity);
  }
  else
  {
    ls->m_UnselectedActor->VisibilityOff();
    ls->m_SelectedActor->VisibilityOff();
    ls->m_SelectedContourActor->VisibilityOff();
  }

  if (m_ShowContour)
  {
    double contourColor[4] = {1.0f, 0.0f, 0.0f, 1.0f}; // red
    ls->m_ContourActor->VisibilityOn();

    // get contour color property
    if (dynamic_cast<mitk::ColorProperty *>(
          this->GetDataNode()->GetPropertyList(renderer)->GetProperty("contourcolor")) != nullptr)
    {
      mitk::Color tmpColor =
        dynamic_cast<mitk::ColorProperty *>(this->GetDataNode()->GetPropertyList(renderer)->GetProperty("contourcolor"))
          ->GetValue();
      contourColor[0] = tmpColor[0];
      contourColor[1] = tmpColor[1];
      contourColor[2] = tmpColor[2];
      contourColor[3] = 1.0f;
    }
    else if (dynamic_cast<mitk::ColorProperty *>(
               this->GetDataNode()->GetPropertyList(nullptr)->GetProperty("contourcolor")) != nullptr)
    {
      mitk::Color tmpColor =
        dynamic_cast<mitk::ColorProperty *>(this->GetDataNode()->GetPropertyList(nullptr)->GetProperty("contourcolor"))
          ->GetValue();
      contourColor[0] = tmpColor[0];
      contourColor[1] = tmpColor[1];
      contourColor[2] = tmpColor[2];
      contourColor[3] = 1.0f;
    }

    ls->m_ContourActor->GetProperty()->SetColor(contourColor);
    ls->m_ContourActor->GetProperty()->SetOpacity(opacity);
  }
  else
  {
    ls->m_ContourActor->VisibilityOff();
  }

  if (needGenerateData)
  {
    // create new vtk render objects (e.g. a circle for a point)
    this->CreateVTKRenderObjects(renderer);
  }
}

void mitk::PointSetVtkMapper2D::SetDefaultProperties(mitk::DataNode *node, mitk::BaseRenderer *renderer, bool overwrite)
{
  node->AddProperty("line width", mitk::IntProperty::New(2), renderer, overwrite);
  node->AddProperty("point line width", mitk::IntProperty::New(1), renderer, overwrite);
  node->AddProperty("point 2D size", mitk::FloatProperty::New(6), renderer, overwrite);
  node->AddProperty("show contour", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("close contour", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("show points", mitk::BoolProperty::New(true), renderer, overwrite);
  node->AddProperty("show distances", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("distance decimal digits", mitk::IntProperty::New(2), renderer, overwrite);
  node->AddProperty("show angles", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("show distant lines", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("layer", mitk::IntProperty::New(1), renderer, overwrite);
  node->AddProperty("Pointset.2D.fill shape",
                    mitk::BoolProperty::New(false),
                    renderer,
                    overwrite); // fill or do not fill the glyph shape
  mitk::PointSetShapeProperty::Pointer pointsetShapeProperty = mitk::PointSetShapeProperty::New();
  node->AddProperty("Pointset.2D.shape", pointsetShapeProperty, renderer, overwrite);
  node->AddProperty("Pointset.2D.distance to plane",
                    mitk::FloatProperty::New(4.0f),
                    renderer,
                    overwrite); // show the point at a certain distance above/below the 2D imaging plane.
  node->AddProperty("Pointset.2D.fixed size on screen", mitk::BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("Pointset.2D.resolution", IntProperty::New(8), renderer, overwrite);
  node->AddProperty("Pointset.2D.keep shape when selected", BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("Pointset.2D.selected.show contour", BoolProperty::New(false), renderer, overwrite);
  node->AddProperty("Pointset.2D.selected.contour color", ColorProperty::New(0.125f, 0.625f, 0.875f), renderer, overwrite);

  Superclass::SetDefaultProperties(node, renderer, overwrite);
}
