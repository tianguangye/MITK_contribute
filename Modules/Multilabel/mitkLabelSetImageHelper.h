/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkLabelSetImageHelper_h
#define mitkLabelSetImageHelper_h

#include <MitkMultilabelExports.h>

#include <mitkDataNode.h>
#include <mitkLabelSetImage.h>

namespace mitk
{
  class DataStorage;

  namespace LabelSetImageHelper
  {
    /**
     * @brief This function creates and returns a new empty segmentation data node.
     * @remark The data is not set. Set it manually to have a properly setup node.
     * @param segmentationName A name for the new segmentation node.
     * @return The new segmentation node as a data node pointer.
     */
    MITKMULTILABEL_EXPORT mitk::DataNode::Pointer CreateEmptySegmentationNode(const std::string& segmentationName = std::string());

    /**
     * @brief This function creates and returns a new data node with a new empty segmentation
     * data structure.
     * The segmentation node is named according to the given reference data node, otherwise a name
     * is passed explicitly.
     * Some properties are set to ensure a proper setup segmentation and node
     * (e.g. link the segmentation node with its parent node).
     *
     * @param referenceNode             The reference node from which the name of the new segmentation node
     *                                  is derived. If passed pointer is null, the name "unknown" will be used.
     * @param initialSegmentationImage  The segmentation image that is used to initialize the label set image.
     * @param segmentationName          An optional name for the new segmentation node.
     * @param dataStorage               The data storage of the reference node (if given, used to generate a unique node name).
     *
     * @return                          The new segmentation node as a data node pointer.
     */
    MITKMULTILABEL_EXPORT mitk::DataNode::Pointer CreateNewSegmentationNode(const DataNode* referenceNode,
      const Image* initialSegmentationImage = nullptr, const std::string& segmentationName = std::string(),
      const DataStorage* dataStorage = nullptr);

    /**
     * @brief This function creates and returns a new label. The label is automatically assigned an
     *        unused generic label name, depending on existing label names in all label sets of the
     *        given label set image.
     *        The color of the label is selected from the MULTILABEL lookup table, following the same
     *        rules of the naming to likely chose a unique color.
     *
     * @param labelSetImage   The label set image that the new label is added to
     * @param namePrefix      The prefix of the label name that is prepended by a sequential number
     * @param hideIDIfUnique Indicates if the ID suffix should be added if the label name prefix would be already unique.
     * true: only add if not unique; false: add always.
     *
     * @return                The new label.
     */
    MITKMULTILABEL_EXPORT Label::Pointer CreateNewLabel(const MultiLabelSegmentation* labelSetImage, const std::string& namePrefix = "Label", bool hideIDIfUnique = false);

    using GroupIDToLabelValueMapType = std::map<mitk::MultiLabelSegmentation::GroupIndexType, MultiLabelSegmentation::LabelValueVectorType>;
    MITKMULTILABEL_EXPORT GroupIDToLabelValueMapType SplitLabelValuesByGroup(const MultiLabelSegmentation* labelSetImage, const MultiLabelSegmentation::LabelValueVectorType& labelValues);

    using LabelClassNameToLabelValueMapType = std::map<std::string, MultiLabelSegmentation::LabelValueVectorType>;
    MITKMULTILABEL_EXPORT LabelClassNameToLabelValueMapType SplitLabelValuesByClassName(const MultiLabelSegmentation* labelSetImage, MultiLabelSegmentation::GroupIndexType groupID);
    MITKMULTILABEL_EXPORT LabelClassNameToLabelValueMapType SplitLabelValuesByClassName(const MultiLabelSegmentation* labelSetImage, MultiLabelSegmentation::GroupIndexType groupID, const MultiLabelSegmentation::LabelValueVectorType& labelValues);

    /** Type that represents the mapping from a source group id to relevant target group ids (first map) and then to mapping to respective
    label value mapping vectors for each target group id (inner/second map).*/
    using SourceToTargetGroupIDToLabelValueMappingMapType = std::map<MultiLabelSegmentation::GroupIndexType, std::map<MultiLabelSegmentation::GroupIndexType, LabelValueMappingVector> >;
    /** Helper function that takes a label mapping vector and deduces which group ids are needed in the source segmentation and target segmentation to conduct a mapping of pixel values.
    The result is e.g. used by TransferLabelContent.*/
    MITKMULTILABEL_EXPORT SourceToTargetGroupIDToLabelValueMappingMapType SplitLabelValueMappingBySourceAndTargetGroup(const MultiLabelSegmentation* sourceSeg, const MultiLabelSegmentation* targetSeg, const LabelValueMappingVector& labelMapping);

    MITKMULTILABEL_EXPORT std::string CreateDisplayGroupName(const MultiLabelSegmentation* labelSetImage, MultiLabelSegmentation::GroupIndexType groupID);

    /** Helper that creates the human readable display name for a label. If label segmentation image is not null, the function will also check
    * if the label is the only label with its name in the segmentation. If not, the tracking ID will be added in square brackets to the display
    * name.
    */
    MITKMULTILABEL_EXPORT std::string CreateDisplayLabelName(const MultiLabelSegmentation* labelSetImage, const Label* label);

    /** Helper that creates a HTML string that contains the display name and a square glyph with the color of the label.
    */
    MITKMULTILABEL_EXPORT std::string CreateHTMLLabelName(const Label* label, const MultiLabelSegmentation* segmentation = nullptr);
  } // namespace LabelSetImageHelper
} // namespace mitk

#endif
