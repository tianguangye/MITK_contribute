/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkImageVtkLegacyIO.h"

#include "mitkIOMimeTypes.h"
#include "mitkImage.h"
#include "mitkImageVtkReadAccessor.h"

#include <vtkErrorCode.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPointsWriter.h>

namespace mitk
{
  ImageVtkLegacyIO::ImageVtkLegacyIO()
    : AbstractFileIO(Image::GetStaticNameOfClass(), IOMimeTypes::VTK_IMAGE_LEGACY_MIMETYPE(), "VTK Legacy Image")
  {
    Options defaultOptions;
    defaultOptions["Save as binary file"] = false;
    this->SetDefaultWriterOptions(defaultOptions);
    this->RegisterService();
  }

  std::vector<BaseData::Pointer> ImageVtkLegacyIO::DoRead()
  {
    // The legay vtk reader cannot work with input streams
    const std::string fileName = this->GetLocalFileName();
    vtkSmartPointer<vtkStructuredPointsReader> reader = vtkSmartPointer<vtkStructuredPointsReader>::New();
    reader->SetFileName(fileName.c_str());
    reader->Update();

    if (reader->GetOutput() != nullptr)
    {
      mitk::Image::Pointer output = mitk::Image::New();
      output->Initialize(reader->GetOutput());
      output->SetVolume(reader->GetOutput()->GetScalarPointer());
      std::vector<BaseData::Pointer> result;
      result.push_back(output.GetPointer());
      return result;
    }
    else
    {
      mitkThrow() << "vtkStructuredPointsReader error: "
                  << vtkErrorCode::GetStringFromErrorCode(reader->GetErrorCode());
    }
  }

  IFileIO::ConfidenceLevel ImageVtkLegacyIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    vtkSmartPointer<vtkStructuredPointsReader> reader = vtkSmartPointer<vtkStructuredPointsReader>::New();
    reader->SetFileName(this->GetLocalFileName().c_str());
    if (reader->IsFileStructuredPoints())
    {
      return Supported;
    }
    return Unsupported;
  }

  void ImageVtkLegacyIO::Write()
  {
    ValidateOutputLocation();

    const auto *input = dynamic_cast<const Image *>(this->GetInput());

    vtkSmartPointer<vtkStructuredPointsWriter> writer = vtkSmartPointer<vtkStructuredPointsWriter>::New();

    // The legacy vtk image writer cannot write to streams
    LocalFile localFile(this);
    writer->SetFileName(localFile.GetFileName().c_str());

    if (us::any_cast<bool>(GetWriterOption("Save as binary file")))
    {
      writer->SetFileTypeToBinary();
    }

    ImageVtkReadAccessor vtkReadAccessor(Image::ConstPointer(input), nullptr, input->GetVtkImageData());
    writer->SetInputData(const_cast<vtkImageData *>(vtkReadAccessor.GetVtkImageData()));

    if (writer->Write() == 0 || writer->GetErrorCode() != 0)
    {
      mitkThrow() << "vtkStructuredPointesWriter error: "
                  << vtkErrorCode::GetStringFromErrorCode(writer->GetErrorCode());
    }
  }

  IFileIO::ConfidenceLevel ImageVtkLegacyIO::GetWriterConfidenceLevel() const
  {
    if (AbstractFileIO::GetWriterConfidenceLevel() == Unsupported)
      return Unsupported;

    const auto *input = dynamic_cast<const Image *>(this->GetInput());
    if (input->GetDimension() == 3)
      return Supported;
    else if (input->GetDimension() < 3)
      return PartiallySupported;
    return Unsupported;
  }

  ImageVtkLegacyIO *ImageVtkLegacyIO::IOClone() const { return new ImageVtkLegacyIO(*this); }
}
