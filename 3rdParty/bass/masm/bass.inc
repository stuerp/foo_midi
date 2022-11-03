; BASS 2.4 MASM include file
; Copyright (c) 1999-2013 Un4seen Developments Ltd.
;
; See the BASS.CHM file for more complete documentation

;  Differences:
;   BASS_SetEAXParameters [call] -> BASS_SetEAXParametersASM [macro]
;   BASS_INFO.size               -> BASS_INFO.ssize
;   BASS_INFO.eax                -> BASS_INFO.haseax
;   BASS_RECORDINFO.size         -> BASS_RECORDINFO.ssize
;   BASS_SAMPLE.length           -> BASS_SAMPLE.slength

BASSVERSION             equ 204h ; API version
BASSVERSIONTEXT         equ "2.4"

float TYPEDEF REAL4
double TYPEDEF REAL8

HMUSIC TYPEDEF DWORD            ; MOD music handle
HSAMPLE TYPEDEF DWORD           ; sample handle
HCHANNEL TYPEDEF DWORD          ; playing sample's channel handle
HSTREAM TYPEDEF DWORD           ; sample stream handle
HRECORD TYPEDEF DWORD           ; recording handle
HSYNC TYPEDEF DWORD             ; synchronizer handle
HDSP TYPEDEF DWORD              ; DSP handle
HFX TYPEDEF DWORD               ; DX8 effect handle
HPLUGIN TYPEDEF DWORD           ; Plugin handle

; Error codes returned by BASS_GetErrorCode()
BASS_OK                 equ 0   ; all is OK
BASS_ERROR_MEM          equ 1   ; memory error
BASS_ERROR_FILEOPEN     equ 2   ; can't open the file
BASS_ERROR_DRIVER       equ 3   ; can't find a free/valid driver
BASS_ERROR_BUFLOST      equ 4   ; the sample buffer was lost
BASS_ERROR_HANDLE       equ 5   ; invalid handle
BASS_ERROR_FORMAT       equ 6   ; unsupported sample format
BASS_ERROR_POSITION     equ 7   ; invalid position
BASS_ERROR_INIT         equ 8   ; BASS_Init has not been successfully called
BASS_ERROR_START        equ 9   ; BASS_Start has not been successfully called
BASS_ERROR_ALREADY      equ 14  ; already initialized/paused/whatever
BASS_ERROR_NOCHAN       equ 18  ; can't get a free channel
BASS_ERROR_ILLTYPE      equ 19  ; an illegal type was specified
BASS_ERROR_ILLPARAM     equ 20  ; an illegal parameter was specified
BASS_ERROR_NO3D         equ 21  ; no 3D support
BASS_ERROR_NOEAX        equ 22  ; no EAX support
BASS_ERROR_DEVICE       equ 23  ; illegal device number
BASS_ERROR_NOPLAY       equ 24  ; not playing
BASS_ERROR_FREQ         equ 25  ; illegal sample rate
BASS_ERROR_NOTFILE      equ 27  ; the stream is not a file stream
BASS_ERROR_NOHW         equ 29  ; no hardware voices available
BASS_ERROR_EMPTY        equ 31  ; the MOD music has no sequence data
BASS_ERROR_NONET        equ 32  ; no internet connection could be opened
BASS_ERROR_CREATE       equ 33  ; couldn't create the file
BASS_ERROR_NOFX         equ 34  ; effects are not enabled
BASS_ERROR_NOTAVAIL     equ 37  ; requested data is not available
BASS_ERROR_DECODE       equ 38  ; the channel is a "decoding channel"
BASS_ERROR_DX           equ 39  ; a sufficient DirectX version is not installed
BASS_ERROR_TIMEOUT      equ 40  ; connection timedout
BASS_ERROR_FILEFORM     equ 41  ; unsupported file format
BASS_ERROR_SPEAKER      equ 42  ; unavailable speaker
BASS_ERROR_VERSION      equ 43  ; invalid BASS version (used by add-ons)
BASS_ERROR_CODEC        equ 44  ; codec is not available/supported
BASS_ERROR_ENDED        equ 45  ; the channel/file has ended
BASS_ERROR_BUSY         equ 46  ; the device is busy
BASS_ERROR_UNKNOWN      equ -1  ; some other mystery problem

; BASS_SetConfig options
BASS_CONFIG_BUFFER      equ 0
BASS_CONFIG_UPDATEPERIOD equ 1
BASS_CONFIG_GVOL_SAMPLE equ 4
BASS_CONFIG_GVOL_STREAM equ 5
BASS_CONFIG_GVOL_MUSIC  equ 6
BASS_CONFIG_CURVE_VOL   equ 7
BASS_CONFIG_CURVE_PAN   equ 8
BASS_CONFIG_FLOATDSP    equ 9
BASS_CONFIG_3DALGORITHM equ 10
BASS_CONFIG_NET_TIMEOUT equ 11
BASS_CONFIG_NET_BUFFER  equ 12
BASS_CONFIG_PAUSE_NOPLAY equ 13
BASS_CONFIG_NET_PREBUF  equ 15
BASS_CONFIG_NET_PASSIVE equ 18
BASS_CONFIG_REC_BUFFER  equ 19
BASS_CONFIG_NET_PLAYLIST equ 21
BASS_CONFIG_MUSIC_VIRTUAL equ 22
BASS_CONFIG_VERIFY      equ 23
BASS_CONFIG_UPDATETHREADS equ 24
BASS_CONFIG_DEV_BUFFER equ 27
BASS_CONFIG_VISTA_TRUEPOS equ 30
BASS_CONFIG_DEV_DEFAULT equ 36
BASS_CONFIG_NET_READTIMEOUT equ 37
BASS_CONFIG_VISTA_SPEAKERS equ 38
BASS_CONFIG_HANDLES     equ 41
BASS_CONFIG_UNICODE     equ 42
BASS_CONFIG_SRC         equ 43
BASS_CONFIG_SRC_SAMPLE  equ 44
BASS_CONFIG_ASYNCFILE_BUFFER equ 45
BASS_CONFIG_OGG_PRESCAN equ 47

; BASS_SetConfigPtr options
BASS_CONFIG_NET_AGENT   equ 16
BASS_CONFIG_NET_PROXY   equ 17

; BASS_Init flags
BASS_DEVICE_8BITS       equ 1   ; 8 bit resolution, else 16 bit
BASS_DEVICE_MONO        equ 2   ; mono, else stereo
BASS_DEVICE_3D          equ 4   ; enable 3D functionality
BASS_DEVICE_LATENCY     equ 100h; calculate device latency (BASS_INFO struct)
BASS_DEVICE_CPSPEAKERS  equ 400h; detect speakers via Windows control panel
BASS_DEVICE_SPEAKERS    equ 800h; force enabling of speaker assignment
BASS_DEVICE_NOSPEAKER   equ 1000h; ignore speaker arrangement
BASS_DEVICE_DMIX        equ 2000h; use ALSA "dmix" plugin
BASS_DEVICE_FREQ		equ 4000h; set device sample rate

; DirectSound interfaces (for use with BASS_GetDSoundObject)
BASS_OBJECT_DS          equ 1   ; IDirectSound
BASS_OBJECT_DS3DL       equ 2   ; IDirectSound3DListener

; Device info structure
BASS_DEVICEINFO struct
        name     DWORD   ?       ; description
        driver   DWORD   ?       ; driver
        flags    DWORD   ?
BASS_DEVICEINFO ends

; BASS_DEVICEINFO flags
BASS_DEVICE_ENABLED     equ 1
BASS_DEVICE_DEFAULT     equ 2
BASS_DEVICE_INIT        equ 4

BASS_INFO struct
        flags    DWORD   ?       ; device capabilities (DSCAPS_xxx flags)
        hwsize   DWORD   ?       ; size of total device hardware memory
        hwfree   DWORD   ?       ; size of free device hardware memory
        freesam  DWORD   ?       ; number of free sample slots in the hardware
        free3d   DWORD   ?       ; number of free 3D sample slots in the hardware
        minrate  DWORD   ?       ; min sample rate supported by the hardware
        maxrate  DWORD   ?       ; max sample rate supported by the hardware
        haseax   DWORD   ?       ; device supports EAX? (always FALSE if BASS_DEVICE_3D was not used)
		minbuf   DWORD   ?       ; recommended minimum buffer length in ms (requires BASS_DEVICE_LATENCY)
        dsver    DWORD   ?       ; DirectSound version
        latency  DWORD   ?       ; delay (in ms) before start of playback (requires BASS_DEVICE_LATENCY)
        initflags DWORD  ?       ; "flags" parameter of BASS_Init call
        speakers DWORD   ?       ; number of speakers available
        freq     DWORD   ?       ; current output rate
BASS_INFO ends

; BASS_INFO flags (from DSOUND.H)
DSCAPS_CONTINUOUSRATE   equ     00000010h      ; supports all sample rates between min/maxrate
DSCAPS_EMULDRIVER       equ     00000020h      ; device does NOT have hardware DirectSound support
DSCAPS_CERTIFIED        equ     00000040h      ; device driver has been certified by Microsoft
DSCAPS_SECONDARYMONO    equ     00000100h      ; mono
DSCAPS_SECONDARYSTEREO  equ     00000200h      ; stereo
DSCAPS_SECONDARY8BIT    equ     00000400h      ; 8 bit
DSCAPS_SECONDARY16BIT   equ     00000800h      ; 16 bit

; Recording device info structure
BASS_RECORDINFO struct
        flags    DWORD   ?      ; device capabilities (DSCCAPS_xxx flags)
        formats  DWORD   ?      ; supported standard formats (WAVE_FORMAT_xxx flags)
        inputs   DWORD   ?      ; number of inputs
        singelin DWORD   ?      ; TRUE = only 1 input can be set at a time
        freq     DWORD   ?      ; current input rate
BASS_RECORDINFO ends

; BASS_RECORDINFO flags (from DSOUND.H)
DSCCAPS_EMULDRIVER      equ     DSCAPS_EMULDRIVER ; device does NOT have hardware DirectSound recording support
DSCCAPS_CERTIFIED       equ     DSCAPS_CERTIFIED  ; device driver has been certified by Microsoft

; Sample info structure & flags
BASS_SAMPLE struct
        freq     DWORD   ?      ; default playback rate
        volume   float   ?      ; default volume (0-1)
        pan      float   ?      ; default pan (-1=left, 0=middle, 1=right)
        flags    DWORD   ?      ; BASS_SAMPLE_xxx flags
        slength  DWORD   ?      ; length (in samples, not bytes)
        max      DWORD   ?      ; maximum simultaneous playbacks
		origres  DWORD   ?      ; original resolution bits
        chans    DWORD   ?      ; number of channels
        mingap   DWORD   ?      ; minimum gap (ms) between creating channels
        mode3d   DWORD   ?      ; BASS_3DMODE_xxx mode
        mindist  float   ?      ; minimum distance
        maxdist  float   ?      ; maximum distance
        iangle   DWORD   ?      ; angle of inside projection cone
        oangle   DWORD   ?      ; angle of outside projection cone
        outvol   float   ?      ; delta-volume outside the projection cone
        vam      DWORD   ?      ; voice allocation/management flags (BASS_VAM_xxx)
        priority DWORD   ?      ; priority (0=lowest, ffffffffh=highest)
BASS_SAMPLE ends

BASS_SAMPLE_8BITS       equ    1        ; 8 bit
BASS_SAMPLE_FLOAT       equ    256      ; 32-bit floating-point
BASS_SAMPLE_MONO        equ    2        ; mono
BASS_SAMPLE_LOOP        equ    4        ; looped
BASS_SAMPLE_3D          equ    8        ; 3D functionality
BASS_SAMPLE_SOFTWARE    equ    16       ; not using hardware mixing
BASS_SAMPLE_MUTEMAX     equ    32       ; mute at max distance (3D only)
BASS_SAMPLE_VAM         equ    64       ; DX7 voice allocation & management
BASS_SAMPLE_FX          equ    128      ; old implementation of DX8 effects
BASS_SAMPLE_OVER_VOL    equ    10000h   ; override lowest volume
BASS_SAMPLE_OVER_POS    equ    20000h   ; override longest playing
BASS_SAMPLE_OVER_DIST   equ    30000h   ; override furthest from listener (3D only)

BASS_STREAM_PRESCAN		equ    20000h   ; enable pin-point seeking/length (MP3/MP2/MP1)
BASS_MP3_SETPOS			equ    BASS_STREAM_PRESCAN
BASS_STREAM_AUTOFREE	equ    40000h   ; automatically free the stream when it stop/ends
BASS_STREAM_RESTRATE	equ    80000h   ; restrict the download rate of internet file streams
BASS_STREAM_BLOCK		equ    100000h  ; download/play internet file stream in small blocks
BASS_STREAM_DECODE		equ    200000h  ; don't play the stream, only decode (BASS_ChannelGetData)
BASS_STREAM_STATUS		equ    800000h  ; give server status info (HTTP/ICY tags) in DOWNLOADPROC

BASS_MUSIC_FLOAT		equ    BASS_SAMPLE_FLOAT
BASS_MUSIC_MONO			equ    BASS_SAMPLE_MONO
BASS_MUSIC_LOOP			equ    BASS_SAMPLE_LOOP
BASS_MUSIC_3D			equ    BASS_SAMPLE_3D
BASS_MUSIC_FX			equ    BASS_SAMPLE_FX
BASS_MUSIC_AUTOFREE		equ    BASS_STREAM_AUTOFREE
BASS_MUSIC_DECODE		equ    BASS_STREAM_DECODE
BASS_MUSIC_PRESCAN		equ    BASS_STREAM_PRESCAN ; calculate playback length
BASS_MUSIC_CALCLEN		equ    BASS_MUSIC_PRESCAN
BASS_MUSIC_RAMP			equ    200h     ; normal ramping
BASS_MUSIC_RAMPS		equ    400h     ; sensitive ramping
BASS_MUSIC_SURROUND		equ    800h     ; surround sound
BASS_MUSIC_SURROUND2	equ    1000h    ; surround sound (mode 2)
BASS_MUSIC_FT2MOD		equ    2000h    ; play .MOD as FastTracker 2 does
BASS_MUSIC_PT1MOD		equ    4000h    ; play .MOD as ProTracker 1 does
BASS_MUSIC_NONINTER		equ    10000h   ; non-interpolated sample mixing
BASS_MUSIC_SINCINTER    equ    800000h  ; sinc interpolated sample mixing
BASS_MUSIC_POSRESET		equ    8000h    ; stop all notes when moving position
BASS_MUSIC_POSRESETEX	equ    400000h  ; stop all notes and reset bmp/etc when moving position
BASS_MUSIC_STOPBACK		equ    80000h   ; stop the music on a backwards jump effect
BASS_MUSIC_NOSAMPLE		equ    100000h  ; don't load the samples

; Speaker assignment flags
BASS_SPEAKER_FRONT      equ 1000000h    ; front speakers
BASS_SPEAKER_REAR       equ 2000000h    ; rear/side speakers
BASS_SPEAKER_CENLFE     equ 3000000h    ; center & LFE speakers (5.1)
BASS_SPEAKER_REAR2      equ 4000000h    ; rear center speakers (7.1)
BASS_SPEAKER_N MACRO n:REQ  ; n'th pair of speakers (max 15)
	EXITM <n SHL 24>
ENDM
BASS_SPEAKER_LEFT       equ 10000000h   ; modifier: left
BASS_SPEAKER_RIGHT      equ 20000000h   ; modifier: right
BASS_SPEAKER_FRONTLEFT  equ BASS_SPEAKER_FRONT or BASS_SPEAKER_LEFT
BASS_SPEAKER_FRONTRIGHT equ BASS_SPEAKER_FRONT or BASS_SPEAKER_RIGHT
BASS_SPEAKER_REARLEFT   equ BASS_SPEAKER_REAR or BASS_SPEAKER_LEFT
BASS_SPEAKER_REARRIGHT  equ BASS_SPEAKER_REAR or BASS_SPEAKER_RIGHT
BASS_SPEAKER_CENTER     equ BASS_SPEAKER_CENLFE or BASS_SPEAKER_LEFT
BASS_SPEAKER_LFE        equ BASS_SPEAKER_CENLFE or BASS_SPEAKER_RIGHT
BASS_SPEAKER_REAR2LEFT  equ BASS_SPEAKER_REAR2 or BASS_SPEAKER_LEFT
BASS_SPEAKER_REAR2RIGHT equ BASS_SPEAKER_REAR2 or BASS_SPEAKER_RIGHT

BASS_ASYNCFILE          equ 40000000h
BASS_UNICODE            equ 80000000h

BASS_RECORD_PAUSE       equ 8000h ; start recording paused

; DX7 voice allocation & management flags
BASS_VAM_HARDWARE		equ 1
BASS_VAM_SOFTWARE		equ 2
BASS_VAM_TERM_TIME		equ 4
BASS_VAM_TERM_DIST		equ 8
BASS_VAM_TERM_PRIO		equ 16

; Channel info structure
BASS_CHANNELINFO struct
        freq     DWORD   ?      ; default playback rate
        chans    DWORD   ?      ; channels
        flags    DWORD   ?      ; BASS_SAMPLE/STREAM/MUSIC/SPEAKER flags
        ctype    DWORD   ?      ; type of channel
        origres  DWORD   ?      ; original resolution
        plugin   DWORD   ?      ; plugin
        sample   DWORD   ?      ; sample
        filename DWORD   ?      ; filename
BASS_CHANNELINFO ends

; BASS_CHANNELINFO types
BASS_CTYPE_SAMPLE       equ 1
BASS_CTYPE_RECORD       equ 2
BASS_CTYPE_STREAM       equ 10000h
BASS_CTYPE_STREAM_OGG   equ 10002h
BASS_CTYPE_STREAM_MP1   equ 10003h
BASS_CTYPE_STREAM_MP2   equ 10004h
BASS_CTYPE_STREAM_MP3   equ 10005h
BASS_CTYPE_STREAM_AIFF	equ 10006h
BASS_CTYPE_STREAM_WAV	equ 40000h ; WAVE flag, LOWORD=codec
BASS_CTYPE_STREAM_WAV_PCM equ 50001h
BASS_CTYPE_STREAM_WAV_FLOAT equ 50003h
BASS_CTYPE_MUSIC_MOD    equ 20000h
BASS_CTYPE_MUSIC_MTM    equ 20001h
BASS_CTYPE_MUSIC_S3M    equ 20002h
BASS_CTYPE_MUSIC_XM     equ 20003h
BASS_CTYPE_MUSIC_IT     equ 20004h
BASS_CTYPE_MUSIC_MO3    equ 00100h ; MO3 flag

BASS_PLUGINFORM struct
        ctype    DWORD   ?      ; channel type
        name     DWORD   ?      ; format description
        exts     DWORD   ?      ; file extension filter (*.ext1;*.ext2;etc...)
BASS_PLUGINFORM ends

BASS_PLUGININFO struct
        version  DWORD   ?      ; version (same form as BASS_GetVersion)
        formatc  DWORD   ?      ; number of formats
        formats  DWORD   ?      ; the array of formats
BASS_PLUGININFO ends

; 3D vector (for 3D positions/velocities/orientations)
BASS_3DVECTOR struct
        x float ?       ; +=right, -=left
        y float ?       ; +=up, -=down
        z float ?       ; +=front, -=behind
BASS_3DVECTOR ends

BASS_3DMODE_NORMAL      equ    0
BASS_3DMODE_RELATIVE    equ    1
BASS_3DMODE_OFF         equ    2

; EAX environments, use with BASS_SetEAXParameters
EAX_PRESET_GENERIC         equ 0
EAX_PRESET_PADDEDCELL      equ 1
EAX_PRESET_ROOM            equ 2
EAX_PRESET_BATHROOM        equ 3
EAX_PRESET_LIVINGROOM      equ 4
EAX_PRESET_STONEROOM       equ 5
EAX_PRESET_AUDITORIUM      equ 6
EAX_PRESET_CONCERTHALL     equ 7
EAX_PRESET_CAVE            equ 8
EAX_PRESET_ARENA           equ 9
EAX_PRESET_HANGAR          equ 10
EAX_PRESET_CARPETEDHALLWAY equ 11
EAX_PRESET_HALLWAY         equ 12
EAX_PRESET_STONECORRIDOR   equ 13
EAX_PRESET_ALLEY           equ 14
EAX_PRESET_FOREST          equ 15
EAX_PRESET_CITY            equ 16
EAX_PRESET_MOUNTAINS       equ 17
EAX_PRESET_QUARRY          equ 18
EAX_PRESET_PLAIN           equ 19
EAX_PRESET_PARKINGLOT      equ 20
EAX_PRESET_SEWERPIPE       equ 21
EAX_PRESET_UNDERWATER      equ 22
EAX_PRESET_DRUGGED         equ 23
EAX_PRESET_DIZZY           equ 24
EAX_PRESET_PSYCHOTIC       equ 25
EAX_PRESET_COUNT           equ 26          ; total number of environments

BASS_SetEAXParametersASM MACRO EAX_PRESET:REQ
   IF EAX_PRESET EQ EAX_PRESET_GENERIC  
    invoke BASS_SetEAXParameters, EAX_PRESET_GENERIC,3F000000h,3FBF1AA0h,3F000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_PADDEDCELL
    invoke BASS_SetEAXParameters, EAX_PRESET,3E800000h,3DCCCCCDh,00000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_ROOM     
    invoke BASS_SetEAXParameters, EAX_PRESET_ROOM,3ED58106h,3ECCCCCDh,3F2A7EFAh
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_BATHROOM
    invoke BASS_SetEAXParameters, EAX_PRESET_BATHROOM,3F272B02h,3FBFDF3Bh,3E29FBE7h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_LIVINGROOM
    invoke BASS_SetEAXParameters, EAX_PRESET_LIVINGROOM,3E54FDF4h,3EF4BC6Ah,00000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_STONEROOM
    invoke BASS_SetEAXParameters, EAX_PRESET_STONEROOM,3F000000h,4013C6A8h,3F6353F8h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_AUDITORIUM
    invoke BASS_SetEAXParameters, EAX_PRESET_AUDITORIUM,3ECE5604h,4088ED91h,3F000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_CONCERTHALL
    invoke BASS_SetEAXParameters, EAX_PRESET_CONCERTHALL,3F000000h,407D8106h,3F000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_CAVE
    invoke BASS_SetEAXParameters, EAX_PRESET_CAVE,3F000000h,4038B439h,3FA6E979h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_ARENA
    invoke BASS_SetEAXParameters, EAX_PRESET_ARENA,3EB8D4FEh,40E91687h,3EA9FBE7h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_HANGAR
    invoke BASS_SetEAXParameters, EAX_PRESET_HANGAR,3F000000h,41200000h,3E99999Ah
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_CARPETEDHALLWAY
    invoke BASS_SetEAXParameters, EAX_PRESET_CARPETEDHALLWAY,3E1CAC08h,3E849BA6h,40000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_HALLWAY
    invoke BASS_SetEAXParameters, EAX_PRESET_HALLWAY,3EB8D4FEh,3FBF1AA0h,00000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_STONECORRIDOR
    invoke BASS_SetEAXParameters, EAX_PRESET_STONECORRIDOR,3EE353F8h,402C9BA6h,3F2353F8h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_ALLEY
    invoke BASS_SetEAXParameters, EAX_PRESET_ALLEY,3E800000h,3FE04189h,3F46A7F0h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_FOREST
    invoke BASS_SetEAXParameters, EAX_PRESET_FOREST,3DE353F8h,404947AEh,3EF1A9FCh
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_CITY
    invoke BASS_SetEAXParameters, EAX_PRESET_CITY,3DE353F8h,40311687h,3E656042h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_MOUNTAINS
    invoke BASS_SetEAXParameters, EAX_PRESET_MOUNTAINS,3E46A7F0h,40FAE979h,3EF1A9FCh
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_QUARRY
    invoke BASS_SetEAXParameters, EAX_PRESET_QUARRY,3F800000h,3FBFDF3Bh,3F000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_PLAIN
    invoke BASS_SetEAXParameters, EAX_PRESET_PLAIN,3DC6A7F0h,40311687h,3E656042h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_PARKINGLOT
    invoke BASS_SetEAXParameters, EAX_PRESET_PARKINGLOT,3E54FDF4h,3FD374BCh,3FC00000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_SEWERPIPE
    invoke BASS_SetEAXParameters, EAX_PRESET_SEWERPIPE,3F26E979h,4038B439h,3E800000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_UNDERWATER
    invoke BASS_SetEAXParameters, EAX_PRESET_UNDERWATER,3F800000h,3FBFDF3Bh,00000000h
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_DRUGGED
    invoke BASS_SetEAXParameters, EAX_PRESET_DRUGGED,3F600000h,410645A2h,3FB1A9FCh
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_DIZZY
    invoke BASS_SetEAXParameters, EAX_PRESET_DIZZY,3E0E5604h,4189DF3Bh,3F2A7EFAh
   ENDIF
   IF EAX_PRESET EQ EAX_PRESET_PSYCHOTIC
    invoke BASS_SetEAXParameters, EAX_PRESET_PSYCHOTIC,3EF8D4FEh,40F20419h,3F4E5604h
   ENDIF
ENDM

BASS_STREAMPROC_END     equ 80000000h	; end of user stream flag

; special STREAMPROCs
STREAMPROC_DUMMY        equ 0           ; "dummy" stream
STREAMPROC_PUSH         equ -1          ; push stream

; BASS_StreamCreateFileUser file systems
STREAMFILE_NOBUFFER		equ 0
STREAMFILE_BUFFER		equ 1
STREAMFILE_BUFFERPUSH	equ 2

BASS_FILEPROCS struct
        close   DWORD   ?
        length_ DWORD   ?
        read    DWORD   ?
        seek    DWORD   ?
BASS_FILEPROCS ends

; BASS_StreamPutFileData options
BASS_FILEDATA_END       equ    0        ; end & close the file

; BASS_StreamGetFilePosition modes
BASS_FILEPOS_CURRENT    equ    0
BASS_FILEPOS_DECODE     equ BASS_FILEPOS_CURRENT
BASS_FILEPOS_DOWNLOAD   equ    1
BASS_FILEPOS_END        equ    2
BASS_FILEPOS_START      equ    3
BASS_FILEPOS_CONNECTED  equ    4
BASS_FILEPOS_BUFFER     equ    5
BASS_FILEPOS_SOCKET     equ    6

; BASS_ChannelSetSync types
BASS_SYNC_POS           equ 0
BASS_SYNC_END           equ 2
BASS_SYNC_META          equ 4
BASS_SYNC_SLIDE         equ 5
BASS_SYNC_STALL         equ 6
BASS_SYNC_DOWNLOAD      equ 7
BASS_SYNC_FREE          equ 8
BASS_SYNC_SETPOS        equ 11
BASS_SYNC_MUSICPOS      equ 10
BASS_SYNC_MUSICINST     equ 1
BASS_SYNC_MUSICFX       equ 3
BASS_SYNC_OGG_CHANGE    equ 12
BASS_SYNC_MIXTIME       equ 40000000h   ; FLAG: sync at mixtime, else at playtime
BASS_SYNC_ONETIME       equ 80000000h   ; FLAG: sync only once, else continuously

; BASS_ChannelIsActive return values
BASS_ACTIVE_STOPPED     equ 0
BASS_ACTIVE_PLAYING     equ 1
BASS_ACTIVE_STALLED     equ 2
BASS_ACTIVE_PAUSED      equ 3

; Channel attributes
BASS_ATTRIB_FREQ        equ 1
BASS_ATTRIB_VOL         equ 2
BASS_ATTRIB_PAN         equ 3
BASS_ATTRIB_EAXMIX      equ 4
BASS_ATTRIB_NOBUFFER    equ 5
BASS_ATTRIB_CPU         equ 7
BASS_ATTRIB_SRC         equ 8
BASS_ATTRIB_MUSIC_AMPLIFY equ 100h
BASS_ATTRIB_MUSIC_PANSEP equ 101h
BASS_ATTRIB_MUSIC_PSCALER equ 102h
BASS_ATTRIB_MUSIC_BPM   equ 103h
BASS_ATTRIB_MUSIC_SPEED equ 104h
BASS_ATTRIB_MUSIC_VOL_GLOBAL equ 105h
BASS_ATTRIB_MUSIC_VOL_CHAN equ 200h ; + channel #
BASS_ATTRIB_MUSIC_VOL_INST equ 300h ; + instrument #

; BASS_ChannelGetData flags
BASS_DATA_AVAILABLE     equ 0           ; query how much data is buffered
BASS_DATA_FLOAT         equ 40000000h   ; flag: return floating-point sample data
BASS_DATA_FFT256        equ 80000000h   ; 256 sample FFT
BASS_DATA_FFT512        equ 80000001h   ; 512 FFT
BASS_DATA_FFT1024       equ 80000002h   ; 1024 FFT
BASS_DATA_FFT2048       equ 80000003h   ; 2048 FFT
BASS_DATA_FFT4096       equ 80000004h   ; 4096 FFT
BASS_DATA_FFT8192       equ 80000005h   ; 8192 FFT
BASS_DATA_FFT16384      equ 80000006h   ; 16384 FFT
BASS_DATA_FFT_INDIVIDUAL equ 10h        ; FFT flag: FFT for each channel, else all combined
BASS_DATA_FFT_NOWINDOW  equ 20h         ; FFT flag: no Hanning window
BASS_DATA_FFT_REMOVEDC  equ 40h         ; FFT flag: pre-remove DC bias
BASS_DATA_FFT_COMPLEX   equ 80h         ; FFT flag: return complex data

; BASS_StreamGetTags types : what's returned
BASS_TAG_ID3            equ 0            ; ID3v1 tags : TAG_ID3 structure
BASS_TAG_ID3V2          equ 1            ; ID3v2 tags : variable length block
BASS_TAG_OGG            equ 2            ; OGG comments : array of null-terminated UTF-8 strings
BASS_TAG_HTTP           equ 3            ; HTTP headers : array of null-terminated ANSI strings
BASS_TAG_ICY            equ 4            ; ICY headers : array of null-terminated ANSI strings
BASS_TAG_META           equ 5            ; ICY metadata : ANSI string
BASS_TAG_APE            equ 6            ; APEv2 tags : series of null-terminated UTF-8 strings
BASS_TAG_VENDOR         equ 9            ; OGG encoder : UTF-8 string
BASS_TAG_LYRICS3        equ 10           ; Lyric3v2 tag : ASCII string
BASS_TAG_CA_CODEC       equ 11           ; CoreAudio codec info : TAG_CA_CODEC structure
BASS_TAG_MF             equ 13           ; Media Foundation tags : series of null-terminated UTF-8 strings
BASS_TAG_WAVEFORMAT     equ 14           ; WAVE format : WAVEFORMATEEX structure
BASS_TAG_RIFF_INFO      equ 100h         ; RIFF "INFO" tags : array of null-terminated ANSI strings
BASS_TAG_RIFF_BEXT      equ 101h         ; RIFF/BWF "bext" tags : TAG_BEXT structure
BASS_TAG_RIFF_CART      equ 102h         ; RIFF/BWF "cart" tags : TAG_CART structure
BASS_TAG_RIFF_DISP      equ 103h         ; RIFF "DISP" text tag : ANSI string
BASS_TAG_APE_BINARY     equ 1000h        ; + index #, binary APEv2 tag : TAG_APE_BINARY structure
BASS_TAG_MUSIC_NAME     equ 10000h       ; MOD music name : ANSI string
BASS_TAG_MUSIC_MESSAGE  equ 10001h       ; MOD message : ANSI string
BASS_TAG_MUSIC_ORDERS   equ 10002h       ; MOD order list : BYTE array of pattern numbers
BASS_TAG_MUSIC_INST     equ 10100h       ; + instrument #, MOD instrument name : ANSI string
BASS_TAG_MUSIC_SAMPLE   equ 10300h       ; + sample #, MOD sample name : ANSI string

; BASS_ChannelGetLength/GetPosition/SetPosition modes
BASS_POS_BYTE           equ 0       ; byte position
BASS_POS_MUSIC_ORDER    equ 1       ; order.row position, MAKELONG(order,row)
BASS_POS_OGG            equ 3       ; OGG bitstream number
BASS_POS_DECODE         equ 10000000h ; flag: get the decoding (not playing) position
BASS_POS_DECODETO       equ 20000000h ; flag: decode to the position instead of seeking

; BASS_RecordSetInput flags
BASS_INPUT_OFF          equ 10000h
BASS_INPUT_ON           equ 20000h

BASS_INPUT_TYPE_MASK		equ ff000000h
BASS_INPUT_TYPE_UNDEF		equ 00000000h
BASS_INPUT_TYPE_DIGITAL		equ 01000000h
BASS_INPUT_TYPE_LINE		equ 02000000h
BASS_INPUT_TYPE_MIC			equ 03000000h
BASS_INPUT_TYPE_SYNTH		equ 04000000h
BASS_INPUT_TYPE_CD			equ 05000000h
BASS_INPUT_TYPE_PHONE		equ 06000000h
BASS_INPUT_TYPE_SPEAKER		equ 07000000h
BASS_INPUT_TYPE_WAVE		equ 08000000h
BASS_INPUT_TYPE_AUX			equ 09000000h
BASS_INPUT_TYPE_ANALOG		equ 0a000000h

; DX8 effect types, use with BASS_ChannelSetFX
BASS_FX_DX8_CHORUS          equ 0
BASS_FX_DX8_COMPRESSOR      equ 1
BASS_FX_DX8_DISTORTION      equ 2
BASS_FX_DX8_ECHO            equ 3
BASS_FX_DX8_FLANGER         equ 4
BASS_FX_DX8_GARGLE          equ 5
BASS_FX_DX8_I3DL2REVERB     equ 6
BASS_FX_DX8_PARAMEQ         equ 7
BASS_FX_DX8_REVERB          equ 8

BASS_DX8_CHORUS struct
  fWetDryMix float ?
  fDepth     float ?
  fFeedback  float ?
  fFrequency float ?
  lWaveform  DWORD ?     ; 0=triangle, 1=sine
  fDelay     float ?
  lPhase     DWORD ?     ; BASS_FX_PHASE_xxx
BASS_DX8_CHORUS ends

BASS_DX8_COMPRESSOR struct
  fGain      float ?
  fAttack    float ?
  fRelease   float ?
  fThreshold float ?
  fRatio     float ?
  fPredelay  float ?
BASS_DX8_COMPRESSOR ends

BASS_DX8_DISTORTION struct
  fGain                   float ?
  fEdge                   float ?
  fPostEQCenterFrequency  float ?
  fPostEQBandwidth        float ?
  fPreLowpassCutoff       float ?
BASS_DX8_DISTORTION ends

BASS_DX8_ECHO struct
  fWetDryMix  float ?
  fFeedback   float ?
  fLeftDelay  float ?
  fRightDelay float ?
  lPanDelay   BYTE  ?
BASS_DX8_ECHO ends

BASS_DX8_FLANGER struct
  fWetDryMix  float ?
  fDepth      float ? 
  fFeedback   float ?
  fFrequency  float ?
  lWaveform   DWORD ?     ; 0=triangle, 1=sine
  fDelay      float ?
  lPhase      DWORD ?     ; BASS_DX8_PHASE_xxx
BASS_DX8_FLANGER ends

BASS_DX8_GARGLE struct
 dwRateHz     DWORD ?     ; Rate of modulation in hz
 dwWaveShape  DWORD ?     ; 0=triangle, 1=square
BASS_DX8_GARGLE ends

BASS_DX8_PARAMEQ struct
  fCenter    float ?
  fBandwidth float ?
  fGain      float ?
BASS_DX8_PARAMEQ ends

BASS_DX8_REVERB struct 
  fInGain          float ?  
  fReverbMix       float ?  
  fReverbTime      float ?    
  fHighFreqRTRatio float ?
BASS_DX8_REVERB ends

BASS_DX8_I3DL2REVERB struct 
  lRoom                  BYTE  ?         ; [-10000, 0] default: -1000 mB
  lRoomHF                BYTE  ?         ; [-10000, 0] default: 0 mB
  flRoomRolloffFactor    float ?         ; [0.0, 10.0] default: 0.0
  flDecayTime            float ?         ; [0.1, 20.0] default: 1.49s
  flDecayHFRatio         float ?         ; [0.1, 2.0] default: 0.83
  lReflections           BYTE  ?         ; [-10000, 1000] default: -2602 mB
  flReflectionsDelay     float ?         ; [0.0, 0.3] default: 0.007 s
  lReverb                BYTE  ?         ; [-10000, 2000] default: 200 mB
  flReverbDelay          float ?         ; [0.0, 0.1] default: 0.011 s
  flDiffusion            float ?         ; [0.0, 100.0] default: 100.0 %
  flDensity              float ?         ; [0.0, 100.0] default: 100.0 %
  flHFReference          float ?         ; [20.0, 20000.0] default: 5000.0 Hz
BASS_DX8_I3DL2REVERB ends

BASS_DX8_PHASE_NEG_180   equ 0
BASS_DX8_PHASE_NEG_90    equ 1
BASS_DX8_PHASE_ZERO      equ 2
BASS_DX8_PHASE_90        equ 3
BASS_DX8_PHASE_180       equ 4

; prototypes

BASS_SetConfig                  PROTO :DWORD,:DWORD
BASS_GetConfig                  PROTO :DWORD
BASS_SetConfigPtr               PROTO :DWORD,:DWORD
BASS_GetConfigPtr               PROTO :DWORD
BASS_GetVersion                 PROTO
BASS_ErrorGetCode               PROTO
BASS_GetDeviceInfo              PROTO :DWORD,:DWORD
BASS_Init                       PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_SetDevice                  PROTO :DWORD
BASS_GetDevice                  PROTO
BASS_Free                       PROTO
BASS_GetDSoundObject            PROTO :DWORD
BASS_GetInfo                    PROTO :DWORD
BASS_Update                     PROTO :DWORD
BASS_GetCPU                     PROTO
BASS_Start                      PROTO
BASS_Stop                       PROTO
BASS_Pause                      PROTO
BASS_SetVolume                  PROTO :DWORD
BASS_GetVolume                  PROTO

BASS_PluginLoad                 PROTO :DWORD,:DWORD
BASS_PluginFree                 PROTO :DWORD
BASS_PluginGetInfo              PROTO :DWORD

BASS_Set3DFactors               PROTO :DWORD,:DWORD,:DWORD
BASS_Get3DFactors               PROTO :DWORD,:DWORD,:DWORD
BASS_Set3DPosition              PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_Get3DPosition              PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_Apply3D                    PROTO
BASS_SetEAXParameters           PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_GetEAXParameters           PROTO :DWORD,:DWORD,:DWORD,:DWORD

BASS_MusicLoad                  PROTO :DWORD,:DWORD,:QWORD,:DWORD,:DWORD,:DWORD
BASS_MusicFree                  PROTO :DWORD

BASS_SampleLoad                 PROTO :DWORD,:DWORD,:QWORD,:DWORD,:DWORD,:DWORD
BASS_SampleCreate               PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_SampleFree                 PROTO :DWORD
BASS_SampleSetData              PROTO :DWORD,:DWORD
BASS_SampleGetData              PROTO :DWORD,:DWORD
BASS_SampleGetInfo              PROTO :DWORD,:DWORD
BASS_SampleSetInfo              PROTO :DWORD,:DWORD
BASS_SampleGetChannel           PROTO :DWORD,:DWORD
BASS_SampleGetChannels          PROTO :DWORD,:DWORD
BASS_SampleStop                 PROTO :DWORD

BASS_StreamCreate               PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_StreamCreateFile           PROTO :DWORD,:DWORD,:QWORD,:QWORD,:DWORD
BASS_StreamCreateURL            PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_StreamCreateFileUser       PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_StreamFree                 PROTO :DWORD
BASS_StreamGetFilePosition      PROTO :DWORD,:DWORD
BASS_StreamPutData              PROTO :DWORD,:DWORD,:DWORD
BASS_StreamPutFileData          PROTO :DWORD,:DWORD,:DWORD

BASS_RecordGetDeviceInfo        PROTO :DWORD,:DWORD
BASS_RecordInit                 PROTO :DWORD
BASS_RecordSetDevice            PROTO :DWORD
BASS_RecordGetDevice            PROTO
BASS_RecordFree                 PROTO
BASS_RecordGetInfo              PROTO :DWORD
BASS_RecordGetInputName         PROTO :DWORD
BASS_RecordSetInput             PROTO :DWORD,:DWORD,:DWORD
BASS_RecordGetInput             PROTO :DWORD,:DWORD
BASS_RecordStart                PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD

BASS_ChannelBytes2Seconds       PROTO :DWORD,:QWORD
BASS_ChannelSeconds2Bytes       PROTO :DWORD,:QWORD
BASS_ChannelGetDevice           PROTO :DWORD
BASS_ChannelSetDevice           PROTO :DWORD,:DWORD
BASS_ChannelIsActive            PROTO :DWORD
BASS_ChannelGetInfo             PROTO :DWORD,:DWORD
BASS_ChannelGetTags             PROTO :DWORD,:DWORD
BASS_ChannelFlags               PROTO :DWORD,:DWORD,:DWORD
BASS_ChannelUpdate              PROTO :DWORD,:DWORD
BASS_ChannelLock                PROTO :DWORD,:DWORD
BASS_ChannelPlay                PROTO :DWORD,:DWORD
BASS_ChannelStop                PROTO :DWORD
BASS_ChannelPause               PROTO :DWORD
BASS_ChannelSetAttribute        PROTO :DWORD,:DWORD,:DWORD
BASS_ChannelGetAttribute        PROTO :DWORD,:DWORD,:DWORD
BASS_ChannelSlideAttribute      PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelIsSliding           PROTO :DWORD,:DWORD
BASS_ChannelSet3DAttributes     PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelGet3DAttributes     PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelSet3DPosition       PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelGet3DPosition       PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelGetLength           PROTO :DWORD,:DWORD
BASS_ChannelSetPosition         PROTO :DWORD,:QWORD,:DWORD
BASS_ChannelGetPosition         PROTO :DWORD,:DWORD
BASS_ChannelGetLevel            PROTO :DWORD
BASS_ChannelGetData             PROTO :DWORD,:DWORD,:DWORD
BASS_ChannelSetSync             PROTO :DWORD,:DWORD,:QWORD,:DWORD,:DWORD
BASS_ChannelRemoveSync          PROTO :DWORD,:DWORD
BASS_ChannelSetDSP              PROTO :DWORD,:DWORD,:DWORD,:DWORD
BASS_ChannelRemoveDSP           PROTO :DWORD,:DWORD
BASS_ChannelSetLink             PROTO :DWORD,:DWORD
BASS_ChannelRemoveLink          PROTO :DWORD,:DWORD
BASS_ChannelSetFX               PROTO :DWORD,:DWORD,:DWORD
BASS_ChannelRemoveFX            PROTO :DWORD,:DWORD

BASS_FXSetParameters            PROTO :DWORD,:DWORD
BASS_FXGetParameters            PROTO :DWORD,:DWORD
BASS_FXReset                    PROTO :DWORD
