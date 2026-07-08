#include "common.h"
#include "MediaController.h"

#include <functional>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt;
using namespace winrt::Windows::Media::Control;

MediaPlaybackController::MediaPlaybackController()
   : worker_(std::bind_front(&MediaPlaybackController::WorkerLoop, this))
{
}

void MediaPlaybackController::RequestPause()
{
   {
      const std::lock_guard lock(taskMutex_);
      tasks_.push_back(MediaTask::Pause);
   }
   taskAvailable_.notify_one();
}

void MediaPlaybackController::RequestResume()
{
   {
      const std::lock_guard lock(taskMutex_);
      tasks_.push_back(MediaTask::Resume);
   }
   taskAvailable_.notify_one();
}

void MediaPlaybackController::WorkerLoop(std::stop_token stopToken)
{
   // The blocking WinRT calls below must stay off the (STA) UI thread;
   // this worker joins the MTA instead.
   winrt::init_apartment();
   for (;;) {
      MediaTask task;
      {
         std::unique_lock lock(taskMutex_);
         if (!taskAvailable_.wait(lock, stopToken,
                                  [this] { return !tasks_.empty(); })) {
            break; // stop requested
         }
         task = tasks_.front();
         tasks_.pop_front();
      }
      if (task == MediaTask::Pause) {
         bool didPause = false;
         if (TryPauseCurrentSession(didPause)) {
            pausedByUs_ = didPause;
         }
      } else if (task == MediaTask::Resume) {
         if (pausedByUs_) {
            TryPlayCurrentSession();
         }
         pausedByUs_ = false;
      }
   }
   winrt::uninit_apartment();
}

bool MediaPlaybackController::TryPauseCurrentSession(bool &pausedMedia)
{
   auto &log = WMLog::GetInstance();
   pausedMedia = false;
   try {
      auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
      auto session = manager.GetCurrentSession();

      if (session) {
         auto playbackInfo = session.GetPlaybackInfo();
         auto status = playbackInfo.PlaybackStatus();

         if (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
            session.TryPauseAsync().get();
            log.LogInfo(L"MediaPlaybackController: Paused current media session.");
            pausedMedia = true;
         } else {
            log.LogDebug(L"MediaPlaybackController: Media is not playing, no action needed.");
         }
      }
   } catch (const hresult_error &e) {
      log.LogError(L"MediaPlaybackController: Failed to pause current media session. Error: %s",
                   e.message().c_str());
      return false;
   }
   return true;
}

bool MediaPlaybackController::TryPlayCurrentSession()
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
            log.LogInfo(L"MediaPlaybackController: Resumed current media session.");
         }
      }
   } catch (const hresult_error &e) {
      log.LogError(L"MediaPlaybackController: Failed to resume current media session. Error: %s",
                   e.message().c_str());
      return false;
   }
   return true;
}
