/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkItkImageIO.h"

#include <mitkArbitraryTimeGeometry.h>
#include <mitkCoreServices.h>
#include <mitkCustomMimeType.h>
#include <mitkIOMimeTypes.h>
#include <mitkIPropertyPersistence.h>
#include <mitkImage.h>
#include <mitkImageReadAccessor.h>
#include <mitkLocaleSwitch.h>
#include <mitkUIDManipulator.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageIOFactory.h>
#include <itkImageIORegion.h>
#include <itkMetaDataObject.h>

#include <algorithm>

namespace mitk
{
  const char *const PROPERTY_NAME_TIMEGEOMETRY_TYPE = "org.mitk.timegeometry.type";
  const char *const PROPERTY_NAME_TIMEGEOMETRY_TIMEPOINTS = "org.mitk.timegeometry.timepoints";
  const char *const PROPERTY_KEY_TIMEGEOMETRY_TYPE = "org_mitk_timegeometry_type";
  const char *const PROPERTY_KEY_TIMEGEOMETRY_TIMEPOINTS = "org_mitk_timegeometry_timepoints";
  const char* const PROPERTY_KEY_UID = "org_mitk_uid";

  ItkImageIO::ItkImageIO(const ItkImageIO &other)
    : AbstractFileIO(other), m_ImageIO(dynamic_cast<itk::ImageIOBase *>(other.m_ImageIO->Clone().GetPointer()))
  {
    this->InitializeDefaultMetaDataKeys();
  }

  std::vector<std::string> ItkImageIO::FixUpImageIOExtensions(const std::string &imageIOName)
  {
    std::vector<std::string> extensions;
    // Try to fix-up some known ITK image IO classes
    if (imageIOName == "GiplImageIO")
    {
      extensions.push_back("gipl");
      extensions.push_back("gipl.gz");
    }
    else if (imageIOName == "GDCMImageIO")
    {
      extensions.push_back("gdcm");
      extensions.push_back("dcm");
      extensions.push_back("DCM");
      extensions.push_back("dc3");
      extensions.push_back("DC3");
      extensions.push_back("ima");
      extensions.push_back("img");
    }
    else if (imageIOName == "PNGImageIO")
    {
      extensions.push_back("png");
      extensions.push_back("PNG");
    }
    else if (imageIOName == "StimulateImageIO")
    {
      extensions.push_back("spr");
    }
    else if (imageIOName == "HDF5ImageIO")
    {
      extensions.push_back("hdf");
      extensions.push_back("h4");
      extensions.push_back("hdf4");
      extensions.push_back("h5");
      extensions.push_back("hdf5");
      extensions.push_back("he4");
      extensions.push_back("he5");
      extensions.push_back("hd5");
    }
    else if ("GE4ImageIO" == imageIOName || "GE5ImageIO" == imageIOName || "Bruker2dseqImageIO" == imageIOName)
    {
      extensions.push_back("");
    }

    if (!extensions.empty())
    {
      MITK_DEBUG << "Fixing up known extensions for " << imageIOName;
    }

    return extensions;
  }

  void ItkImageIO::FixUpCustomMimeTypeName(const std::string &imageIOName, CustomMimeType &customMimeType)
  {
    if ("GE4ImageIO" == imageIOName)
    {
      customMimeType.SetName(this->AbstractFileReader::GetMimeTypePrefix() + "ge4");
    }
    else if ("GE5ImageIO" == imageIOName)
    {
      customMimeType.SetName(this->AbstractFileReader::GetMimeTypePrefix() + "ge5");
    }
    else if ("Bruker2dseqImageIO" == imageIOName)
    {
      customMimeType.SetName(this->AbstractFileReader::GetMimeTypePrefix() + "bruker2dseq");
    }
  }

  ItkImageIO::ItkImageIO(itk::ImageIOBase::Pointer imageIO)
    : AbstractFileIO(Image::GetStaticNameOfClass()), m_ImageIO(imageIO)
  {
    if (m_ImageIO.IsNull())
    {
      mitkThrow() << "ITK ImageIOBase argument must not be nullptr";
    }

    this->AbstractFileReader::SetMimeTypePrefix(IOMimeTypes::DEFAULT_BASE_NAME() + ".image.");
    this->InitializeDefaultMetaDataKeys();

    std::vector<std::string> readExtensions = m_ImageIO->GetSupportedReadExtensions();

    if (readExtensions.empty())
    {
      std::string imageIOName = m_ImageIO->GetNameOfClass();
      MITK_DEBUG << "ITK ImageIOBase " << imageIOName << " does not provide read extensions";
      readExtensions = FixUpImageIOExtensions(imageIOName);
    }

    CustomMimeType customReaderMimeType;
    customReaderMimeType.SetCategory("Images");
    for (std::vector<std::string>::const_iterator iter = readExtensions.begin(), endIter = readExtensions.end();
         iter != endIter;
         ++iter)
    {
      std::string extension = *iter;
      if (!extension.empty() && extension[0] == '.')
      {
        extension.assign(iter->begin() + 1, iter->end());
      }
      customReaderMimeType.AddExtension(extension);
    }

    auto extensions = customReaderMimeType.GetExtensions();
    if (extensions.empty() || (extensions.size() == 1 && extensions[0].empty()))
    {
      std::string imageIOName = m_ImageIO->GetNameOfClass();
      FixUpCustomMimeTypeName(imageIOName, customReaderMimeType);
    }

    this->AbstractFileReader::SetMimeType(customReaderMimeType);

    std::vector<std::string> writeExtensions = imageIO->GetSupportedWriteExtensions();
    if (writeExtensions.empty())
    {
      std::string imageIOName = imageIO->GetNameOfClass();
      MITK_DEBUG << "ITK ImageIOBase " << imageIOName << " does not provide write extensions";
      writeExtensions = FixUpImageIOExtensions(imageIOName);
    }

    if (writeExtensions != readExtensions)
    {
      CustomMimeType customWriterMimeType;
      customWriterMimeType.SetCategory("Images");
      for (std::vector<std::string>::const_iterator iter = writeExtensions.begin(), endIter = writeExtensions.end();
           iter != endIter;
           ++iter)
      {
        std::string extension = *iter;
        if (!extension.empty() && extension[0] == '.')
        {
          extension.assign(iter->begin() + 1, iter->end());
        }
        customWriterMimeType.AddExtension(extension);
      }

      auto extensions = customWriterMimeType.GetExtensions();
      if (extensions.empty() || (extensions.size() == 1 && extensions[0].empty()))
      {
        std::string imageIOName = m_ImageIO->GetNameOfClass();
        FixUpCustomMimeTypeName(imageIOName, customWriterMimeType);
      }

      this->AbstractFileWriter::SetMimeType(customWriterMimeType);
    }

    std::string description = std::string("ITK ") + imageIO->GetNameOfClass();
    this->SetReaderDescription(description);
    this->SetWriterDescription(description);

    this->RegisterService();
  }

  ItkImageIO::ItkImageIO(const CustomMimeType &mimeType, itk::ImageIOBase::Pointer imageIO, int rank)
    : AbstractFileIO(Image::GetStaticNameOfClass(), mimeType, std::string("ITK ") + imageIO->GetNameOfClass()),
      m_ImageIO(imageIO)
  {
    if (m_ImageIO.IsNull())
    {
      mitkThrow() << "ITK ImageIOBase argument must not be nullptr";
    }

    this->AbstractFileReader::SetMimeTypePrefix(IOMimeTypes::DEFAULT_BASE_NAME() + ".image.");
    this->InitializeDefaultMetaDataKeys();

    if (rank)
    {
      this->AbstractFileReader::SetRanking(rank);
      this->AbstractFileWriter::SetRanking(rank);
    }

    this->RegisterService();
  }

  std::vector<TimePointType> ConvertMetaDataObjectToTimePointList(const itk::MetaDataObjectBase* data)
  {
    const auto* timeGeometryTimeData =
      dynamic_cast<const itk::MetaDataObject<std::string>*>(data);
    std::vector<TimePointType> result;

    if (timeGeometryTimeData)
    {
      std::string dataStr = timeGeometryTimeData->GetMetaDataObjectValue();
      std::stringstream stream(dataStr);
      TimePointType tp;
      while (stream >> tp)
      {
        result.push_back(tp);
      }
    }

    return result;
  };

  Image::Pointer ItkImageIO::LoadRawMitkImageFromImageIO(itk::ImageIOBase* imageIO, const std::string& path)
  {
    LocaleSwitch localeSwitch("C");

    Image::Pointer image = Image::New();

    const unsigned int MINDIM = 2;
    const unsigned int MAXDIM = 4;

    MITK_INFO << "loading " << path << " via itk::ImageIOFactory... " << std::endl;

    // Check to see if we can read the file given the name or prefix
    if (path.empty())
    {
      mitkThrow() << "Empty filename in mitk::ItkImageIO ";
    }

    // Got to allocate space for the image. Determine the characteristics of
    // the image.
    imageIO->SetFileName(path);
    imageIO->ReadImageInformation();

    unsigned int ndim = imageIO->GetNumberOfDimensions();
    if (ndim < MINDIM || ndim > MAXDIM)
    {
      MITK_WARN << "Sorry, only dimensions 2, 3 and 4 are supported. The given file has " << ndim
        << " dimensions! Reading as 4D.";
      ndim = MAXDIM;
    }

    itk::ImageIORegion ioRegion(ndim);
    itk::ImageIORegion::SizeType ioSize = ioRegion.GetSize();
    itk::ImageIORegion::IndexType ioStart = ioRegion.GetIndex();

    unsigned int dimensions[MAXDIM];
    dimensions[0] = 0;
    dimensions[1] = 0;
    dimensions[2] = 0;
    dimensions[3] = 0;

    ScalarType spacing[MAXDIM];
    spacing[0] = 1.0f;
    spacing[1] = 1.0f;
    spacing[2] = 1.0f;
    spacing[3] = 1.0f;

    Point3D origin;
    origin.Fill(0);

    unsigned int i;
    for (i = 0; i < ndim; ++i)
    {
      ioStart[i] = 0;
      ioSize[i] = imageIO->GetDimensions(i);
      if (i < MAXDIM)
      {
        dimensions[i] = imageIO->GetDimensions(i);
        spacing[i] = imageIO->GetSpacing(i);
        if (spacing[i] <= 0)
          spacing[i] = 1.0f;
      }
      if (i < 3)
      {
        origin[i] = imageIO->GetOrigin(i);
      }
    }

    ioRegion.SetSize(ioSize);
    ioRegion.SetIndex(ioStart);

    MITK_INFO << "ioRegion: " << ioRegion << std::endl;
    imageIO->SetIORegion(ioRegion);
    void* buffer = new unsigned char[imageIO->GetImageSizeInBytes()];
    imageIO->Read(buffer);

    image->Initialize(MakePixelType(imageIO), ndim, dimensions);
    image->SetImportChannel(buffer, 0, Image::ManageMemory);

    const itk::MetaDataDictionary& dictionary = imageIO->GetMetaDataDictionary();

    // access direction of itk::Image and include spacing
    mitk::Matrix3D matrix;
    matrix.SetIdentity();
    unsigned int j, itkDimMax3 = (ndim >= 3 ? 3 : ndim);
    for (i = 0; i < itkDimMax3; ++i)
      for (j = 0; j < itkDimMax3; ++j)
        matrix[i][j] = imageIO->GetDirection(j)[i];

    // re-initialize PlaneGeometry with origin and direction
    PlaneGeometry* planeGeometry = image->GetSlicedGeometry(0)->GetPlaneGeometry(0);
    planeGeometry->SetOrigin(origin);
    planeGeometry->GetIndexToWorldTransform()->SetMatrix(matrix);

    // re-initialize SlicedGeometry3D
    SlicedGeometry3D* slicedGeometry = image->GetSlicedGeometry(0);
    slicedGeometry->InitializeEvenlySpaced(planeGeometry, image->GetDimension(2));
    slicedGeometry->SetSpacing(spacing);

    MITK_INFO << slicedGeometry->GetCornerPoint(false, false, false);
    MITK_INFO << slicedGeometry->GetCornerPoint(true, true, true);

    // re-initialize TimeGeometry
    TimeGeometry::Pointer timeGeometry;

    if (dictionary.HasKey(PROPERTY_NAME_TIMEGEOMETRY_TYPE) || dictionary.HasKey(PROPERTY_KEY_TIMEGEOMETRY_TYPE))
    { // also check for the name because of backwards compatibility. Past code version stored with the name and not with
      // the key
      itk::MetaDataObject<std::string>::ConstPointer timeGeometryTypeData = nullptr;
      if (dictionary.HasKey(PROPERTY_NAME_TIMEGEOMETRY_TYPE))
      {
        timeGeometryTypeData =
          dynamic_cast<const itk::MetaDataObject<std::string> *>(dictionary.Get(PROPERTY_NAME_TIMEGEOMETRY_TYPE));
      }
      else
      {
        timeGeometryTypeData =
          dynamic_cast<const itk::MetaDataObject<std::string> *>(dictionary.Get(PROPERTY_KEY_TIMEGEOMETRY_TYPE));
      }

      if (timeGeometryTypeData->GetMetaDataObjectValue() == ArbitraryTimeGeometry::GetStaticNameOfClass())
      {
        MITK_INFO << "used time geometry: " << ArbitraryTimeGeometry::GetStaticNameOfClass();
        typedef std::vector<TimePointType> TimePointVector;
        TimePointVector timePoints;

        if (dictionary.HasKey(PROPERTY_NAME_TIMEGEOMETRY_TIMEPOINTS))
        {
          timePoints = ConvertMetaDataObjectToTimePointList(dictionary.Get(PROPERTY_NAME_TIMEGEOMETRY_TIMEPOINTS));
        }
        else if (dictionary.HasKey(PROPERTY_KEY_TIMEGEOMETRY_TIMEPOINTS))
        {
          timePoints = ConvertMetaDataObjectToTimePointList(dictionary.Get(PROPERTY_KEY_TIMEGEOMETRY_TIMEPOINTS));
        }

        if (timePoints.empty())
        {
          MITK_ERROR << "Stored timepoints are empty. Meta information seems to bee invalid. Switch to ProportionalTimeGeometry fallback";
        }
        else if (timePoints.size() - 1 != image->GetDimension(3))
        {
          MITK_ERROR << "Stored timepoints (" << timePoints.size() - 1 << ") and size of image time dimension ("
            << image->GetDimension(3) << ") do not match. Switch to ProportionalTimeGeometry fallback";
        }
        else
        {
          ArbitraryTimeGeometry::Pointer arbitraryTimeGeometry = ArbitraryTimeGeometry::New();
          TimePointVector::const_iterator pos = timePoints.begin();
          auto prePos = pos++;

          for (; pos != timePoints.end(); ++prePos, ++pos)
          {
            arbitraryTimeGeometry->AppendNewTimeStepClone(slicedGeometry, *prePos, *pos);
          }

          timeGeometry = arbitraryTimeGeometry;
        }
      }
    }

    if (timeGeometry.IsNull())
    { // Fallback. If no other valid time geometry has been created, create a ProportionalTimeGeometry
      MITK_INFO << "used time geometry: " << ProportionalTimeGeometry::GetStaticNameOfClass();
      ProportionalTimeGeometry::Pointer propTimeGeometry = ProportionalTimeGeometry::New();
      propTimeGeometry->Initialize(slicedGeometry, image->GetDimension(3));
      timeGeometry = propTimeGeometry;
    }

    image->SetTimeGeometry(timeGeometry);

    buffer = nullptr;
    MITK_INFO << "number of image components: " << image->GetPixelType().GetNumberOfComponents();
    return image;
  }

  itk::MetaDataObjectBase::Pointer ConvertTimePointListToMetaDataObject(const mitk::TimeGeometry* timeGeometry)
  {
    std::stringstream stream;
    stream << timeGeometry->GetTimeBounds(0)[0];
    const auto maxTimePoints = timeGeometry->CountTimeSteps();
    for (TimeStepType pos = 0; pos < maxTimePoints; ++pos)
    {
      auto timeBounds = timeGeometry->GetTimeBounds(pos);

      ///////////////////////////////////////
      // Workaround T27883. See https://phabricator.mitk.org/T27883#219473 for more details.
      // This workaround should be removed as soon as T28262 is solved!
      if (pos + 1 == maxTimePoints && timeBounds[0]==timeBounds[1])
      {
        timeBounds[1] = timeBounds[0] + 1.;
      }
      // End of workaround for T27883
      //////////////////////////////////////

      stream << " " << timeBounds[1];
    }
    auto result = itk::MetaDataObject<std::string>::New();
    result->SetMetaDataObjectValue(stream.str());
    return result.GetPointer();
  };

  PropertyList::Pointer ItkImageIO::ExtractMetaDataAsPropertyList(const itk::MetaDataDictionary& dictionary, const std::string& mimeTypeName, const std::vector<std::string>& defaultMetaDataKeys)
  {
    LocaleSwitch localeSwitch("C");
    PropertyList::Pointer result = PropertyList::New();

    for (auto iter = dictionary.Begin(), iterEnd = dictionary.End(); iter != iterEnd;
      ++iter)
    {
      if (iter->second->GetMetaDataObjectTypeInfo() == typeid(std::string))
      {
        const std::string& key = iter->first;
        std::string assumedPropertyName = key;
        std::replace(assumedPropertyName.begin(), assumedPropertyName.end(), '_', '.');

        // Check if there is already a info for the key and our mime type.
        CoreServicePointer<IPropertyPersistence> propPersistenceService(CoreServices::GetPropertyPersistence());
        IPropertyPersistence::InfoResultType infoList = propPersistenceService->GetInfoByKey(key);

        auto predicate = [&mimeTypeName](const PropertyPersistenceInfo::ConstPointer& x) {
          return x.IsNotNull() && x->GetMimeTypeName() == mimeTypeName;
        };
        auto finding = std::find_if(infoList.begin(), infoList.end(), predicate);

        if (finding == infoList.end())
        {
          auto predicateWild = [](const PropertyPersistenceInfo::ConstPointer& x) {
            return x.IsNotNull() && x->GetMimeTypeName() == PropertyPersistenceInfo::ANY_MIMETYPE_NAME();
          };
          finding = std::find_if(infoList.begin(), infoList.end(), predicateWild);
        }

        PropertyPersistenceInfo::ConstPointer info;

        if (finding != infoList.end())
        {
          assumedPropertyName = (*finding)->GetName();
          info = *finding;
        }
        else
        { // we have not found anything suitable so we generate our own info
          auto newInfo = PropertyPersistenceInfo::New();
          newInfo->SetNameAndKey(assumedPropertyName, key);
          newInfo->SetMimeTypeName(PropertyPersistenceInfo::ANY_MIMETYPE_NAME());
          info = newInfo;
        }

        std::string value =
          dynamic_cast<itk::MetaDataObject<std::string> *>(iter->second.GetPointer())->GetMetaDataObjectValue();

        mitk::BaseProperty::Pointer loadedProp = info->GetDeserializationFunction()(value);

        if (loadedProp.IsNull())
        {
          MITK_ERROR << "Property cannot be correctly deserialized and is skipped. Check if data format is valid. Problematic property value string: \"" << value << "\"; Property info used to deserialized: " << info;
          break;
        }

        result->SetProperty(assumedPropertyName.c_str(), loadedProp);

        // Read properties should be persisted unless they are default properties
        // which are written anyway
        bool isDefaultKey(false);

        for (const auto& defaultKey : defaultMetaDataKeys)
        {
          if (defaultKey.length() <= assumedPropertyName.length())
          {
            // does the start match the default key
            if (assumedPropertyName.substr(0, defaultKey.length()).find(defaultKey) != std::string::npos)
            {
              isDefaultKey = true;
              break;
            }
          }
        }

        if (!isDefaultKey)
        {
          propPersistenceService->AddInfo(info);
        }
      }
    }
    return result;
  }

  std::vector<BaseData::Pointer> ItkImageIO::DoRead()
  {
    std::vector<BaseData::Pointer> result;

    auto image = LoadRawMitkImageFromImageIO(this->m_ImageIO, this->GetLocalFileName());

    const itk::MetaDataDictionary& dictionary = this->m_ImageIO->GetMetaDataDictionary();

    //meta data handling
    auto props = ExtractMetaDataAsPropertyList(this->m_ImageIO->GetMetaDataDictionary(), this->GetMimeType()->GetName(), this->m_DefaultMetaDataKeys);
    for (auto& [name, prop] : *(props->GetMap()))
    {
      image->SetProperty(name, prop);
    }

    // Handle UID
    if (dictionary.HasKey(PROPERTY_KEY_UID))
    {
      itk::MetaDataObject<std::string>::ConstPointer uidData = dynamic_cast<const itk::MetaDataObject<std::string>*>(dictionary.Get(PROPERTY_KEY_UID));
      if (uidData.IsNotNull())
      {
        mitk::UIDManipulator uidManipulator(image);
        uidManipulator.SetUID(uidData->GetMetaDataObjectValue());
      }
    }

    MITK_INFO << "...finished!";

    result.push_back(image.GetPointer());
    return result;
  }

  AbstractFileIO::ConfidenceLevel ItkImageIO::GetReaderConfidenceLevel() const
  {
    return m_ImageIO->CanReadFile(GetLocalFileName().c_str()) ? IFileReader::Supported : IFileReader::Unsupported;
  }

  void ItkImageIO::PreparImageIOToWriteImage(itk::ImageIOBase* imageIO, const Image* image)
  {
    // Switch the current locale to "C"
    LocaleSwitch localeSwitch("C");

    // Clone the image geometry, because we might have to change it
    // for writing purposes
    BaseGeometry::Pointer geometry = image->GetGeometry()->Clone();

    // Check if geometry information will be lost
    if (image->GetDimension() == 2 && !geometry->Is2DConvertable())
    {
      MITK_WARN << "Saving a 2D image with 3D geometry information. Geometry information will be lost! You might "
        "consider using Convert2Dto3DImageFilter before saving.";

      // set matrix to identity
      mitk::AffineTransform3D::Pointer affTrans = mitk::AffineTransform3D::New();
      affTrans->SetIdentity();
      mitk::Vector3D spacing = geometry->GetSpacing();
      mitk::Point3D origin = geometry->GetOrigin();
      geometry->SetIndexToWorldTransform(affTrans);
      geometry->SetSpacing(spacing);
      geometry->SetOrigin(origin);
    }

    // Implementation of writer using itkImageIO directly. This skips the use
    // of templated itkImageFileWriter, which saves the multiplexing on MITK side.

    const unsigned int dimension = image->GetDimension();
    const unsigned int* const dimensions = image->GetDimensions();
    const mitk::PixelType pixelType = image->GetPixelType();
    const mitk::Vector3D mitkSpacing = geometry->GetSpacing();
    const mitk::Point3D mitkOrigin = geometry->GetOrigin();

    // Due to templating in itk, we are forced to save a 4D spacing and 4D Origin,
    // though they are not supported in MITK
    itk::Vector<double, 4u> spacing4D;
    spacing4D[0] = mitkSpacing[0];
    spacing4D[1] = mitkSpacing[1];
    spacing4D[2] = mitkSpacing[2];
    spacing4D[3] = 1; // There is no support for a 4D spacing. However, we should have a valid value here

    itk::Vector<double, 4u> origin4D;
    origin4D[0] = mitkOrigin[0];
    origin4D[1] = mitkOrigin[1];
    origin4D[2] = mitkOrigin[2];
    origin4D[3] = 0; // There is no support for a 4D origin. However, we should have a valid value here

    // Set the necessary information for imageIO
    imageIO->SetNumberOfDimensions(dimension);
    imageIO->SetPixelType(pixelType.GetPixelType());
    imageIO->SetComponentType(static_cast<int>(pixelType.GetComponentType()) < PixelComponentUserType
      ? pixelType.GetComponentType()
      : itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE);
    imageIO->SetNumberOfComponents(pixelType.GetNumberOfComponents());

    itk::ImageIORegion ioRegion(dimension);

    for (unsigned int i = 0; i < dimension; i++)
    {
      imageIO->SetDimensions(i, dimensions[i]);
      imageIO->SetSpacing(i, spacing4D[i]);
      imageIO->SetOrigin(i, origin4D[i]);

      mitk::Vector3D mitkDirection(0.0);
      mitkDirection.SetVnlVector(geometry->GetIndexToWorldTransform()->GetMatrix().GetVnlMatrix().get_column(i).as_ref());
      itk::Vector<double, 4u> direction4D;
      direction4D[0] = mitkDirection[0];
      direction4D[1] = mitkDirection[1];
      direction4D[2] = mitkDirection[2];

      // MITK only supports a 3x3 direction matrix. Due to templating in itk, however, we must
      // save a 4x4 matrix for 4D images. in this case, add an homogneous component to the matrix.
      if (i == 3)
      {
        direction4D[3] = 1; // homogenous component
      }
      else
      {
        direction4D[3] = 0;
      }
      vnl_vector<double> axisDirection(dimension);
      for (unsigned int j = 0; j < dimension; j++)
      {
        axisDirection[j] = direction4D[j] / spacing4D[i];
      }
      imageIO->SetDirection(i, axisDirection);

      ioRegion.SetSize(i, image->GetLargestPossibleRegion().GetSize(i));
      ioRegion.SetIndex(i, image->GetLargestPossibleRegion().GetIndex(i));
    }

    imageIO->SetIORegion(ioRegion);

    // Handle time geometry
    const auto* arbitraryTG = dynamic_cast<const ArbitraryTimeGeometry*>(image->GetTimeGeometry());
    if (arbitraryTG)
    {
      itk::EncapsulateMetaData<std::string>(imageIO->GetMetaDataDictionary(),
        PROPERTY_KEY_TIMEGEOMETRY_TYPE,
        ArbitraryTimeGeometry::GetStaticNameOfClass());

      auto metaTimePoints = ConvertTimePointListToMetaDataObject(arbitraryTG);
      imageIO->GetMetaDataDictionary().Set(PROPERTY_KEY_TIMEGEOMETRY_TIMEPOINTS, metaTimePoints);
    }
  }

  void ItkImageIO::SavePropertyListAsMetaData(itk::MetaDataDictionary& dictionary, const PropertyList* properties, const std::string& mimeTypeName)
  {
    // Switch the current locale to "C"
    LocaleSwitch localeSwitch("C");

    for (const auto& property : *properties->GetMap())
    {
      mitk::CoreServicePointer<IPropertyPersistence> propPersistenceService(mitk::CoreServices::GetPropertyPersistence());
      IPropertyPersistence::InfoResultType infoList = propPersistenceService->GetInfo(property.first, mimeTypeName, true);

      if (infoList.empty())
      {
        continue;
      }

      std::string value = mitk::BaseProperty::VALUE_CANNOT_BE_CONVERTED_TO_STRING;
      try
      {
        value = infoList.front()->GetSerializationFunction()(property.second);
      }
      catch (const std::exception& e)
      {
        MITK_ERROR << "Error when serializing content of property. This often indicates the use of an out dated reader. Property will not be stored. Skipped property: " << property.first << ". Reason: " << e.what();
      }
      catch (...)
      {
        MITK_ERROR << "Unknown error when serializing content of property. This often indicates the use of an out dated reader. Property will not be stored. Skipped property: " << property.first;
      }

      if (value == mitk::BaseProperty::VALUE_CANNOT_BE_CONVERTED_TO_STRING)
      {
        continue;
      }

      std::string key = infoList.front()->GetKey();

      itk::EncapsulateMetaData<std::string>(dictionary, key, value);
    }
  }

  void ItkImageIO::Write()
  {
    const auto *image = dynamic_cast<const mitk::Image *>(this->GetInput());

    if (image == nullptr)
    {
      mitkThrow() << "Cannot write non-image data";
    }

    PreparImageIOToWriteImage(m_ImageIO, image);

    LocalFile localFile(this);
    const std::string path = localFile.GetFileName();

    MITK_INFO << "Writing image: " << path << std::endl;

    try
    {
      // Handle properties
      SavePropertyListAsMetaData(m_ImageIO->GetMetaDataDictionary(), image->GetPropertyList(), this->GetMimeType()->GetName());
      // Handle UID
      itk::EncapsulateMetaData<std::string>(m_ImageIO->GetMetaDataDictionary(), PROPERTY_KEY_UID, image->GetUID());

      // use compression if available
      m_ImageIO->UseCompressionOn();
      m_ImageIO->SetFileName(path);

      ImageReadAccessor imageAccess(image);
      LocaleSwitch localeSwitch2("C");
      m_ImageIO->Write(imageAccess.GetData());
    }
    catch (const std::exception &e)
    {
      mitkThrow() << e.what();
    }
  }

  AbstractFileIO::ConfidenceLevel ItkImageIO::GetWriterConfidenceLevel() const
  {
    // Check if the image dimension is supported
    const auto *image = dynamic_cast<const Image *>(this->GetInput());
    if (image == nullptr)
    {
      // We cannot write a null object, DUH!
      return IFileWriter::Unsupported;
    }

    if (!m_ImageIO->SupportsDimension(image->GetDimension()))
    {
      // okay, dimension is not supported. We have to look at a special case:
      // 3D-Image with one slice. We can treat that as a 2D image.
      if ((image->GetDimension() == 3) && (image->GetSlicedGeometry()->GetSlices() == 1))
        return IFileWriter::Supported;
      else
        return IFileWriter::Unsupported;
    }

    // Check if geometry information will be lost
    if (image->GetDimension() == 2 && !image->GetGeometry()->Is2DConvertable())
    {
      return IFileWriter::PartiallySupported;
    }
    return IFileWriter::Supported;
  }

  ItkImageIO *ItkImageIO::IOClone() const { return new ItkImageIO(*this); }
  void ItkImageIO::InitializeDefaultMetaDataKeys()
  {
    this->m_DefaultMetaDataKeys.push_back("NRRD.space");
    this->m_DefaultMetaDataKeys.push_back("NRRD.kinds");
    this->m_DefaultMetaDataKeys.push_back(PROPERTY_NAME_TIMEGEOMETRY_TYPE);
    this->m_DefaultMetaDataKeys.push_back(PROPERTY_NAME_TIMEGEOMETRY_TIMEPOINTS);
    this->m_DefaultMetaDataKeys.push_back("ITK.InputFilterName");
  }
}
