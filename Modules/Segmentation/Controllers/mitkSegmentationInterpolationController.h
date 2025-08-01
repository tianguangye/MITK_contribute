/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkSegmentationInterpolationController_h
#define mitkSegmentationInterpolationController_h

#include "mitkCommon.h"
#include "mitkImage.h"
#include <MitkSegmentationExports.h>
#include <mitkShapeBasedInterpolationAlgorithm.h>

#include <itkImage.h>
#include <itkObjectFactory.h>

#include <map>
#include <mutex>
#include <utility>
#include <vector>

namespace mitk
{
  class Image;

  /**
    \brief Generates interpolations of 2D slices.

    \sa QmitkSlicesInterpolator
    \sa QmitkInteractiveSegmentation

    \ingroup ToolManagerEtAl

    This class keeps track of the contents of a 3D segmentation image.
    \attention mitk::SegmentationInterpolationController assumes that the image contains pixel values of 0 and 1.

    After you set the segmentation image using SetSegmentationVolume(), the whole image is scanned for pixels other than
    0.
    SegmentationInterpolationController registers as an observer to the segmentation image, and repeats the scan
    whenvever the
    image is modified.

    You can prevent this (time consuming) scan if you do the changes slice-wise and send difference images to
    SegmentationInterpolationController.
    For this purpose SetChangedSlice() should be used. mitk::OverwriteImageFilter already does this every time it
    changes a
    slice of an image. There is a static method InterpolatorForImage(), which can be used to find out if there already
    is an interpolator
    instance for a specified image. OverwriteImageFilter uses this to get to know its interpolator.

    SegmentationInterpolationController needs to maintain some information about the image slices (in every dimension).
    This information is stored internally in m_SegmentationCountInSlice, which is basically three std::vectors (one for
    each dimension).
    Each item describes one image dimension, each vector item holds the count of pixels in "its" slice.

    $Author$
  */
  class MITKSEGMENTATION_EXPORT SegmentationInterpolationController : public itk::Object
  {
  public:
    mitkClassMacroItkParent(SegmentationInterpolationController, itk::Object);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

      /**
        \brief Find interpolator for a given image.
        \return nullptr if there is no interpolator yet.

        This method is useful if several "clients" modify the same image and want to access the interpolations.
        Then they can share the same object.
       */
      static SegmentationInterpolationController *InterpolatorForImage(const Image *);

    /**
      \brief Block reaction to an images Modified() events.

      Blocking the scan of the whole image is especially useful when you are about to change a single slice
      of the image. Then you would send a difference image of this single slice to SegmentationInterpolationController
      but call image->Modified() anyway. Before calling image->Modified() you should block
      SegmentationInterpolationController's reactions to this modified by using this method.
    */
    void BlockModified(bool);

    /**
      \brief Initialize with a whole volume.

      Will scan the volume for segmentation pixels (values other than 0) and fill some internal data structures.
      You don't have to call this method every time something changes, but only
      when several slices at once change.

      When you change a single slice, call SetChangedSlice() instead.
    */
    void SetSegmentationVolume(const Image *segmentation);

    /**
      \brief Update after changing a single slice.

      \param sliceDiff is a 2D image with the difference image of the slice determined by sliceDimension and sliceIndex.
             The difference is (pixel value in the new slice minus pixel value in the old slice).

      \param sliceDimension Number of the dimension which is constant for all pixels of the meant slice.

      \param sliceIndex Which slice to take, in the direction specified by sliceDimension. Count starts from 0.

      \param timeStep Which time step is changed
    */
    void SetChangedSlice(const Image *sliceDiff,
                         unsigned int sliceDimension,
                         unsigned int sliceIndex,
                         unsigned int timeStep);
    void SetChangedVolume(const Image *sliceDiff, unsigned int timeStep);

    /**
      \brief Generates an interpolated image for the given slice.

      \param sliceDimension Number of the dimension which is constant for all pixels of the meant slice.

      \param sliceIndex Which slice to take, in the direction specified by sliceDimension. Count starts from 0.

      \param currentPlane

      \param timeStep Which time step to use

      \param algorithm Optional algorithm instance to potentially benefit from caching for repeated interpolation
    */
    Image::Pointer Interpolate(unsigned int sliceDimension,
                               unsigned int sliceIndex,
                               const mitk::PlaneGeometry *currentPlane,
                               unsigned int timeStep,
                               mitk::ShapeBasedInterpolationAlgorithm::Pointer algorithm = nullptr);

    void OnImageModified(const itk::EventObject &);

    /**
     * Activate/Deactivate the 2D interpolation.
    */
    void Activate2DInterpolation(bool);

    /**
     * Enable slice extraction cache for upper and lower slices.
    */
    void EnableSliceImageCache();

    /**
     * Disable slice extraction cache for upper and lower slices.
    */
    void DisableSliceImageCache();

    /**
      \brief Get existing instance or create a new one
    */
    static SegmentationInterpolationController *GetInstance();

  protected:
    /**
      \brief Protected class of mitk::SegmentationInterpolationController. Don't use (you shouldn't be able to do so)!
    */
    class MITKSEGMENTATION_EXPORT SetChangedSliceOptions
    {
    public:
      SetChangedSliceOptions(
        unsigned int sd, unsigned int si, unsigned int d0, unsigned int d1, unsigned int t, const void *pixels)
        : sliceDimension(sd), sliceIndex(si), dim0(d0), dim1(d1), timeStep(t), pixelData(pixels)
      {
      }

      unsigned int sliceDimension;
      unsigned int sliceIndex;
      unsigned int dim0;
      unsigned int dim1;
      unsigned int timeStep;
      const void *pixelData;
    };

    typedef std::vector<unsigned int> DirtyVectorType;
    // typedef std::vector< DirtyVectorType[3] > TimeResolvedDirtyVectorType; // cannot work with C++, so next line is
    // used for implementation
    typedef std::vector<std::vector<DirtyVectorType>> TimeResolvedDirtyVectorType;
    typedef std::map<const Image *, SegmentationInterpolationController *> InterpolatorMapType;

    SegmentationInterpolationController(); // purposely hidden
    ~SegmentationInterpolationController() override;

    /// internal scan of a single slice
    template <typename DATATYPE>
    void ScanChangedSlice(const itk::Image<DATATYPE, 2> *, const SetChangedSliceOptions &options);

    template <typename TPixel, unsigned int VImageDimension>
    void ScanChangedVolume(const itk::Image<TPixel, VImageDimension> *, unsigned int timeStep);

    template <typename DATATYPE>
    void ScanWholeVolume(const itk::Image<DATATYPE, 3> *, const Image *volume, unsigned int timeStep);

    void PrintStatus();

    /**
     * Extract a slice and optionally use a caching mechanism if enabled.
    */
    mitk::Image::Pointer ExtractSlice(const PlaneGeometry* planeGeometry, unsigned int sliceIndex, unsigned int timeStep, bool cache = false);

    /**
      An array of flags. One for each dimension of the image. A flag is set, when a slice in a certain dimension
      has at least one pixel that is not 0 (which would mean that it has to be considered by the interpolation
      algorithm).

      E.g. flags for axial slices are stored in m_SegmentationCountInSlice[0][index].

      Enhanced with time steps it is now m_SegmentationCountInSlice[timeStep][0][index]
    */
    TimeResolvedDirtyVectorType m_SegmentationCountInSlice;

    static InterpolatorMapType s_InterpolatorForImage;

    Image::ConstPointer m_Segmentation;
    std::pair<unsigned long, bool> m_SegmentationModifiedObserverTag; // first: actual tag, second: tag assigned / valid?
    bool m_BlockModified;
    bool m_2DInterpolationActivated;

    bool m_EnableSliceImageCache;
    std::map<std::pair<unsigned int, unsigned int>, Image::Pointer> m_SliceImageCache;
    std::mutex m_SliceImageCacheMutex;
  };

} // namespace

#endif
