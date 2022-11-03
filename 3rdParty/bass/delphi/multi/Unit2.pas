unit Unit2;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Bass;

type
  TForm2 = class(TForm)
    ListBox1: TListBox;
    Button1: TButton;
    procedure Button1Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormDestroy(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form2: TForm2;

implementation

{$R *.dfm}

procedure TForm2.Button1Click(Sender: TObject);
begin
  Form2.ModalResult := ListBox1.ItemIndex + 1;
end;

procedure TForm2.FormCreate(Sender: TObject);
var
  i: Integer;
  ADeviceInfo: BASS_DEVICEINFO;
begin
  i := 1;
  while BASS_GetDeviceInfo(I, ADeviceInfo) do
  begin
    ListBox1.Items.Add(ADeviceInfo.name);
    i := i + 1;
  end;
end;

procedure TForm2.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caFree;
end;

procedure TForm2.FormDestroy(Sender: TObject);
begin
  Form2 := nil;
end;

end.
