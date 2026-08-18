/*
 WinMute
           Copyright (c) 2011-2026 Alexander Steinhoefer

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the author nor the names of its contributors may
      be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// Pauses/resumes the current system media session (pause on mute, resume on
// unmute). The underlying WinRT calls block on cross-process RPC, so they run
// on a dedicated worker thread; the public methods only enqueue a task and
// return immediately.
class MediaPlaybackController {
   public:
    MediaPlaybackController();
    ~MediaPlaybackController() = default;  // jthread stops and joins the worker
    MediaPlaybackController(const MediaPlaybackController&) = delete;
    MediaPlaybackController& operator=(const MediaPlaybackController&) = delete;

    // Pauses the current media session if it is currently playing.
    void RequestPause();
    // Resumes the current media session, but only if a previous RequestPause
    // actually paused it.
    void RequestResume();

   private:
    enum class MediaTask { Pause, Resume };

    void WorkerLoop(std::stop_token stopToken);
    bool TryPauseCurrentSession(bool& pausedMedia);
    bool TryPlayCurrentSession();

    std::mutex taskMutex_;
    std::condition_variable_any taskAvailable_;
    std::deque<MediaTask> tasks_;
    bool pausedByUs_ = false;  // only accessed on the worker thread

    // Declared last: it is destroyed (stopped and joined) first, while all
    // other members are still alive.
    std::jthread worker_;
};
