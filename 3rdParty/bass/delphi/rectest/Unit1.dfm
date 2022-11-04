object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'BASS recording example'
  ClientHeight = 118
  ClientWidth = 373
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  DesignSize = (
    373
    118)
  TextHeight = 15
  object Bevel1: TBevel
    Left = 206
    Top = 63
    Width = 161
    Height = 15
  end
  object Label1: TLabel
    Left = 8
    Top = 66
    Width = 30
    Height = 15
    Caption = '------'
  end
  object Label2: TLabel
    Left = 180
    Top = 88
    Width = 6
    Height = 15
    Caption = '0'
  end
  object lPos: TLabel
    Left = 206
    Top = 63
    Width = 167
    Height = 13
    Alignment = taCenter
    Anchors = [akTop, akRight]
    AutoSize = False
    Caption = '----------------------------'
    ExplicitLeft = 200
  end
  object cb1: TComboBox
    Left = 8
    Top = 8
    Width = 186
    Height = 23
    Style = csDropDownList
    TabOrder = 0
    OnChange = cb1Change
  end
  object cb2: TComboBox
    Left = 8
    Top = 37
    Width = 186
    Height = 23
    Style = csDropDownList
    BiDiMode = bdLeftToRight
    ParentBiDiMode = False
    TabOrder = 1
    OnChange = cb2Change
  end
  object TrackBar1: TTrackBar
    Left = 8
    Top = 87
    Width = 170
    Height = 45
    Max = 100
    TabOrder = 2
    OnChange = TrackBar1Change
  end
  object bPlay: TButton
    Left = 206
    Top = 37
    Width = 81
    Height = 23
    Anchors = [akTop, akRight]
    Caption = 'Play'
    TabOrder = 3
    OnClick = bPlayClick
  end
  object bRecord: TButton
    Left = 206
    Top = 8
    Width = 159
    Height = 23
    Anchors = [akTop, akRight]
    Caption = 'Record'
    TabOrder = 4
    OnClick = bRecordClick
  end
  object bSave: TButton
    Left = 288
    Top = 37
    Width = 77
    Height = 23
    Anchors = [akTop, akRight]
    Caption = 'Save'
    TabOrder = 5
    OnClick = bSaveClick
  end
  object Timer1: TTimer
    Interval = 200
    OnTimer = Timer1Timer
    Left = 136
    Top = 64
  end
  object SaveDialog: TSaveDialog
    Filter = 'Wave Files|*.wav'
    Left = 96
    Top = 64
  end
end
