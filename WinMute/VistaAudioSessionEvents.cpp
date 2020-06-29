/*
 WinMute
           Copyright (c) 2020, Alexander Steinhoefer

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

#include "StdAfx.h"
#include "WinAudio.h"
#include "VistaAudioSessionEvents.h"


VistaAudioSessionEvents::VistaAudioSessionEvents(WinAudio* notifyParent) :
   ref_(1)
{
   notifyParent_ = notifyParent;
}

VistaAudioSessionEvents::~VistaAudioSessionEvents()
{
}

ULONG STDMETHODCALLTYPE VistaAudioSessionEvents::AddRef()
{
   return InterlockedIncrement(&ref_);
}

ULONG STDMETHODCALLTYPE VistaAudioSessionEvents::Release()
{
   ULONG ref = InterlockedDecrement(&ref_);
   if (ref == 0) {
      delete this;
   }
   return ref;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::QueryInterface(
   REFIID riid, VOID **ppvInterface)
{
   if (riid == IID_IUnknown) {
      AddRef();
      *ppvInterface = reinterpret_cast<IUnknown*>(this);
   } else if (riid == __uuidof(IAudioSessionEvents)) {
      AddRef();
      *ppvInterface = reinterpret_cast<IAudioSessionEvents*>(this);
   } else {
      *ppvInterface = nullptr;
      return E_NOINTERFACE;
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnDisplayNameChanged(
   LPCWSTR /*newDisplayName*/, LPCGUID /*eventContext*/)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnIconPathChanged(
   LPCWSTR /*newIconPath*/, LPCGUID /*eventContext*/)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnSimpleVolumeChanged(
   float /*newVolume*/, BOOL /*newMute*/, LPCGUID /*eventContext*/)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnChannelVolumeChanged(
   DWORD /*channelCount*/, float* /*newChannelVolumeArray[]*/,
   DWORD /*changedChannel*/, LPCGUID /*eventContext*/)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnGroupingParamChanged(
   LPCGUID /*newGroupingParam*/, LPCGUID /*eventContext*/)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnStateChanged(
   AudioSessionState /*newState*/)
{
   return S_OK;
}

/* See
 * http://blogs.msdn.com/b/larryosterman/archive/2007/10/31/what-happens-when-audio-rendering-fails.aspx
 * for detailed information about each of these entries */
HRESULT STDMETHODCALLTYPE VistaAudioSessionEvents::OnSessionDisconnected(
   AudioSessionDisconnectReason disconnectReason)
{

   switch (disconnectReason)  {
   case DisconnectReasonDeviceRemoval:
   case DisconnectReasonFormatChanged:
   case DisconnectReasonSessionDisconnected:
      notifyParent_->ShouldReInit();
      break;
   case DisconnectReasonServerShutdown:
      TaskDialog(nullptr,
                 nullptr,
                 PROGRAM_NAME,
                 _T("The audio service has been shut down. "),
                 _T("WinMute is not able to recover from that condition.\n")
                 _T("Please try restarting the program"),
                 TDCBF_OK_BUTTON,
                 TD_WARNING_ICON,
                 nullptr);
      break;
   case DisconnectReasonSessionLogoff:
      break;
   case DisconnectReasonExclusiveModeOverride:
      break;
   }
   return S_OK;
}
