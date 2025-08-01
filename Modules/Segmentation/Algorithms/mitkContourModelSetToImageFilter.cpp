/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkContourModelSetToImageFilter.h"

#include <mitkContourModelSet.h>
#include <mitkContourModelUtils.h>
#include <mitkExtractSliceFilter.h>
#include <mitkImageWriteAccessor.h>
#include <mitkProgressBar.h>
#include <mitkTimeHelper.h>
#include <mitkLabel.h>
#include <mitkVtkImageOverwrite.h>

mitk::ContourModelSetToImageFilter::ContourModelSetToImageFilter()
  : m_MakeOutputBinary(true),
    m_MakeOutputLabelPixelType(false),
    m_PaintingPixelValue(1),
    m_TimeStep(0),
    m_ReferenceImage(nullptr)
{
  // Create the output.
  itk::DataObject::Pointer output = this->MakeOutput(0);
  Superclass::SetNumberOfRequiredInputs(1);
  Superclass::SetNumberOfRequiredOutputs(1);
  Superclass::SetNthOutput(0, output);
}

mitk::ContourModelSetToImageFilter::~ContourModelSetToImageFilter()
{
}

void mitk::ContourModelSetToImageFilter::SetMakeOutputBinary(bool makeOutputBinary)
{
  if (m_MakeOutputBinary != makeOutputBinary)
  {
    m_MakeOutputBinary = makeOutputBinary;

    if (m_MakeOutputBinary)
      m_MakeOutputLabelPixelType = false;

    this->Modified();
  }
}

void mitk::ContourModelSetToImageFilter::SetMakeOutputLabelPixelType(bool makeOutputLabelPixelType)
{
  if (m_MakeOutputLabelPixelType != makeOutputLabelPixelType)
  {
    m_MakeOutputLabelPixelType = makeOutputLabelPixelType;

    if (m_MakeOutputLabelPixelType)
      m_MakeOutputBinary = false;

    this->Modified();
  }
}

void mitk::ContourModelSetToImageFilter::GenerateInputRequestedRegion()
{
  mitk::Image *output = this->GetOutput();
  if ((output->IsInitialized() == false))
    return;

  GenerateTimeInInputRegion(output, const_cast<mitk::Image *>(m_ReferenceImage));
}

void mitk::ContourModelSetToImageFilter::GenerateOutputInformation()
{
  mitk::Image::Pointer output = this->GetOutput();

  itkDebugMacro(<< "GenerateOutputInformation()");

  if ((m_ReferenceImage == nullptr) || (m_ReferenceImage->IsInitialized() == false) ||
      (m_ReferenceImage->GetTimeGeometry() == nullptr))
    return;

  if (m_MakeOutputLabelPixelType)
  {
    output->Initialize(mitk::MakeScalarPixelType<mitk::Label::PixelType>(), *m_ReferenceImage->GetTimeGeometry(), 1);
  }
  else if (m_MakeOutputBinary)
  {
    output->Initialize(mitk::MakeScalarPixelType<unsigned char>(), *m_ReferenceImage->GetTimeGeometry(), 1);
  }
  else
  {
    output->Initialize(m_ReferenceImage->GetPixelType(), *m_ReferenceImage->GetTimeGeometry());
  }

  output->SetPropertyList(m_ReferenceImage->GetPropertyList()->Clone());
}

itk::DataObject::Pointer mitk::ContourModelSetToImageFilter::MakeOutput(DataObjectPointerArraySizeType /*idx*/)
{
  return OutputType::New().GetPointer();
}

itk::DataObject::Pointer mitk::ContourModelSetToImageFilter::MakeOutput(const DataObjectIdentifierType &name)
{
  itkDebugMacro("MakeOutput(" << name << ")");
  if (this->IsIndexedOutputName(name))
  {
    return this->MakeOutput(this->MakeIndexFromOutputName(name));
  }
  return OutputType::New().GetPointer();
}

const mitk::ContourModelSet *mitk::ContourModelSetToImageFilter::GetInput(void)
{
  if (this->GetNumberOfInputs() < 1)
  {
    return nullptr;
  }

  return static_cast<const mitk::ContourModelSet *>(this->ProcessObject::GetInput(0));
}

void mitk::ContourModelSetToImageFilter::SetInput(const ContourModelSet *input)
{
  // Process object is not const-correct so the const_cast is required here
  this->ProcessObject::SetNthInput(0, const_cast<mitk::ContourModelSet *>(input));
}

void mitk::ContourModelSetToImageFilter::SetImage(const mitk::Image *refImage)
{
  m_ReferenceImage = refImage;
}

const mitk::Image *mitk::ContourModelSetToImageFilter::GetImage(void)
{
  return m_ReferenceImage;
}

void mitk::ContourModelSetToImageFilter::GenerateData()
{
  auto *contourSet = const_cast<mitk::ContourModelSet *>(this->GetInput());

  // Initializing progressbar
  unsigned int num_contours = contourSet->GetContourModelList()->size();
  mitk::ProgressBar::GetInstance()->AddStepsToDo(num_contours);

  // Assure that the volume data of the output is set (fill volume with zeros)
  this->InitializeOutputEmpty();

  mitk::Image::Pointer outputImage = const_cast<mitk::Image *>(this->GetOutput());

  if (outputImage.IsNull() || outputImage->IsInitialized() == false || !outputImage->IsVolumeSet(m_TimeStep))
  {
    mitkThrow() << "Error creating output for specified image!";
  }

  if (!contourSet || contourSet->GetContourModelList()->size() == 0)
  {
    mitkThrow() << "No contours specified!";
  }

  mitk::BaseGeometry *outputImageGeo = outputImage->GetGeometry(m_TimeStep);

  // Create mitkVtkImageOverwrite which is needed to write the slice back into the volume
  vtkSmartPointer<mitkVtkImageOverwrite> reslice = vtkSmartPointer<mitkVtkImageOverwrite>::New();

  // Create ExtractSliceFilter for extracting the corresponding slices from the volume
  mitk::ExtractSliceFilter::Pointer extractor = mitk::ExtractSliceFilter::New(reslice);
  extractor->SetInput(outputImage);
  extractor->SetTimeStep(m_TimeStep);
  extractor->SetResliceTransformByGeometry(outputImageGeo);

  // Fill each contour of the contourmodelset into the image
  auto it = contourSet->Begin();
  auto end = contourSet->End();
  while (it != end)
  {
    mitk::ContourModel *contour = it->GetPointer();

    // 1. Create slice geometry using the contour points
    mitk::PlaneGeometry::Pointer plane = mitk::PlaneGeometry::New();
    mitk::Point3D point3D, tempPoint;
    mitk::Vector3D normal;

    mitk::Image::Pointer slice;

    int sliceIndex;
    bool isFrontside = true;
    bool isRotated = false;

    // Determine plane orientation
    point3D = contour->GetVertexAt(0)->Coordinates;
    tempPoint = contour->GetVertexAt(contour->GetNumberOfVertices() * 0.25)->Coordinates;
    mitk::Vector3D vec = point3D - tempPoint;
    vec.Normalize();
    outputImageGeo->WorldToIndex(point3D, point3D);

    mitk::AnatomicalPlane orientation;
    if (mitk::Equal(vec[0], 0))
    {
      orientation = mitk::AnatomicalPlane::Sagittal;
      sliceIndex = point3D[0];
    }
    else if (mitk::Equal(vec[1], 0))
    {
      orientation = mitk::AnatomicalPlane::Coronal;
      sliceIndex = point3D[1];
    }
    else if (mitk::Equal(vec[2], 0))
    {
      orientation = mitk::AnatomicalPlane::Axial;
      sliceIndex = point3D[2];
    }
    else
    {
      // TODO Maybe rotate geometry to extract slice?
      MITK_ERROR
        << "Cannot detect correct slice number! Only axial, sagittal and coronal oriented contours are supported!";
      return;
    }

    // Initialize plane using the detected orientation
    plane->InitializeStandardPlane(outputImageGeo, orientation, sliceIndex, isFrontside, isRotated);
    point3D = plane->GetOrigin();
    normal = plane->GetNormal();
    normal.Normalize();
    point3D += normal * 0.5; // pixelspacing is 1, so half the spacing is 0.5
    plane->SetOrigin(point3D);

    // 2. Extract slice at the given position
    extractor->SetWorldGeometry(plane);
    extractor->SetVtkOutputRequest(false);
    reslice->SetOverwriteMode(false);

    extractor->Modified();
    extractor->Update();

    slice = extractor->GetOutput();
    slice->DisconnectPipeline();

    // 3. Fill contour into slice
    mitk::ContourModel::Pointer projectedContour =
      mitk::ContourModelUtils::ProjectContourTo2DSlice(slice, contour);
    mitk::ContourModelUtils::FillContourInSlice(projectedContour, 0, slice, m_PaintingPixelValue);

    // 4. Write slice back into image volume
    reslice->SetInputSlice(slice->GetVtkImageData());

    // set overwrite mode to true to write back to the image volume
    reslice->SetOverwriteMode(true);
    reslice->Modified();

    extractor->Modified();
    extractor->Update();

    reslice->SetInputSlice(nullptr);

    // Progress
    mitk::ProgressBar::GetInstance()->Progress();

    ++it;
  }

  outputImage->Modified();
  outputImage->GetVtkImageData()->Modified();
}

void mitk::ContourModelSetToImageFilter::InitializeOutputEmpty()
{
  // Initialize the output's volume with zeros
  mitk::Image *output = this->GetOutput();
  unsigned int byteSize = output->GetPixelType().GetSize();

  if (output->GetDimension() < 4)
  {
    for (unsigned int dim = 0; dim < output->GetDimension(); ++dim)
    {
      byteSize *= output->GetDimension(dim);
    }

    mitk::ImageWriteAccessor writeAccess(output, output->GetVolumeData(0));

    memset(writeAccess.GetData(), 0, byteSize);
  }
  else
  {
    // if we have a time-resolved image we need to set memory to 0 for each time step
    for (unsigned int dim = 0; dim < 3; ++dim)
    {
      byteSize *= output->GetDimension(dim);
    }

    for (unsigned int volumeNumber = 0; volumeNumber < output->GetDimension(3); volumeNumber++)
    {
      mitk::ImageWriteAccessor writeAccess(output, output->GetVolumeData(volumeNumber));

      memset(writeAccess.GetData(), 0, byteSize);
    }
  }
}

mitk::Image::Pointer mitk::ConvertContourModelSetToLabelMask(const mitk::Image* refImage, mitk::ContourModelSet* contourSet)
{
  if (nullptr == refImage)
    mitkThrow() << "Cannot convert to label mask. Passed reference image is nullptr.";
  if (nullptr == contourSet)
    mitkThrow() << "Cannot convert to label mask. Passed input is nullptr.";

  // Use mitk::ContourModelSetToImageFilter to fill the ContourModelSet into the image
  mitk::ContourModelSetToImageFilter::Pointer contourFiller = mitk::ContourModelSetToImageFilter::New();
  contourFiller->SetImage(refImage);
  contourFiller->SetInput(contourSet);
  contourFiller->MakeOutputLabelPixelTypeOn();

  try
  {
    contourFiller->Update();
  }
  catch (const std::exception& e)
  {
    MITK_ERROR << "Contour model conversion failed: " << e.what();
    return nullptr;
  }

  return contourFiller->GetOutput();
}

mitk::Image::Pointer mitk::ConvertContourModelToLabelMask(const mitk::Image* refImage, mitk::ContourModel* contourModel)
{
  if (nullptr == refImage)
    mitkThrow() << "Cannot convert to label mask. Passed reference image is nullptr.";
  if (nullptr == contourModel)
    mitkThrow() << "Cannot convert to label mask. Passed input is nullptr.";

  auto contourModelSet = mitk::ContourModelSet::New();
  contourModelSet->AddContourModel(contourModel);
  return ConvertContourModelSetToLabelMask(refImage, contourModelSet);
}
