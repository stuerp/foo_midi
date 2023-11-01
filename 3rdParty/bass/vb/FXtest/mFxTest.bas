Attribute VB_Name = "mFxTest"
Option Explicit

Public fxchan As Long      ' output stream handle
Public fxchansync As Long  ' output stream FREE sync

Public Sub DeviceFreeSync(ByVal handle As Long, ByVal channel As Long, ByVal data As Long, ByVal user As Long)
    ' the device output stream has been freed due to format change, get a new one with new format
    If Not fxchan Then Exit Sub
    fxchan = BASS_StreamCreate(0, 0, 0, STREAMPROC_DEVICE, 0)
    fxchansync = BASS_ChannelSetSync(fxchan, BASS_SYNC_FREE, 0, AddressOf DeviceFreeSync, 0)
    Call frmFXtest.SetupFX
End Sub
