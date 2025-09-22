param 
(
    [string] $InputFile = "input.bin",
    [string] $OutputHeader = "output.h",
    [string] $ArrayName = "binaryData"
)

$Bytes = [System.IO.File]::ReadAllBytes($InputFile);

$Content = @();

$Content += "#pragma once";
$Content += "";
$Content += "const unsigned char $ArrayName[] = {";

$Bytes.ForEach({ $Content += "    0x{0:X2}," -f $_ });

$Content += "};";

Set-Content -Path $OutputHeader -Value $Content -Encoding ASCII;
