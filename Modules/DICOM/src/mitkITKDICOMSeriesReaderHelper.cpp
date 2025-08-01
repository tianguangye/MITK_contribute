/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

//#define MBILOG_ENABLE_DEBUG

#include <dcmtk/dcmdata/dcvrdt.h>
#include <dcmtk/ofstd/ofstd.h>

#define BOOST_DATE_TIME_NO_LIB
//Prevent unnecessary/unwanted auto link in this compilation when activating boost libraries in the MITK superbuild
//It is necessary because BOOST_ALL_DYN_LINK overwrites BOOST_DATE_TIME_NO_LIB
#if defined(BOOST_ALL_DYN_LINK)
  #undef BOOST_ALL_DYN_LINK
#endif

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "mitkITKDICOMSeriesReaderHelper.h"
#include "mitkITKDICOMSeriesReaderHelper.txx"

#include "mitkDICOMGDCMTagScanner.h"
#include "mitkArbitraryTimeGeometry.h"

#include "dcmtk/dcmdata/dcvrda.h"


const mitk::DICOMTag mitk::ITKDICOMSeriesReaderHelper::AcquisitionDateTag = mitk::DICOMTag( 0x0008, 0x0022 );
const mitk::DICOMTag mitk::ITKDICOMSeriesReaderHelper::AcquisitionTimeTag = mitk::DICOMTag( 0x0008, 0x0032 );
const mitk::DICOMTag mitk::ITKDICOMSeriesReaderHelper::TriggerTimeTag = mitk::DICOMTag( 0x0018, 0x1060 );

#define switchTypeCase(Dim, IOType, T) \
  case IOType:                    \
    return LoadDICOMByITK<T, Dim>( filenames, correctTilt, tiltInfo, io );

bool mitk::ITKDICOMSeriesReaderHelper::CanHandleFile( const std::string& filename )
{
  MITK_DEBUG << "ITKDICOMSeriesReaderHelper::CanHandleFile " << filename;
  itk::GDCMImageIO::Pointer tester = itk::GDCMImageIO::New();
  return tester->CanReadFile( filename.c_str() );
}

template<unsigned int TDim>
mitk::Image::Pointer
mitk::ITKDICOMSeriesReaderHelper::LoadByTypeDispatch(const StringContainer& filenames,
  bool correctTilt,
  const GantryTiltInformation& tiltInfo,
  itk::GDCMImageIO::Pointer& io)
{
  if (io->GetPixelType() == itk::IOPixelEnum::SCALAR)
  {
    switch (io->GetComponentType())
    {
      switchTypeCase( TDim, itk::IOComponentEnum::UCHAR, unsigned char)
      switchTypeCase( TDim, itk::IOComponentEnum::CHAR, char)
      switchTypeCase( TDim, itk::IOComponentEnum::USHORT, unsigned short)
      switchTypeCase( TDim, itk::IOComponentEnum::SHORT, short)
      switchTypeCase( TDim, itk::IOComponentEnum::UINT, unsigned int)
      switchTypeCase( TDim, itk::IOComponentEnum::INT, int)
      switchTypeCase( TDim, itk::IOComponentEnum::ULONG, long unsigned int)
      switchTypeCase( TDim, itk::IOComponentEnum::LONG, long int)
      switchTypeCase( TDim, itk::IOComponentEnum::FLOAT, float)
      switchTypeCase( TDim, itk::IOComponentEnum::DOUBLE, double)
      default: MITK_ERROR
        << "Found unsupported DICOM scalar pixel type: (enum value) "
        << io->GetComponentType();
    }
  }
  else if (io->GetPixelType() == itk::IOPixelEnum::RGB)
  {
    switch (io->GetComponentType())
    {
      switchTypeCase( TDim, itk::IOComponentEnum::UCHAR, itk::RGBPixel<unsigned char>)
      switchTypeCase( TDim, 
       itk::IOComponentEnum::CHAR, itk::RGBPixel<char>) switchTypeCase( TDim, itk::IOComponentEnum::USHORT,
          itk::RGBPixel<unsigned short>)
        switchTypeCase( TDim, itk::IOComponentEnum::SHORT, itk::RGBPixel<short>) switchTypeCase( TDim, 
          itk::IOComponentEnum::UINT, itk::RGBPixel<unsigned int>) switchTypeCase( TDim, itk::IOComponentEnum::INT, itk::RGBPixel<int>)
        switchTypeCase( TDim, itk::IOComponentEnum::ULONG, itk::RGBPixel<long unsigned int>)
        switchTypeCase( TDim, itk::IOComponentEnum::LONG, itk::RGBPixel<long int>) switchTypeCase( TDim, 
          itk::IOComponentEnum::FLOAT, itk::RGBPixel<float>) switchTypeCase( TDim, itk::IOComponentEnum::DOUBLE,
            itk::RGBPixel<double>) default
    : MITK_ERROR
        << "Found unsupported DICOM scalar pixel type: (enum value) "
        << io->GetComponentType();
    }
  }
  MITK_ERROR << "Unsupported DICOM pixel type";
  return nullptr;
}

mitk::Image::Pointer mitk::ITKDICOMSeriesReaderHelper::Load( const StringContainer& filenames,
                                                             bool correctTilt,
                                                             const GantryTiltInformation& tiltInfo )
{
  if ( filenames.empty() )
  {
    MITK_DEBUG
      << "Calling LoadDicomSeries with empty filename string container. Probably invalid application logic.";
    return nullptr; // this is not actually an error but the result is very simple
  }

  typedef itk::GDCMImageIO DcmIoType;
  DcmIoType::Pointer io = DcmIoType::New();

  try
  {
    if ( io->CanReadFile( filenames.front().c_str() ) )
    {
      io->SetFileName( filenames.front().c_str() );
      io->ReadImageInformation();

      if(io->GetNumberOfDimensions()==2 || io->GetSpacing(2)==0.)
      {
        if (filenames.size() > 1)
        {
          MITK_ERROR << "Invalid application logic was called to load multiple DICOM files into one image volume, but at least one DICOM file indicated that it is 2D.";
          return nullptr;
        }

        return LoadByTypeDispatch<2>(filenames, correctTilt, tiltInfo, io);
      }
      else
      {
        return LoadByTypeDispatch<3>(filenames, correctTilt, tiltInfo, io);
      }
    }
  }
  catch ( const itk::MemoryAllocationError& e )
  {
    MITK_ERROR << "Out of memory. Cannot load DICOM series: " << e.what();
  }
  catch ( const std::exception& e )
  {
    MITK_ERROR << "Error encountered when loading DICOM series:" << e.what();
  }
  catch ( ... )
  {
    MITK_ERROR << "Unspecified error encountered when loading DICOM series.";
  }

  return nullptr;
}

#define switch3DnTCase( IOType, T ) \
  case IOType:                      \
    return LoadDICOMByITK3DnT<T>( filenamesLists, correctTilt, tiltInfo, io );

mitk::Image::Pointer mitk::ITKDICOMSeriesReaderHelper::Load3DnT( const StringContainerList& filenamesLists,
                                                                 bool correctTilt,
                                                                 const GantryTiltInformation& tiltInfo )
{
  if ( filenamesLists.empty() || filenamesLists.front().empty() )
  {
    MITK_DEBUG
      << "Calling LoadDicomSeries with empty filename string container. Probably invalid application logic.";
    return nullptr; // this is not actually an error but the result is very simple
  }

  typedef itk::GDCMImageIO DcmIoType;
  DcmIoType::Pointer io = DcmIoType::New();

  try
  {
    if ( io->CanReadFile( filenamesLists.front().front().c_str() ) )
    {
      io->SetFileName( filenamesLists.front().front().c_str() );
      io->ReadImageInformation();

      if ( io->GetPixelType() == itk::IOPixelEnum::SCALAR )
      {
        switch ( io->GetComponentType() )
        {
          switch3DnTCase(itk::IOComponentEnum::UCHAR, unsigned char) switch3DnTCase(itk::IOComponentEnum::CHAR, char)
              switch3DnTCase(itk::IOComponentEnum::USHORT, unsigned short) switch3DnTCase(
                itk::IOComponentEnum::SHORT, short) switch3DnTCase(itk::IOComponentEnum::UINT,
                                                        unsigned int) switch3DnTCase(itk::IOComponentEnum::INT, int)
                switch3DnTCase(itk::IOComponentEnum::ULONG, long unsigned int) switch3DnTCase(itk::IOComponentEnum::LONG, long int)
                  switch3DnTCase(itk::IOComponentEnum::FLOAT, float) switch3DnTCase(itk::IOComponentEnum::DOUBLE, double) default
            : MITK_ERROR
              << "Found unsupported DICOM scalar pixel type: (enum value) "
              << io->GetComponentType();
        }
      }
      else if ( io->GetPixelType() == itk::IOPixelEnum::RGB )
      {
        switch ( io->GetComponentType() )
        {
          switch3DnTCase(itk::IOComponentEnum::UCHAR, itk::RGBPixel<unsigned char>)
              switch3DnTCase(itk::IOComponentEnum::CHAR, itk::RGBPixel<char>) switch3DnTCase(
                itk::IOComponentEnum::USHORT, itk::RGBPixel<unsigned short>) switch3DnTCase(itk::IOComponentEnum::SHORT,
                                                                                 itk::RGBPixel<short>)
                switch3DnTCase(itk::IOComponentEnum::UINT, itk::RGBPixel<unsigned int>) switch3DnTCase(
                  itk::IOComponentEnum::INT, itk::RGBPixel<int>) switch3DnTCase(itk::IOComponentEnum::ULONG,
                                                                     itk::RGBPixel<long unsigned int>)
                  switch3DnTCase(itk::IOComponentEnum::LONG, itk::RGBPixel<long int>) switch3DnTCase(
                    itk::IOComponentEnum::FLOAT, itk::RGBPixel<float>) switch3DnTCase(itk::IOComponentEnum::DOUBLE,
                                                                           itk::RGBPixel<double>) default
            : MITK_ERROR
              << "Found unsupported DICOM scalar pixel type: (enum value) "
              << io->GetComponentType();
        }
      }

      MITK_ERROR << "Unsupported DICOM pixel type";
      return nullptr;
    }
  }
  catch ( const itk::MemoryAllocationError& e )
  {
    MITK_ERROR << "Out of memory. Cannot load DICOM series: " << e.what();
  }
  catch ( const std::exception& e )
  {
    MITK_ERROR << "Error encountered when loading DICOM series:" << e.what();
  }
  catch ( ... )
  {
    MITK_ERROR << "Unspecified error encountered when loading DICOM series.";
  }

  return nullptr;
}

bool ConvertDICOMDateTimeString( const std::string& dateString,
                                 const std::string& timeString,
                                 OFDateTime& time )
{
  OFString content( timeString.c_str() );

  if ( !dateString.empty() )
  {
    content = OFString( dateString.c_str() ).append( content );
  }
  else
  {
    // This is a workaround for DICOM data that has an AquisitionTime but no AquisitionDate.
    // In this case, we use the current date. That's not really nice, but is absolutely OK
    // as we're only interested in the time anyways...
    OFString currentDate;
    DcmDate::getCurrentDate( currentDate );
    content = currentDate.append( content );
  }

  const OFCondition result = DcmDateTime::getOFDateTimeFromString( content, time );

  return result.good();
}

boost::posix_time::ptime ConvertOFDateTimeToPTime( const OFDateTime& time )
{
  const boost::posix_time::ptime min_date_time(boost::posix_time::min_date_time);
  const boost::gregorian::date::year_type min_year = min_date_time.date().year();
  unsigned int year = time.getDate().getYear();

  if (year < min_year)
  {
    MITK_WARN << "Year " << year << " is before the supported minimum " << min_year
              << " in Boost.Date_Time. Adding " << min_year << " years to proceed.";
    year += min_year;
  }

  const boost::gregorian::date boostDate(
    year, time.getDate().getMonth(), time.getDate().getDay() );

  const boost::posix_time::time_duration boostTime =
    boost::posix_time::hours( time.getTime().getHour() )
    + boost::posix_time::minutes( time.getTime().getMinute() )
    + boost::posix_time::seconds( static_cast<int>(time.getTime().getSecond()) )
    + boost::posix_time::milliseconds( time.getTime().getMilliSecond() );

  boost::posix_time::ptime result( boostDate, boostTime );

  return result;
}

OFDateTime GetLowerDateTime( const OFDateTime& time1, const OFDateTime& time2 )
{
  OFDateTime result = time1;

  if ( ( time2.getDate() < time1.getDate() )
       || ( ( time2.getDate() == time1.getDate() ) && ( time2.getTime() < time1.getTime() ) ) )
  {
    result = time2;
  }

  return result;
}

OFDateTime GetUpperDateTime( const OFDateTime& time1, const OFDateTime& time2 )
{
  OFDateTime result = time1;

  if ( ( time2.getDate() > time1.getDate() )
       || ( ( time2.getDate() == time1.getDate() ) && ( time2.getTime() > time1.getTime() ) ) )
  {
    result = time2;
  }

  return result;
}

double ComputeMiliSecDuration( const OFDateTime& start, const OFDateTime& stop )
{
  const boost::posix_time::ptime startTime = ConvertOFDateTimeToPTime( start );
  const boost::posix_time::ptime stopTime  = ConvertOFDateTimeToPTime( stop );

  ::boost::posix_time::time_duration duration = stopTime - startTime;

  return duration.total_milliseconds();
}

bool mitk::ITKDICOMSeriesReaderHelper::ExtractDateTimeBoundsAndTriggerOfTimeStep(
  const StringContainer& filenamesOfTimeStep, DateTimeBounds& bounds, TimeBounds& triggerBounds)
{
  DICOMGDCMTagScanner::Pointer filescanner = DICOMGDCMTagScanner::New();
  filescanner->SetInputFiles(filenamesOfTimeStep);
  filescanner->AddTag(AcquisitionDateTag);
  filescanner->AddTag(AcquisitionTimeTag);
  filescanner->AddTag(TriggerTimeTag);
  filescanner->Scan();

  const DICOMDatasetAccessingImageFrameList frameList = filescanner->GetFrameInfoList();

  bool result = false;
  bool firstAq = true;
  bool firstTr = true;

  triggerBounds = TimeBounds(0.0);

  for (auto pos = frameList.cbegin(); pos != frameList.cend(); ++pos)
  {
    const std::string aqDateStr = (*pos)->GetTagValueAsString(AcquisitionDateTag).value;
    const std::string aqTimeStr = (*pos)->GetTagValueAsString(AcquisitionTimeTag).value;
    const std::string triggerTimeStr = (*pos)->GetTagValueAsString(TriggerTimeTag).value;

    OFDateTime aqDateTime;
    const bool convertAqResult = ConvertDICOMDateTimeString(aqDateStr, aqTimeStr, aqDateTime);

    OFBool convertTriggerResult;
    mitk::ScalarType triggerTime = OFStandard::atof(triggerTimeStr.c_str(), &convertTriggerResult);

    if (convertAqResult)
    {
      if (firstAq)
      {
        bounds[0] = aqDateTime;
        bounds[1] = aqDateTime;
        firstAq = false;
      }
      else
      {
        bounds[0] = GetLowerDateTime(bounds[0], aqDateTime);
        bounds[1] = GetUpperDateTime(bounds[1], aqDateTime);
      }
      result = true;
    }

    if (convertTriggerResult)
    {
      if (firstTr)
      {
        triggerBounds[0] = triggerTime;
        triggerBounds[1] = triggerTime;
        firstTr = false;
      }
      else
      {
        triggerBounds[0] = std::min(triggerBounds[0], triggerTime);
        triggerBounds[1] = std::max(triggerBounds[1], triggerTime);
      }
      result = true;
    }
  }

  return result;
};

bool mitk::ITKDICOMSeriesReaderHelper::ExtractTimeBoundsOfTimeStep(
  const StringContainer& filenamesOfTimeStep, TimeBounds& bounds, const OFDateTime& baselineDateTime, bool& usedTriggerBounds )
{
  DateTimeBounds aqDTBounds;
  TimeBounds triggerBounds;

  bool result = ExtractDateTimeBoundsAndTriggerOfTimeStep(filenamesOfTimeStep, aqDTBounds, triggerBounds);

  mitk::ScalarType lowerBound = ComputeMiliSecDuration( baselineDateTime, aqDTBounds[0] );
  mitk::ScalarType upperBound = ComputeMiliSecDuration( baselineDateTime, aqDTBounds[1] );
  if ( lowerBound < mitk::eps || upperBound < mitk::eps || usedTriggerBounds)
  {
    lowerBound = triggerBounds[0];
    upperBound = triggerBounds[1];
    usedTriggerBounds = true;
  }
  else
  {
    usedTriggerBounds = false;
  }

  bounds[0] = lowerBound;
  bounds[1] = upperBound;

  return result;
};

mitk::ITKDICOMSeriesReaderHelper::TimeBoundsList
  mitk::ITKDICOMSeriesReaderHelper::ExtractTimeBoundsOfTimeSteps(
    const StringContainerList& filenamesOfTimeSteps )
{
  TimeBoundsList result;

  OFDateTime baseLine;

  // extract the timebounds
  DateTimeBounds baselineDateTimeBounds;
  TimeBounds triggerBounds;
  auto pos = filenamesOfTimeSteps.cbegin();
  ExtractDateTimeBoundsAndTriggerOfTimeStep(*pos, baselineDateTimeBounds, triggerBounds);
  baseLine = baselineDateTimeBounds[0];

  // timebounds for baseline is 0
  TimeBounds bounds( 0.0 );
  result.push_back( bounds );

  //start with not using trigger time. Will be changed by ExtractTimeBoundsOfTimeStep if needed.
  bool usedTriggerTime = false;

  // iterate over the remaining timesteps
  for ( ++pos;
        pos != filenamesOfTimeSteps.cend();
        ++pos )
  {
    TimeBounds bounds( 0.0 );
    TimeBounds dateTimeBounds;

    // extract the timebounds relative to the baseline
    if ( ExtractTimeBoundsOfTimeStep( *pos, dateTimeBounds, baseLine, usedTriggerTime) )
    {
      bounds[0] = dateTimeBounds[0];
      bounds[1] = dateTimeBounds[1];
    }

    result.push_back( bounds );
  }

  if (usedTriggerTime)
    MITK_DEBUG << "Used trigger time to extract time bounds of passed files";

  return result;
};

mitk::TimeGeometry::Pointer
  mitk::ITKDICOMSeriesReaderHelper::GenerateTimeGeometry( const BaseGeometry* templateGeometry,
                                                          const TimeBoundsList& boundsList )
{
  TimeGeometry::Pointer timeGeometry;

  double check = 0.0;
  const auto boundListSize = boundsList.size();
  for ( std::size_t pos = 0; pos < boundListSize; ++pos )
  {
    check += boundsList[pos][0];
    check += boundsList[pos][1];
  }

  if ( check < mitk::eps )
  { // if all bounds are zero we assume that the bounds could not be correctly determined
    // and as a fallback generate a time geometry in the old mitk style
    ProportionalTimeGeometry::Pointer newTimeGeometry = ProportionalTimeGeometry::New();
    newTimeGeometry->Initialize( templateGeometry, boundListSize );
    timeGeometry = newTimeGeometry.GetPointer();
  }
  else
  {
    ArbitraryTimeGeometry::Pointer newTimeGeometry = ArbitraryTimeGeometry::New();
    newTimeGeometry->ClearAllGeometries();
    newTimeGeometry->ReserveSpaceForGeometries( boundListSize );

    for ( std::size_t pos = 0; pos < boundListSize; ++pos )
    {
      TimeBounds bounds = boundsList[pos];
      if ( pos + 1 < boundListSize )
      { //Currently we do not explicitly support "gaps" in the time coverage
        //thus we set the max time bound of a time step to the min time bound
        //of its successor.
        bounds[1] = boundsList[pos + 1][0];
      }

      newTimeGeometry->AppendNewTimeStepClone(templateGeometry, bounds[0], bounds[1]);
    }
    timeGeometry = newTimeGeometry.GetPointer();
  }

  return timeGeometry;
};
