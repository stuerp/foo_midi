<#
.SYNOPSIS
    
.DESCRIPTION
    
.EXAMPLE
    C:\PS> .\Build-FB2KComponent.ps1
.OUTPUTS
    
#>

[CmdletBinding()]
param
(
    [parameter(Mandatory, HelpMessage='Target Name')]
        [string] $TargetName,
    [parameter(Mandatory, HelpMessage='Target File Name')]
        [string] $TargetFileName,
    [parameter(Mandatory, HelpMessage='Platform')]
        [string] $Platform,
    [parameter(Mandatory, HelpMessage='OutputPath')]
        [string] $OutputPath
)

#Requires -Version 7.2

Set-StrictMode -Version Latest;
Set-PSDebug -Strict; # Equivalent of VBA "Option Explicit".

$ErrorActionPreference = 'Stop';

# Note: The working directory is the solution directory.

Write-Host "Building package `"$TargetName`"...";

if ($Platform -eq 'x64')
{
    $PackagePath = "../out/$TargetName/x64";

    if (!(Test-Path -Path $PackagePath))
    {
        Write-Host "Creating directory `"$PackagePath`"...";
        $null = New-Item -Path '../out/' -Name "$TargetName/x64" -ItemType 'directory';
    }

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath";
    }

    if (Test-Path -Path "3rdParty/bass/x64/bass.dll")
    {
        Write-Host "Copying BASS libaries to `"$PackagePath`"...";
        Copy-Item "3rdParty/bass/x64/bass.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/x64/bassmidi.dll" -Destination "$PackagePath";
    }

    if (Test-Path -Path "$OutputPath/../../Release/vsthost32.exe")
    {
        Write-Host "Copying 32-bit vsthost32.exe to `"$PackagePath/vsthost32.exe`"...";
        Copy-Item "$OutputPath/../../Release//vsthost32.exe" -Destination "$PackagePath/vsthost32.exe";
    }

    if (Test-Path -Path "$OutputPath/vsthost64.exe")
    {
        Write-Host "Copying 64-bit vsthost64.exe to `"$PackagePath/vsthost64.exe`"...";
        Copy-Item "$OutputPath/vsthost64.exe" -Destination "$PackagePath/vsthost64.exe";
    }

    Copy-Item "$PackagePath\*" "../bin/profile/user-components-x64/$TargetName";
}
elseif ($Platform -eq 'Win32')
{
    $PackagePath = "../out/$TargetName";

    if (!(Test-Path -Path $PackagePath))
    {
        Write-Host "Creating directory `"$PackagePath`"...";
        $null = New-Item -Path '../out/' -Name $TargetName -ItemType 'directory';
    }

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath";
    }

    if (Test-Path -Path "3rdParty/bass/bass.dll")
    {
        Write-Host "Copying BASS libaries to `"$PackagePath`"...";
        Copy-Item "3rdParty/bass/bass.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/bassmidi.dll" -Destination "$PackagePath";
    }

    if (Test-Path -Path "$OutputPath/vsthost32.exe")
    {
        Write-Host "Copying 32-bit vsthost32.exe to `"$PackagePath/vsthost32.exe`"...";
        Copy-Item "$OutputPath/vsthost32.exe" -Destination "$PackagePath/vsthost32.exe";
    }

    if (Test-Path -Path "$OutputPath/../x64/Release/vsthost64.exe")
    {
        Write-Host "Copying 64-bit vsthost64.exe to `"$PackagePath/vsthost64.exe`"...";
        Copy-Item "$OutputPath/../x64/Release/vsthost64.exe" -Destination "$PackagePath/vsthost64.exe";
    }
}
else
{
    Write-Host "Unknown platform: $Platform";
    exit;
}

Compress-Archive -Force -Path ../out/$TargetName/* -DestinationPath ../out/$TargetName.fb2k-component;

Write-Host "Done.";
