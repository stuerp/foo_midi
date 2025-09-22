<#
.SYNOPSIS
    Builds the foobar2000 component package.
.DESCRIPTION
    This script will be executed unconditionally during the Post-build step. It copies all the necessary files to an output directory and creates the zip archive.
.EXAMPLE
    C:\PS> .\Build-FB2KComponent.ps1
.OUTPUTS
    *.fb2k-component
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

Write-Host "Building package `"$TargetName`" for platform `"$Platform`" from `"$PSScriptRoot`"...";

$PackagePath = "..\out\$TargetName";

# Create the package directory (for both x86 and x64): "..\out\TargetName" and "..\out\TargetName\x64".
Write-Host "Creating directory `"$PackagePath`"...";

$null = New-Item -Path '..\out\' -Name "$TargetName\x64" -ItemType 'directory' -Force;

# Copy the shared x86\x64 files to the package directory.
Write-Host "Copying shared component files to the package directory...";

$PackagePath = "..\out\$TargetName";

$SharedFiles = @(
    "$OutputPath..\..\x86\Release\vsthost32.exe",
    "$OutputPath..\..\x64\Release\vsthost64.exe",
    "$OutputPath..\..\x86\Release\scpipe32.exe",
    "$OutputPath..\..\x64\Release\scpipe64.exe",
    "3rdParty\fmmidi\Programs.txt",                 # juno's fmmidi
    "3rdParty\FluidSynth\FluidSynth.cfg"            # FluidSynth configuration
);

$SharedFiles | ForEach-Object {

    if (Test-Path -Path $_)
    {
        Write-Host "Copying `"$_`" to `"$PackagePath`"...";
        $null = Copy-Item $_ -Destination $PackagePath -Force;
    }
};

# Copy the platform-specific files to the package directory.
if ($Platform -eq 'x64')
{
    $PackagePath += '\x64';

    # Copy the platform-specific component DLL to the package directory.
    if (Test-Path -Path "$OutputPath\$TargetFileName")
    {
        Write-Host "Copying component `"$TargetFileName`" to `"$PackagePath`"...";

        $null = Copy-Item "$OutputPath\$TargetFileName" -Destination "$PackagePath" -Force;
    }

    # Copy the platform-specific support files to the package directory (if any).
    $DLLPath = "3rdParty\bass\x64\bass.dll";

    if (Test-Path -Path $DLLPath)
    {
        Write-Host "Copying platform-specific BASS libraries to `"$PackagePath`"...";

        $null = Copy-Item "3rdParty\bass\x64\bass.dll" -Destination "$PackagePath" -Force;
        $null = Copy-Item "3rdParty\bass\x64\bass_mpc.dll" -Destination "$PackagePath" -Force;
        $null = Copy-Item "3rdParty\bass\x64\bassflac.dll" -Destination "$PackagePath" -Force;
        $null = Copy-Item "3rdParty\bass\x64\bassmidi.dll" -Destination "$PackagePath" -Force;
        $null = Copy-Item "3rdParty\bass\x64\bassopus.dll" -Destination "$PackagePath" -Force;
        $null = Copy-Item "3rdParty\bass\x64\basswv.dll" -Destination "$PackagePath" -Force;
    }
    else
    {
        Write-Host "Failed to find platform-specific BASS libraries: `"$DLLPath`" not found.";
        exit;
    }

    # Install the component in the foobar2000 x64 components directory: "..\bin\profile\user-components-x64\TargetName"
    Write-Host "Installing $Platform component in foobar2000 64-bit profile...";

    $foobar2000Path = '..\bin';

    if (Test-Path -Path "$foobar2000Path\foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path\profile\user-components-x64";

        Write-Host "Creating directory `"$ComponentPath\$TargetName`"...";

        $null = New-Item -Path "$ComponentPath" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Copying $Platform component to foobar2000 64-bit profile...";

        $null = Copy-Item "$PackagePath\*.*" -Destination "$ComponentPath\$TargetName" -Force;
    }
    else
    {
        Write-Host "Failed to install `"$Platform`" component: foobar2000 64-bit directory not found.";
        exit;
    }

    # Install the shared component files in the foobar2000 x64 components directory: "..\bin\profile\user-components-x64\TargetName"
    Write-Host "Copying shared component files to foobar2000 64-bit profile...";

    $SharedFiles | ForEach-Object {

        if (Test-Path -Path $_)
        {
            Write-Host "Copying `"$_`" to `"$ComponentPath\$TargetName`"...";

            $null = Copy-Item $_ -Destination $ComponentPath\$TargetName -Force;
        }
    };
}
elseif ($Platform -eq 'Win32')
{
    # Copy the platform-specific component DLL to the package directory.
    if (Test-Path -Path "$OutputPath\$TargetFileName")
    {
        Write-Host "Copying component `"$TargetFileName`" to `"$PackagePath`"...";

        $null = Copy-Item "$OutputPath\$TargetFileName" -Destination "$PackagePath" -Force;
    }

    # Copy the platform-specific support files to the package directory (if any).
    $DLLPath = "3rdParty\bass\bass.dll";

    if (Test-Path -Path $DLLPath)
    {
        Write-Host "Copying platform-specific BASS libraries to `"$PackagePath`"...";

        Copy-Item "3rdParty\bass\bass.dll" -Destination "$PackagePath" -Force;
        Copy-Item "3rdParty\bass\bass_mpc.dll" -Destination "$PackagePath" -Force;
        Copy-Item "3rdParty\bass\bassflac.dll" -Destination "$PackagePath" -Force;
        Copy-Item "3rdParty\bass\bassmidi.dll" -Destination "$PackagePath" -Force;
        Copy-Item "3rdParty\bass\bassopus.dll" -Destination "$PackagePath" -Force;
        Copy-Item "3rdParty\bass\basswv.dll" -Destination "$PackagePath" -Force;
    }
    else
    {
        Write-Host "Failed to find platform-specific BASS libraries: `"$DLLPath`" not found.";
        exit;
    }

    # Install the component in the foobar2000 x64 components directory: "..\bin-x86\profile\user-components\TargetName"
    $foobar2000Path = '..\bin.x86';

    if (Test-Path -Path "$foobar2000Path\foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path\profile\user-components";

        Write-Host "Creating directory `"$ComponentPath\$TargetName`"...";

        $null = New-Item -Path "$ComponentPath" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Installing $Platform component in foobar2000 32-bit profile...";

        $null = Copy-Item "$PackagePath\*.*" -Destination "$ComponentPath\$TargetName" -Force;
    }
    else
    {
        Write-Host "Failed to install `"$Platform`" component: foobar2000 32-bit directory not found.";
        exit;
    }
}
else
{
    Write-Host "Failed to create package for unknown platform `"$Platform`".";
    exit;
}

# Create the package archive.
Write-Host "Creating archive `"$TargetName.fb2k-component`"...";

Compress-Archive -Force -Path ..\out\$TargetName\* -DestinationPath ..\out\$TargetName.fb2k-component;

Write-Host "Done.";
