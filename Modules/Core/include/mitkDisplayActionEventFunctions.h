/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkDisplayActionEventFunctions_h
#define mitkDisplayActionEventFunctions_h

#include <MitkCoreExports.h>

#include "mitkStdFunctionCommand.h"

namespace mitk
{
  namespace DisplayActionEventFunctions
  {
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayMoveEvent'.
    *   The function performs a move of the camera controller of the sending renderer by a vector
    *   that was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from
    *   renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction MoveSenderCameraAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplaySetCrosshairEvent'.
    *   The function performs a slice selection of the slice navigation controller and will set
    *   the cross hair for all 2D-render windows.
    *   The new position was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from
    *   renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction SetCrosshairAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayZoomEvent'.
    *   The function performs a zoom of the camera controller of the sending renderer by a zoom factor
    *   that was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from
    *   renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction ZoomSenderCameraAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayScrollEvent'.
    *   The function performs a slice scrolling of the slice navigation controller of the sending renderer.
    *   The new position was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from
    *   renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction ScrollSliceStepperAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplaySetLevelWindowEvent'.
    *   The function sets the 'levelwindow' property of the topmost visible image that is display by the sending renderer.
    *   The level and window value for this property were previously determined by the mouse interaction event.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction SetLevelWindowAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayMoveEvent'.
    *   The function performs a move of the camera controller of all renderer (synchronized)
    *   by a vector that was previously determined by the mouse interaction event.
    *   The renderer need to be managed by the same rendering manager.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from / send
    *   changes to renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction MoveCameraSynchronizedAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplaySetCrosshairEvent'.
    *   The function performs a slice selection of the slice navigation controller and will set
    *   the cross hair for all 2D-render windows.
    *   The new position was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from / send
    *   changes to renderers whose name begins with this prefix.
    * @todo Currently there is no need to distinguish between this and the non-synchronized version
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction SetCrosshairSynchronizedAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayZoomEvent'.
    *   The function performs a zoom of the camera controller of all 2D-renderer (synchronized)
    *   by a zoom factor that was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from / send
    *   changes to renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction ZoomCameraSynchronizedAction(const std::string& prefixFilter = "");
    /**
    * @brief Returns an 'std::function' that can be used  to react on the 'DisplayScrollEvent'.
    *   The function performs a slice scrolling of the slice navigation controller of all 2D-renderer (synchronized).
    *   The new position was previously determined by the mouse interaction event.
    *
    * @param prefixFilter The prefix of associated renderer names. The action will only react to events from / send
    *   changes to renderers whose name begins with this prefix.
    */
    MITKCORE_EXPORT StdFunctionCommand::ActionFunction ScrollSliceStepperSynchronizedAction(const std::string& prefixFilter = "");

  } // end namespace DisplayActionEventFunctions
} // end namespace mitk

#endif
