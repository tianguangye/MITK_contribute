file(GLOB_RECURSE H_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

set(CPP_FILES
  mitkCoreActivator.cpp
  mitkCoreObjectFactory.cpp
  mitkCoreObjectFactoryBase.cpp
  mitkCoreServices.cpp
  mitkException.cpp

  Algorithms/mitkBaseDataSource.cpp
  Algorithms/mitkClippedSurfaceBoundsCalculator.cpp
  Algorithms/mitkCompareImageDataFilter.cpp
  Algorithms/mitkCompositePixelValueToString.cpp
  Algorithms/mitkConvert2Dto3DImageFilter.cpp
  Algorithms/mitkDataNodeSource.cpp
  Algorithms/mitkExtractSliceFilter.cpp
  Algorithms/mitkExtractSliceFilter2.cpp
  Algorithms/mitkHistogramGenerator.cpp
  Algorithms/mitkImageChannelSelector.cpp
  Algorithms/mitkImageSliceSelector.cpp
  Algorithms/mitkImageSource.cpp
  Algorithms/mitkImageTimeSelector.cpp
  Algorithms/mitkImageToImageFilter.cpp
  Algorithms/mitkImageToSurfaceFilter.cpp
  Algorithms/mitkMultiComponentImageDataComparisonFilter.cpp
  Algorithms/mitkPlaneGeometryDataToSurfaceFilter.cpp
  Algorithms/mitkPointSetSource.cpp
  Algorithms/mitkPointSetToPointSetFilter.cpp
  Algorithms/mitkRGBToRGBACastImageFilter.cpp
  Algorithms/mitkSubImageSelector.cpp
  Algorithms/mitkSurfaceSource.cpp
  Algorithms/mitkSurfaceToImageFilter.cpp
  Algorithms/mitkSurfaceToSurfaceFilter.cpp
  Algorithms/mitkTemporalJoinImagesFilter.cpp
  Algorithms/mitkUIDGenerator.cpp
  Algorithms/mitkVolumeCalculator.cpp

  Controllers/mitkBaseController.cpp
  Controllers/mitkCallbackFromGUIThread.cpp
  Controllers/mitkCameraController.cpp
  Controllers/mitkCameraRotationController.cpp
  Controllers/mitkCrosshairManager.cpp
  Controllers/mitkLimitedLinearUndo.cpp
  Controllers/mitkOperationEvent.cpp
  Controllers/mitkPlanePositionManager.cpp
  Controllers/mitkProgressBar.cpp
  Controllers/mitkRenderingManager.cpp
  Controllers/mitkSliceNavigationController.cpp
  Controllers/mitkSliceNavigationHelper.cpp
  Controllers/mitkStatusBar.cpp
  Controllers/mitkStepper.cpp
  Controllers/mitkTestManager.cpp
  Controllers/mitkTimeNavigationController.cpp
  Controllers/mitkUndoController.cpp
  Controllers/mitkVerboseLimitedLinearUndo.cpp
  Controllers/mitkVtkLayerController.cpp

  DataManagement/mitkAbstractTransformGeometry.cpp
  DataManagement/mitkAffineTransform3D.cpp
  DataManagement/mitkAnatomicalStructureColorPresets.cpp
  DataManagement/mitkAnnotationProperty.cpp
  DataManagement/mitkApplicationCursor.cpp
  DataManagement/mitkApplyTransformMatrixOperation.cpp
  DataManagement/mitkArbitraryTimeGeometry.cpp
  DataManagement/mitkBaseData.cpp
  DataManagement/mitkBaseGeometry.cpp
  DataManagement/mitkBaseProperty.cpp
  DataManagement/mitkChannelDescriptor.cpp
  DataManagement/mitkClippingProperty.cpp
  DataManagement/mitkColorProperty.cpp
  DataManagement/mitkCrosshairData.cpp
  DataManagement/mitkDataNode.cpp
  DataManagement/mitkDataStorage.cpp
  DataManagement/mitkEnumerationProperty.cpp
  DataManagement/mitkFloatPropertyExtension.cpp
  DataManagement/mitkGenericIDRelationRule.cpp
  DataManagement/mitkGeometry3D.cpp
  DataManagement/mitkGeometryData.cpp
  DataManagement/mitkGeometryTransformHolder.cpp
  DataManagement/mitkGroupTagProperty.cpp
  DataManagement/mitkIdentifiable.cpp
  DataManagement/mitkImage.cpp
  DataManagement/mitkImageAccessorBase.cpp
  DataManagement/mitkImageCaster.cpp
  DataManagement/mitkImageCastPart1.cpp
  DataManagement/mitkImageCastPart2.cpp
  DataManagement/mitkImageCastPart3.cpp
  DataManagement/mitkImageCastPart4.cpp
  DataManagement/mitkImageDataItem.cpp
  DataManagement/mitkImageDescriptor.cpp
  DataManagement/mitkImageReadAccessor.cpp
  DataManagement/mitkImageStatisticsHolder.cpp
  DataManagement/mitkImageVtkReadAccessor.cpp
  DataManagement/mitkImageVtkWriteAccessor.cpp
  DataManagement/mitkImageWriteAccessor.cpp
  DataManagement/mitkINodeSelectionService.cpp
  DataManagement/mitkIntPropertyExtension.cpp
  DataManagement/mitkIPersistenceService.cpp
  DataManagement/mitkIPropertyAliases.cpp
  DataManagement/mitkIPropertyDescriptions.cpp
  DataManagement/mitkIPropertyDeserialization.cpp
  DataManagement/mitkIPropertyExtensions.cpp
  DataManagement/mitkIPropertyFilters.cpp
  DataManagement/mitkIPropertyOwner.cpp
  DataManagement/mitkIPropertyPersistence.cpp
  DataManagement/mitkIPropertyProvider.cpp
  DataManagement/mitkIPropertyRelations.cpp
  DataManagement/mitkITKEventObserverGuard.cpp
  DataManagement/mitkLandmarkProjector.cpp
  DataManagement/mitkLandmarkProjectorBasedCurvedGeometry.cpp
  DataManagement/mitkLevelWindow.cpp
  DataManagement/mitkLevelWindowManager.cpp
  DataManagement/mitkLevelWindowPreset.cpp
  DataManagement/mitkLevelWindowProperty.cpp
  DataManagement/mitkLookupTable.cpp
  DataManagement/mitkLookupTableProperty.cpp
  DataManagement/mitkLookupTables.cpp
  DataManagement/mitkMaterial.cpp
  DataManagement/mitkMemoryUtilities.cpp
  DataManagement/mitkModalityProperty.cpp
  DataManagement/mitkModifiedLock.cpp
  DataManagement/mitkNodePredicateAnd.cpp
  DataManagement/mitkNodePredicateBase.cpp
  DataManagement/mitkNodePredicateCompositeBase.cpp
  DataManagement/mitkNodePredicateData.cpp
  DataManagement/mitkNodePredicateDataProperty.cpp
  DataManagement/mitkNodePredicateDataType.cpp
  DataManagement/mitkNodePredicateDataUID.cpp
  DataManagement/mitkNodePredicateDimension.cpp
  DataManagement/mitkNodePredicateFunction.cpp
  DataManagement/mitkNodePredicateGeometry.cpp
  DataManagement/mitkNodePredicateNot.cpp
  DataManagement/mitkNodePredicateOr.cpp
  DataManagement/mitkNodePredicateProperty.cpp
  DataManagement/mitkNodePredicateSubGeometry.cpp
  DataManagement/mitkNodeSelectionService.cpp
  DataManagement/mitkNumericConstants.cpp
  DataManagement/mitkPlaneGeometry.cpp
  DataManagement/mitkPlaneGeometryData.cpp
  DataManagement/mitkPlaneOperation.cpp
  DataManagement/mitkPlaneOrientationProperty.cpp
  DataManagement/mitkPointOperation.cpp
  DataManagement/mitkPointSet.cpp
  DataManagement/mitkPointSetShapeProperty.cpp
  DataManagement/mitkProperties.cpp
  DataManagement/mitkPropertyAliases.cpp
  DataManagement/mitkPropertyDescriptions.cpp
  DataManagement/mitkPropertyDeserialization.cpp
  DataManagement/mitkPropertyExtension.cpp
  DataManagement/mitkPropertyExtensions.cpp
  DataManagement/mitkPropertyFilter.cpp
  DataManagement/mitkPropertyFilters.cpp
  DataManagement/mitkPropertyKeyPath.cpp
  DataManagement/mitkPropertyList.cpp
  DataManagement/mitkPropertyListReplacedObserver.cpp
  DataManagement/mitkPropertyNameHelper.cpp
  DataManagement/mitkPropertyObserver.cpp
  DataManagement/mitkPropertyPersistence.cpp
  DataManagement/mitkPropertyPersistenceInfo.cpp
  DataManagement/mitkPropertyRelationRuleBase.cpp
  DataManagement/mitkPropertyRelations.cpp
  DataManagement/mitkProportionalTimeGeometry.cpp
  DataManagement/mitkRenderingModeProperty.cpp
  DataManagement/mitkResliceMethodProperty.cpp
  DataManagement/mitkRestorePlanePositionOperation.cpp
  DataManagement/mitkRotationOperation.cpp
  DataManagement/mitkScaleOperation.cpp
  DataManagement/mitkSlicedData.cpp
  DataManagement/mitkSlicedGeometry3D.cpp
  DataManagement/mitkSmartPointerProperty.cpp
  DataManagement/mitkSourceImageRelationRule.cpp
  DataManagement/mitkStandaloneDataStorage.cpp
  DataManagement/mitkStringProperty.cpp
  DataManagement/mitkSurface.cpp
  DataManagement/mitkSurfaceOperation.cpp
  DataManagement/mitkTemporoSpatialStringProperty.cpp
  DataManagement/mitkThinPlateSplineCurvedGeometry.cpp
  DataManagement/mitkTimeGeometry.cpp
  DataManagement/mitkTransferFunction.cpp
  DataManagement/mitkTransferFunctionInitializer.cpp
  DataManagement/mitkTransferFunctionProperty.cpp
  DataManagement/mitkUIDManipulator.cpp
  DataManagement/mitkVectorProperty.cpp
  DataManagement/mitkVtkInterpolationProperty.cpp
  DataManagement/mitkVtkRepresentationProperty.cpp
  DataManagement/mitkVtkResliceInterpolationProperty.cpp
  DataManagement/mitkVtkScalarModeProperty.cpp
  DataManagement/mitkWeakPointerProperty.cpp

  Interactions/mitkAction.cpp
  Interactions/mitkBindDispatcherInteractor.cpp
  Interactions/mitkDataInteractor.cpp
  Interactions/mitkDispatcher.cpp
  Interactions/mitkDisplayActionEventBroadcast.cpp
  Interactions/mitkDisplayActionEventFunctions.cpp
  Interactions/mitkDisplayActionEventHandler.cpp
  Interactions/mitkDisplayActionEventHandlerDesynchronized.cpp
  Interactions/mitkDisplayActionEventHandlerStd.cpp
  Interactions/mitkDisplayActionEventHandlerSynchronized.cpp
  Interactions/mitkDisplayCoordinateOperation.cpp
  Interactions/mitkEventConfig.cpp
  Interactions/mitkEventFactory.cpp
  Interactions/mitkEventRecorder.cpp
  Interactions/mitkEventStateMachine.cpp
  Interactions/mitkInteractionEvent.cpp
  Interactions/mitkInteractionEventConst.cpp
  Interactions/mitkInteractionEventHandler.cpp
  Interactions/mitkInteractionEventObserver.cpp
  Interactions/mitkInteractionKeyEvent.cpp
  Interactions/mitkInteractionPositionEvent.cpp
  Interactions/mitkInteractionSchemeSwitcher.cpp
  Interactions/mitkInternalEvent.cpp
  Interactions/mitkMouseDoubleClickEvent.cpp
  Interactions/mitkMouseMoveEvent.cpp
  Interactions/mitkMousePressEvent.cpp
  Interactions/mitkMouseReleaseEvent.cpp
  Interactions/mitkMouseWheelEvent.cpp
  Interactions/mitkPointSetDataInteractor.cpp
  Interactions/mitkSinglePointDataInteractor.cpp
  Interactions/mitkStateMachineAction.cpp
  Interactions/mitkStateMachineCondition.cpp
  Interactions/mitkStateMachineContainer.cpp
  Interactions/mitkStateMachineState.cpp
  Interactions/mitkStateMachineTransition.cpp
  Interactions/mitkVtkEventAdapter.cpp
  Interactions/mitkVtkInteractorStyle.cxx
  Interactions/mitkXML2EventParser.cpp

  IO/mitkAbstractFileIO.cpp
  IO/mitkAbstractFileReader.cpp
  IO/mitkAbstractFileWriter.cpp
  IO/mitkCustomMimeType.cpp
  IO/mitkFileReader.cpp
  IO/mitkFileReaderRegistry.cpp
  IO/mitkFileReaderSelector.cpp
  IO/mitkFileReaderWriterBase.cpp
  IO/mitkFileWriter.cpp
  IO/mitkFileWriterRegistry.cpp
  IO/mitkFileWriterSelector.cpp
  IO/mitkGeometry3DToXML.cpp
  IO/mitkGeometryDataReaderService.cpp
  IO/mitkGeometryDataWriterService.cpp
  IO/mitkIFileIO.cpp
  IO/mitkIFileReader.cpp
  IO/mitkIFileWriter.cpp
  IO/mitkImageVtkLegacyIO.cpp
  IO/mitkImageVtkXmlIO.cpp
  IO/mitkIMimeTypeProvider.cpp
  IO/mitkIOConstants.cpp
  IO/mitkIOMetaInformationPropertyConstants.cpp
  IO/mitkIOMimeTypes.cpp
  IO/mitkIOUtil.cpp
  IO/mitkIOVolumeSplitReason.cpp
  IO/mitkIPreferences.cpp
  IO/mitkIPreferencesService.cpp
  IO/mitkIPreferencesStorage.cpp
  IO/mitkItkImageIO.cpp
  IO/mitkItkLoggingAdapter.cpp
  IO/mitkLegacyFileReaderService.cpp
  IO/mitkLegacyFileWriterService.cpp
  IO/mitkLocaleSwitch.cpp
  IO/mitkLogBackend.cpp
  IO/mitkMimeType.cpp
  IO/mitkMimeTypeProvider.cpp
  IO/mitkOperation.cpp
  IO/mitkPixelType.cpp
  IO/mitkPointSetReaderService.cpp
  IO/mitkPointSetWriterService.cpp
  IO/mitkPreferenceListReaderOptionsFunctor.cpp
  IO/mitkPreferences.cpp
  IO/mitkPreferencesService.cpp
  IO/mitkProportionalTimeGeometryToXML.cpp
  IO/mitkRawImageFileReader.cpp
  IO/mitkStandardFileLocations.cpp
  IO/mitkSurfaceStlIO.cpp
  IO/mitkSurfaceVtkIO.cpp
  IO/mitkSurfaceVtkLegacyIO.cpp
  IO/mitkSurfaceVtkXmlIO.cpp
  IO/mitkUtf8Util.cpp
  IO/mitkVtkLoggingAdapter.cpp
  IO/mitkXMLPreferencesStorage.cpp

  Rendering/mitkAbstractAnnotationRenderer.cpp
  Rendering/mitkAnnotation.cpp
  Rendering/mitkAnnotationUtils.cpp
  Rendering/mitkBaseRenderer.cpp
  Rendering/mitkBaseRendererHelper.cpp
  Rendering/mitkCrosshairVtkMapper2D.cpp
  Rendering/mitkGradientBackground.cpp
  Rendering/mitkImageVtkMapper2D.cpp
  Rendering/mitkMapper.cpp
  Rendering/mitkPlaneGeometryDataMapper2D.cpp
  Rendering/mitkPlaneGeometryDataVtkMapper3D.cpp
  Rendering/mitkPointSetVtkMapper2D.cpp
  Rendering/mitkPointSetVtkMapper3D.cpp
  Rendering/mitkRenderWindow.cpp
  Rendering/mitkRenderWindowBase.cpp
  Rendering/mitkRenderWindowFrame.cpp
  Rendering/mitkSurfaceVtkMapper2D.cpp
  Rendering/mitkSurfaceVtkMapper3D.cpp
  Rendering/mitkVideoRecorder.cpp
  Rendering/mitkVtkEventProvider.cpp
  Rendering/mitkVtkMapper.cpp
  Rendering/mitkVtkPropRenderer.cpp
  Rendering/mitkVtkWidgetRendering.cpp
  Rendering/vtkMitkLevelWindowFilter.cpp
  Rendering/vtkMitkRectangleProp.cpp
  Rendering/vtkMitkRenderProp.cpp
  Rendering/vtkMitkThickSlicesFilter.cpp
  Rendering/vtkNeverTranslucentTexture.cpp
)

set(RESOURCE_FILES
Interactions/globalConfig.xml
Interactions/DisplayInteraction.xml
Interactions/DisplayConfigMITKBase.xml
Interactions/DisplayConfigPACSBase.xml
Interactions/DisplayConfigCrosshair.xml
Interactions/DisplayConfigRotation.xml
Interactions/DisplayConfigActivateCoupling.xml
Interactions/DisplayConfigSwivel.xml
Interactions/DisplayConfigPACSPan.xml
Interactions/DisplayConfigPACSScroll.xml
Interactions/DisplayConfigPACSZoom.xml
Interactions/DisplayConfigPACSLevelWindow.xml
Interactions/DisplayConfigBlockLMB.xml
Interactions/PointSet.xml
Interactions/PointSetConfig.xml
Interactions/PointSetConfigLMB.xml

mitkLevelWindowPresets.xml
mitkAnatomicalStructureColorPresets.xml
)
