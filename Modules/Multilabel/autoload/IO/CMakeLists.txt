mitk_create_module(MultilabelIO
  DEPENDS PUBLIC MitkMultilabel MitkSceneSerialization
  PACKAGE_DEPENDS PRIVATE ITK|IONRRD ITK|IONIFTI
  AUTOLOAD_WITH MitkCore
)
