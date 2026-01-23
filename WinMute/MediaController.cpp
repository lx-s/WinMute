#include "common.h"
#include "MediaController.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt;
using namespace winrt::Windows::Media::Control;

bool MediaController::TryPauseCurrentSession(bool &paused_media)
{
   auto &log = WMLog::GetInstance();
   try {
      auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
      auto session = manager.GetCurrentSession();

      if (session) {
         auto playbackInfo = session.GetPlaybackInfo();
         auto status = playbackInfo.PlaybackStatus();

         if (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
            session.TryPauseAsync().get();
            log.LogInfo(L"MediaController: Paused current media session.");
            paused_media = true;
         } else {
            log.LogDebug(L"MediaController: Media is not playing, no action needed.\n");
            paused_media = false;
         }
      }
   } catch (const hresult_error& e) {
      log.LogError(L"MediaController: Failed to pause current media session. Error: %s",
                   e.message().c_str());
      return false;
   }
   return true;
}

bool MediaController::TryPlayCurrentSession()
{
   auto &log = WMLog::GetInstance();
   try {
      auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
      auto session = manager.GetCurrentSession();

      if (session) {
         auto playbackInfo = session.GetPlaybackInfo();
         auto status = playbackInfo.PlaybackStatus();

         if (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused) {
            session.TryPlayAsync().get();
            log.LogInfo(L"MediaController: Resumed current media session.");
         }
      }
   } catch (const hresult_error &e) {
      log.LogError(L"MediaController: Failed to resume current media session. Error: %s",
                   e.message().c_str());
      return false;
   }
   return true;
}
