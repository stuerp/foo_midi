VERSION 5.00
Begin VB.Form frmNetradio 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS internet radio example"
   ClientHeight    =   4440
   ClientLeft      =   4455
   ClientTop       =   1185
   ClientWidth     =   4365
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4440
   ScaleWidth      =   4365
   ShowInTaskbar   =   0   'False
   Begin VB.Timer tmrStall 
      Left            =   3720
      Top             =   1920
   End
   Begin VB.Frame fra0 
      Caption         =   "Presets"
      Height          =   1080
      Left            =   112
      TabIndex        =   0
      Top             =   48
      Width           =   4135
      Begin VB.CommandButton cmdPset 
         Caption         =   "1"
         Height          =   360
         Index           =   0
         Left            =   1229
         TabIndex        =   3
         Top             =   216
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "1"
         Height          =   360
         Index           =   1
         Left            =   1229
         TabIndex        =   4
         Top             =   600
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "2"
         Height          =   360
         Index           =   2
         Left            =   1810
         TabIndex        =   5
         Top             =   216
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "2"
         Height          =   360
         Index           =   3
         Left            =   1810
         TabIndex        =   6
         Top             =   600
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "3"
         Height          =   360
         Index           =   4
         Left            =   2391
         TabIndex        =   7
         Top             =   216
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "3"
         Height          =   360
         Index           =   5
         Left            =   2391
         TabIndex        =   8
         Top             =   600
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "4"
         Height          =   360
         Index           =   6
         Left            =   2972
         TabIndex        =   9
         Top             =   216
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "4"
         Height          =   360
         Index           =   7
         Left            =   2972
         TabIndex        =   10
         Top             =   600
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "5"
         Height          =   360
         Index           =   8
         Left            =   3553
         TabIndex        =   11
         Top             =   216
         Width           =   447
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "5"
         Height          =   360
         Index           =   9
         Left            =   3553
         TabIndex        =   12
         Top             =   600
         Width           =   447
      End
      Begin VB.Label lbl1 
         Alignment       =   1  'Right Justify
         Caption         =   "High bitrate"
         Height          =   192
         Left            =   111
         TabIndex        =   1
         Top             =   312
         Width           =   1073
      End
      Begin VB.Label lbl2 
         Alignment       =   1  'Right Justify
         Caption         =   "Low bitrate"
         Height          =   192
         Left            =   111
         TabIndex        =   2
         Top             =   672
         Width           =   1073
      End
   End
   Begin VB.Frame fra13 
      Caption         =   "Custom"
      Height          =   672
      Left            =   112
      TabIndex        =   13
      Top             =   1200
      Width           =   4135
      Begin VB.TextBox txt20 
         Height          =   288
         Left            =   111
         TabIndex        =   14
         Top             =   240
         Width           =   3241
      End
      Begin VB.CommandButton cmdPset 
         Caption         =   "open"
         Height          =   288
         Index           =   10
         Left            =   3464
         TabIndex        =   15
         Top             =   240
         Width           =   559
      End
   End
   Begin VB.Frame fra16 
      Caption         =   "Currently playing"
      Height          =   1368
      Left            =   112
      TabIndex        =   16
      Top             =   1944
      Width           =   4135
      Begin VB.Label lbl30 
         Alignment       =   2  'Center
         Height          =   384
         Left            =   111
         TabIndex        =   17
         Top             =   240
         Width           =   3911
      End
      Begin VB.Label lbl31 
         Alignment       =   2  'Center
         Caption         =   "not playing"
         Height          =   384
         Left            =   111
         TabIndex        =   18
         Top             =   672
         Width           =   3911
      End
      Begin VB.Label lbl32 
         Alignment       =   2  'Center
         Height          =   192
         Left            =   111
         TabIndex        =   19
         Top             =   1104
         Width           =   3911
      End
   End
   Begin VB.Frame fra20 
      Caption         =   "Proxy server"
      Height          =   960
      Left            =   112
      TabIndex        =   20
      Top             =   3384
      Width           =   4135
      Begin VB.TextBox txt40 
         Height          =   288
         Left            =   111
         TabIndex        =   21
         Top             =   240
         Width           =   3911
      End
      Begin VB.CheckBox chk41 
         Caption         =   "Direct connection"
         Height          =   240
         Left            =   223
         TabIndex        =   23
         Top             =   624
         Width           =   1609
      End
      Begin VB.Label lbl22 
         Alignment       =   1  'Right Justify
         Caption         =   "[user:pass@]server:port"
         Height          =   192
         Left            =   2324
         TabIndex        =   22
         Top             =   576
         Width           =   1699
      End
   End
End
Attribute VB_Name = "frmNetradio"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'/*
'    BASS internet radio example
'    Copyright (c) 2002-2022 Un4seen Developments Ltd.
'
'   other code: mNetradio.bas
'*/

Option Explicit

Private Sub cmdPset_Click(index As Integer)
    Call Preset(index)
End Sub

Private Sub Form_Load()
    Call Initialize
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Call Finish
End Sub

Private Sub tmrStall_Timer()
    Call TimerProc
End Sub
