<#
.SYNOPSIS
    Builds the foobar2000 component package.
.DESCRIPTION
    This script will be executed unconditionally during the Post-build step. It copies all the necessary files to an output directory and creates the zip archive.
.EXAMPLE
    C:\PS> .\Build-FB2KComponent.ps1
.OUTPUTS
    foo_midi.fb2k-component
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
        Copy-Item "3rdParty/bass/x64/bass_mpc.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/x64/bassflac.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/x64/bassmidi.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/x64/bassopus.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/x64/basswv.dll" -Destination "$PackagePath";
    }

    if (Test-Path -Path "../bin")
    {
        Write-Host "Installing component in foobar2000 32-bit...";
        Copy-Item "$PackagePath/../*" "../bin/x86/profile/user-components/$TargetName" -Force;

        Write-Host "Installing component in foobar2000 64-bit...";
        Copy-Item "$PackagePath/*" "../bin/profile/user-components-x64/$TargetName" -Force;
    }
}
elseif ($Platform -eq 'Win32')
{
    $PackagePath = "../out/$TargetName";

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

    if (Test-Path -Path "3rdParty/bass/bass.dll")
    {
        Write-Host "Copying BASS libaries to `"$PackagePath`"...";
        Copy-Item "3rdParty/bass/bass.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/bass_mpc.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/bassflac.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/bassmidi.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/bassopus.dll" -Destination "$PackagePath";
        Copy-Item "3rdParty/bass/basswv.dll" -Destination "$PackagePath";
    }

    if (Test-Path -Path "../bin")
    {
        Write-Host "Installing component in foobar2000 32-bit...";
        Copy-Item "$PackagePath/*" "../bin/x86/profile/user-components/$TargetName" -Force;

        Write-Host "Installing component in foobar2000 64-bit...";
        Copy-Item "$PackagePath/x64/*" "../bin/profile/user-components-x64/$TargetName" -Force;
    }
}
else
{
    Write-Host "Unknown platform: $Platform";
    exit;
}

$PackagePath = "../out/$TargetName";

if (Test-Path -Path "$OutputPath/vsthost32.exe")
{
    Write-Host "Copying vsthost32.exe to `"$PackagePath/vsthost32.exe`"...";
    Copy-Item "$OutputPath/vsthost32.exe" -Destination "$PackagePath/vsthost32.exe";
}

if (Test-Path -Path "$OutputPath/../x64/Release/vsthost64.exe")
{
    Write-Host "Copying vsthost64.exe to `"$PackagePath/vsthost64.exe`"...";
    Copy-Item "$OutputPath/../x64/Release/vsthost64.exe" -Destination "$PackagePath/vsthost64.exe";
}

if (Test-Path -Path "$OutputPath/scpipe32.exe")
{
    Write-Host "Copying scpipe32.exe to `"$PackagePath/scpipe32.exe`"...";
    Copy-Item "$OutputPath/scpipe32.exe" -Destination "$PackagePath/scpipe32.exe";
}

if (Test-Path -Path "$OutputPath/../x64/Release/scpipe64.exe")
{
    Write-Host "Copying scpipe64.exe to `"$PackagePath/scpipe64.exe`"...";
    Copy-Item "$OutputPath/../x64/Release/scpipe64.exe" -Destination "$PackagePath/scpipe64.exe";
}

Compress-Archive -Force -Path ../out/$TargetName/* -DestinationPath ../out/$TargetName.fb2k-component;

Write-Host "Done.";
