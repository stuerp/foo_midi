object Form2: TForm2
  Left = 193
  Top = 107
  BorderStyle = bsDialog
  ClientHeight = 106
  ClientWidth = 200
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object ListBox1: TListBox
    Left = 8
    Top = 8
    Width = 185
    Height = 65
    ItemHeight = 13
    TabOrder = 0
  end
  object Button1: TButton
    Left = 120
    Top = 80
    Width = 75
    Height = 17
    Caption = 'OK'
    Default = True
    TabOrder = 1
    OnClick = Button1Click
  end
end
