!include "MUI2.nsh"

; General
Name "CookieMonster"
OutFile "CookieMonster_Setup.exe"
InstallDir "$PROGRAMFILES\CookieMonster"
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Language
!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    
    ; Copy executable
    File "build\Release\cookiemonster.exe"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Create start menu shortcut
    CreateDirectory "$SMPROGRAMS\CookieMonster"
    CreateShortCut "$SMPROGRAMS\CookieMonster\CookieMonster.lnk" "$INSTDIR\cookiemonster.exe"
    CreateShortCut "$SMPROGRAMS\CookieMonster\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    
    ; Write registry keys for uninstaller
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CookieMonster" \
        "DisplayName" "CookieMonster"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CookieMonster" \
        "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CookieMonster" \
        "Publisher" "CookieMonster Team"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CookieMonster" \
        "DisplayVersion" "1.1.0"
SectionEnd

Section "Uninstall"
    ; Remove files
    Delete "$INSTDIR\cookiemonster.exe"
    Delete "$INSTDIR\Uninstall.exe"
    
    ; Remove shortcuts
    Delete "$SMPROGRAMS\CookieMonster\CookieMonster.lnk"
    Delete "$SMPROGRAMS\CookieMonster\Uninstall.lnk"
    RMDir "$SMPROGRAMS\CookieMonster"
    
    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CookieMonster"
    
    ; Remove installation directory
    RMDir "$INSTDIR"
SectionEnd 