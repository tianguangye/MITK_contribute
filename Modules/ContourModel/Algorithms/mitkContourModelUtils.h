/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkContourModelUtils_h
#define mitkContourModelUtils_h

#include <mitkContourModel.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>

#include <vtkSmartPointer.h>

#include <MitkContourModelExports.h>

namespace mitk
{
  /**
   * \brief Helpful methods for working with contours and images
   *
   *
   */
  class MITKCONTOURMODEL_EXPORT ContourModelUtils : public itk::Object
  {
  public:
    mitkClassMacroItkParent(ContourModelUtils, itk::Object);

    /**
      \brief Projects a contour onto an image point by point. Converts from world to index coordinates.

      \param slice
      \param contourIn3D
    */
    static ContourModel::Pointer ProjectContourTo2DSlice(const Image *slice,
                                                         const ContourModel *contourIn3D);

    /**
      \brief Projects a slice index coordinates of a contour back into world coordinates.

      \param sliceGeometry
      \param contourIn2D
    */
    static ContourModel::Pointer BackProjectContourFrom2DSlice(const BaseGeometry *sliceGeometry,
                                                               const ContourModel *contourIn2D);

    /**
    \brief Fill a contour in a 2D slice with a specified pixel value.
    This overloaded version uses the contour at the passed contourTimeStep
    to fill the passed image slice.
    \deprecated This function is deprecated. Use FillContourInSlice2() (in
    conjunction e.g. with TransferLabelContentAtTimeStep()) instead.
    \pre sliceImage points to a valid instance
    \pre projectedContour points to a valid instance
    */
    //[[deprecated]]
    DEPRECATED(static void FillContourInSlice(const ContourModel* projectedContour,
      TimeStepType contourTimeStep,
      Image* sliceImage,
      int paintingPixelValue = 1));

    /**
    \brief Fill a contour in a 2D slice with a specified pixel value.
    This version always uses the contour of time step 0 and fills the image.
    \param projectedContour Pointer to the contour that should be projected.
    \param sliceImage Pointer to the image which content should be altered by
    adding the contour with the specified paintingPixelValue.
    \param paintingPixelValue
    \pre sliceImage points to a valid instance
    \pre projectedContour points to a valid instance
    */
    static void FillContourInSlice2(const ContourModel* projectedContour,
      Image* sliceImage,
      int paintingPixelValue = 1);

    /**
    \brief Fill a contour in a 2D slice with a specified pixel value.
    This overloaded version uses the contour at the passed contourTimeStep
    to fill the passed image slice.
    \param projectedContour Pointer to the contour that should be projected.
    \param contourTimeStep
    \param sliceImage Pointer to the image which content should be altered by
    \param paintingPixelValue
    adding the contour with the specified paintingPixelValue.
    \pre sliceImage points to a valid instance
    \pre projectedContour points to a valid instance
    */
    static void FillContourInSlice2(const ContourModel* projectedContour,
      TimeStepType contourTimeStep,
      Image* sliceImage,
      int paintingPixelValue = 1);

    /**
    \brief Fills the paintingPixelValue into every pixel of resultImage as indicated by filledImage.
    If a LableSet image is specified it also by incorporating the rules of LabelSet images when filling the content.
    \param filledImage Pointer to the image content that should be checked to decide if a pixel in resultImage should
    be filled with paintingPixelValue or not.
    \param resultImage Pointer to the image content that should be overwritten guided by the content of filledImage.
    If an LabelSet instance is passed its states (e.g. locked labels etc...) will be used. If nullptr or an normal image
    is passed, then simply any pixel position indicated by filledImage will be overwritten.
    \param paintingPixelValue the pixelvalue/label that should be used in the result image when filling.
    \param fillForegroundThreshold The threshold value that decides if a pixel in the filled image counts
    as foreground (>=fillForegroundThreshold) or not.
    \deprecated This function is deprecated. Use TransferLabelContent() instead.
    */
    [[deprecated]]
    static void FillSliceInSlice(vtkSmartPointer<vtkImageData> filledImage,
                                 vtkSmartPointer<vtkImageData> resultImage,
                                 int paintingPixelValue,
                                 double fillForegroundThreshold = 1.0);

    /**
    \brief Move the contour in time step 0 to to a new contour model at the given time step.
    */
    static ContourModel::Pointer MoveZerothContourTimeStep(const ContourModel *contour, TimeStepType timeStep);

  protected:
    ContourModelUtils();
    ~ContourModelUtils() override;
  };
}

#endif
