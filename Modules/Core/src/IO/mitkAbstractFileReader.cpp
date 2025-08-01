/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <mitkAbstractFileReader.h>

#include <mitkCustomMimeType.h>
#include <mitkIOUtil.h>
#include <mitkUtf8Util.h>

#include <mitkFileReaderWriterBase.h>
#include <mitkVersion.h>
#include <mitkIOMetaInformationPropertyConstants.h>

#include <usGetModuleContext.h>
#include <usModuleContext.h>
#include <usPrototypeServiceFactory.h>

#include <itksys/SystemTools.hxx>

#include <fstream>

namespace mitk
{
  AbstractFileReader::InputStream::InputStream(IFileReader *reader, std::ios_base::openmode mode)
    : std::istream(nullptr), m_Stream(nullptr)
  {
    std::istream *stream = reader->GetInputStream();
    if (stream)
    {
      this->init(stream->rdbuf());
    }
    else
    {
      m_Stream = new std::ifstream(reader->GetInputLocation().c_str(), mode);
      this->init(m_Stream->rdbuf());
    }
  }

  AbstractFileReader::InputStream::~InputStream() { delete m_Stream; }
  class AbstractFileReader::Impl : public FileReaderWriterBase
  {
  public:
    Impl() : FileReaderWriterBase(), m_Stream(nullptr), m_PrototypeFactory(nullptr) {}
    Impl(const Impl &other) : FileReaderWriterBase(other), m_Stream(nullptr), m_PrototypeFactory(nullptr) {}
    std::string m_Location;
    std::string m_TmpFile;
    std::istream *m_Stream;

    us::PrototypeServiceFactory *m_PrototypeFactory;
    us::ServiceRegistration<IFileReader> m_Reg;

    const PropertyList* m_Properties;
  };

  AbstractFileReader::AbstractFileReader() : d(new Impl) {}
  AbstractFileReader::~AbstractFileReader()
  {
    UnregisterService();

    delete d->m_PrototypeFactory;

    if (!d->m_TmpFile.empty())
    {
      std::remove(d->m_TmpFile.c_str());
    }
  }

  AbstractFileReader::AbstractFileReader(const AbstractFileReader &other) : IFileReader(), d(new Impl(*other.d.get()))
  {
  }

  AbstractFileReader::AbstractFileReader(const CustomMimeType &mimeType, const std::string &description) : d(new Impl)
  {
    d->SetMimeType(mimeType);
    d->SetDescription(description);
  }

  ////////////////////// Reading /////////////////////////

  std::vector<BaseData::Pointer> AbstractFileReader::Read()
  {
    std::vector<BaseData::Pointer> result = this->DoRead();

    const auto options = this->GetOptions();

    for (auto& data : result)
    {
      data->SetProperty(PropertyKeyPathToPropertyName(IOMetaInformationPropertyConstants::READER_DESCRIPTION()), StringProperty::New(d->GetDescription()));
      data->SetProperty(PropertyKeyPathToPropertyName(IOMetaInformationPropertyConstants::READER_VERSION()), StringProperty::New(MITK_VERSION_STRING));
      data->SetProperty(PropertyKeyPathToPropertyName(IOMetaInformationPropertyConstants::READER_MIME_NAME()), StringProperty::New(d->GetMimeType()->GetName()));
      data->SetProperty(PropertyKeyPathToPropertyName(IOMetaInformationPropertyConstants::READER_MIME_CATEGORY()), StringProperty::New(d->GetMimeType()->GetCategory()));
      if (this->GetInputStream() == nullptr)
      {
        data->SetProperty(PropertyKeyPathToPropertyName(IOMetaInformationPropertyConstants::READER_INPUTLOCATION()), StringProperty::New(Utf8Util::Local8BitToUtf8(this->GetInputLocation())));
      }

      for (const auto& option : options)
      {
        auto optionpath = IOMetaInformationPropertyConstants::READER_OPTION_ROOT().AddElement(option.first);
        data->SetProperty(PropertyKeyPathToPropertyName(optionpath), StringProperty::New(option.second.ToString()));
      }
    }

    return result;
  }

  DataStorage::SetOfObjects::Pointer AbstractFileReader::Read(DataStorage &ds)
  {
    DataStorage::SetOfObjects::Pointer result = DataStorage::SetOfObjects::New();
    std::vector<BaseData::Pointer> data = this->Read();
    for (auto iter = data.begin(); iter != data.end(); ++iter)
    {
      mitk::DataNode::Pointer node = mitk::DataNode::New();
      node->SetData(*iter);
      this->SetDefaultDataNodeProperties(node, this->GetInputLocation());
      ds.Add(node);
      result->InsertElement(result->Size(), node);
    }
    return result;
  }

  IFileReader::ConfidenceLevel AbstractFileReader::GetConfidenceLevel() const
  {
    if (d->m_Stream)
    {
      if (*d->m_Stream)
        return Supported;
    }
    else
    {
      if (itksys::SystemTools::FileExists(Utf8Util::Local8BitToUtf8(this->GetInputLocation()).c_str(), true))
      {
        return Supported;
      }
    }
    return Unsupported;
  }

  //////////// µS Registration & Properties //////////////

  us::ServiceRegistration<IFileReader> AbstractFileReader::RegisterService(us::ModuleContext *context)
  {
    if (d->m_PrototypeFactory)
      return us::ServiceRegistration<IFileReader>();

    if (context == nullptr)
    {
      context = us::GetModuleContext();
    }

    d->RegisterMimeType(context);

    if (this->GetMimeType()->GetName().empty())
    {
      MITK_WARN << "Not registering reader due to empty MIME type.";
      return us::ServiceRegistration<IFileReader>();
    }

    struct PrototypeFactory : public us::PrototypeServiceFactory
    {
      AbstractFileReader *const m_Prototype;

      PrototypeFactory(AbstractFileReader *prototype) : m_Prototype(prototype) {}
      us::InterfaceMap GetService(us::Module * /*module*/,
                                  const us::ServiceRegistrationBase & /*registration*/) override
      {
        return us::MakeInterfaceMap<IFileReader>(m_Prototype->Clone());
      }

      void UngetService(us::Module * /*module*/,
                        const us::ServiceRegistrationBase & /*registration*/,
                        const us::InterfaceMap &service) override
      {
        delete us::ExtractInterface<IFileReader>(service);
      }
    };

    d->m_PrototypeFactory = new PrototypeFactory(this);
    us::ServiceProperties props = this->GetServiceProperties();
    d->m_Reg = context->RegisterService<IFileReader>(d->m_PrototypeFactory, props);
    return d->m_Reg;
  }

  void AbstractFileReader::UnregisterService()
  {
    try
    {
      if (d->m_Reg.IsAvailable())
        d->m_Reg.Unregister();
    }
    catch (const std::exception &)
    {
    }
  }

  us::ServiceProperties AbstractFileReader::GetServiceProperties() const
  {
    us::ServiceProperties result;

    result[IFileReader::PROP_DESCRIPTION()] = this->GetDescription();
    result[IFileReader::PROP_MIMETYPE()] = this->GetMimeType()->GetName();
    result[us::ServiceConstants::SERVICE_RANKING()] = this->GetRanking();
    return result;
  }

  us::ServiceRegistration<CustomMimeType> AbstractFileReader::RegisterMimeType(us::ModuleContext *context)
  {
    return d->RegisterMimeType(context);
  }

  std::vector< std::string > AbstractFileReader::GetReadFiles(){ return m_ReadFiles; }

  void AbstractFileReader::SetMimeType(const CustomMimeType &mimeType) { d->SetMimeType(mimeType); }
  void AbstractFileReader::SetDescription(const std::string &description) { d->SetDescription(description); }
  void AbstractFileReader::SetRanking(int ranking) { d->SetRanking(ranking); }
  int AbstractFileReader::GetRanking() const { return d->GetRanking(); }
  std::string AbstractFileReader::GetLocalFileName() const
  {
    std::string localFileName;
    if (d->m_Stream)
    {
      if (d->m_TmpFile.empty())
      {
        // write the stream contents to temporary file
        std::string ext = Utf8Util::Utf8ToLocal8Bit(itksys::SystemTools::GetFilenameExtension(Utf8Util::Local8BitToUtf8(this->GetInputLocation())));
        std::ofstream tmpStream;
        localFileName = mitk::IOUtil::CreateTemporaryFile(
          tmpStream, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary, "XXXXXX" + ext);
        tmpStream << d->m_Stream->rdbuf();
        d->m_TmpFile = localFileName;
      }
      else
      {
        localFileName = d->m_TmpFile;
      }
    }
    else
    {
      localFileName = d->m_Location;
    }
    return localFileName;
  }

  //////////////////////// Options ///////////////////////

  void AbstractFileReader::SetDefaultOptions(const IFileReader::Options &defaultOptions)
  {
    d->SetDefaultOptions(defaultOptions);
  }

  IFileReader::Options AbstractFileReader::GetDefaultOptions() const { return d->GetDefaultOptions(); }
  void AbstractFileReader::SetInput(const std::string &location)
  {
    d->m_Location = location;
    d->m_Stream = nullptr;
  }

  void AbstractFileReader::SetInput(const std::string &location, std::istream *is)
  {
    if (d->m_Stream != is && !d->m_TmpFile.empty())
    {
      std::remove(d->m_TmpFile.c_str());
      d->m_TmpFile.clear();
    }
    d->m_Location = location;
    d->m_Stream = is;
  }

  std::string AbstractFileReader::GetInputLocation() const { return d->m_Location; }
  std::istream *AbstractFileReader::GetInputStream() const { return d->m_Stream; }
  MimeType AbstractFileReader::GetRegisteredMimeType() const { return d->GetRegisteredMimeType(); }
  IFileReader::Options AbstractFileReader::GetOptions() const { return d->GetOptions(); }
  us::Any AbstractFileReader::GetOption(const std::string &name) const { return d->GetOption(name); }
  void AbstractFileReader::SetOptions(const Options &options) { d->SetOptions(options); }
  void AbstractFileReader::SetOption(const std::string &name, const us::Any &value) { d->SetOption(name, value); }
  ////////////////// MISC //////////////////

  void AbstractFileReader::AddProgressCallback(const ProgressCallback &callback) { d->AddProgressCallback(callback); }
  void AbstractFileReader::RemoveProgressCallback(const ProgressCallback &callback)
  {
    d->RemoveProgressCallback(callback);
  }

  void AbstractFileReader::SetProperties(const PropertyList* properties)
  {
    d->m_Properties = properties;
  }

  const PropertyList* AbstractFileReader::GetProperties() const
  {
    return d->m_Properties;
  }

  ////////////////// µS related Getters //////////////////

  const CustomMimeType *AbstractFileReader::GetMimeType() const { return d->GetMimeType(); }
  void AbstractFileReader::SetMimeTypePrefix(const std::string &prefix) { d->SetMimeTypePrefix(prefix); }
  std::string AbstractFileReader::GetMimeTypePrefix() const { return d->GetMimeTypePrefix(); }
  std::string AbstractFileReader::GetDescription() const { return d->GetDescription(); }
  void AbstractFileReader::SetDefaultDataNodeProperties(DataNode *node, const std::string &filePath)
  {
    // path
    if (!filePath.empty())
    {
      auto path = itksys::SystemTools::GetFilenamePath(Utf8Util::Local8BitToUtf8(filePath));
      node->SetProperty(StringProperty::PATH, mitk::StringProperty::New(path));
    }

    // name already defined?
    mitk::StringProperty::Pointer nameProp = dynamic_cast<mitk::StringProperty *>(node->GetProperty("name", nullptr, false));
    if (nameProp.IsNull() || nameProp->GetValue() == DataNode::NO_NAME_VALUE())
    {
      // name already defined in BaseData
      mitk::StringProperty::Pointer baseDataNameProp =
        dynamic_cast<mitk::StringProperty *>(node->GetData()->GetProperty("name").GetPointer());
      if (baseDataNameProp.IsNull() || baseDataNameProp->GetValue() == DataNode::NO_NAME_VALUE())
      {
        // name neither defined in node, nor in BaseData -> name = filebasename;
        auto name = this->GetRegisteredMimeType().GetFilenameWithoutExtension(filePath);
        name = Utf8Util::Local8BitToUtf8(name);
        nameProp = mitk::StringProperty::New(name);
        node->SetProperty("name", nameProp);
      }
    }

    // visibility
    if (!node->GetProperty("visible"))
    {
      node->SetVisibility(true);
    }
  }
}
