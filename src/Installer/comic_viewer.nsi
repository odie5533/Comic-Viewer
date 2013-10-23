; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Comic Viewer"
!define PRODUCT_VERSION "0.1"
!define PRODUCT_PUBLISHER ""
!define PRODUCT_WEB_SITE ""
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\comic_viewer.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define DIST_FILES "..\dist\*"
!define THUMBNAIL_PROVIDER "..\thumbnail_provider\"

!define GTK_INSTALLER_EXE "gtk2-runtime-2.22.0-2010-10-21-ash.exe"
!define GTK_INSTALLER_URL "http://softlayer.dl.sourceforge.net/project/gtk-win/GTK%2B%20Runtime%20Environment/GTK%2B%202.22/${GTK_INSTALLER_EXE}"

!define SHORT_NAME "cviewer"
!define HKLM_SOFTWARE "Software\${PRODUCT_NAME}"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "fileassoc.nsh"
!include "x64.nsh"
!include LogicLib.nsh
!include "Sections.nsh"
!include "WinVer.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.txt"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
; !insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
#!define MUI_FINISHPAGE_RUN "$INSTDIR\comic_viewer.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\Comic Viewer"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "Comic Viewer" SEC01
  SetOutPath $INSTDIR
  SetOverwrite try
  File /r ${DIST_FILES}
SectionEnd

Section "File Associations" SEC02
  !insertmacro APP_ASSOCIATE "cbz" "${SHORT_NAME}.cbzfile" "CBZ File" "$INSTDIR\comic_viewer.exe,0" "Open with ComicViewer" "$INSTDIR\comic_viewer.exe $\"%1$\""
  !insertmacro APP_ASSOCIATE "cbr" "${SHORT_NAME}.cbrfile" "CBR File" "$INSTDIR\comic_viewer.exe,0" "Open with ComicViewer" "$INSTDIR\comic_viewer.exe $\"%1$\""
  WriteRegStr HKLM "${HKLM_SOFTWARE}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${HKLM_SOFTWARE}\capabilities" "ApplicationName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${HKLM_SOFTWARE}\capabilities\FileAssociations" ".CBZ" "${SHORT_NAME}.CBZ"
  WriteRegStr HKLM "${HKLM_SOFTWARE}\capabilities\FileAssociations" ".CBR" "${SHORT_NAME}.CBR"
  WriteRegStr HKLM "Software\RegisteredApplications" "${PRODUCT_NAME}" "${HKLM_SOFTWARE}\capabilities"
  ;AppAssocReg::SetAppAsDefaultAll "${PRODUCT_NAME}"
  ;pop $R9
  ;DetailPrint "SetAppAsDefaultAll return value $R9"
SectionEnd

Section "Thumbnail Previews" SEC03
  SetOutPath $INSTDIR
  SetOverwrite try
  File '${THUMBNAIL_PROVIDER}\7z.exe'
  File '${THUMBNAIL_PROVIDER}\7z.dll'
  ${If} ${RunningX64}
    File /oname=ComicThumbnailProvider.dll '${THUMBNAIL_PROVIDER}\ComicThumbnailProvider64.dll'
  ${ELSE}
    File '${THUMBNAIL_PROVIDER}\ComicThumbnailProvider.dll'
  ${EndIf}
  ExecWait 'regsvr32.exe /s "$INSTDIR\ComicThumbnailProvider.dll"'
SectionEnd

; idea from the script at http://forums.winamp.com/showthread.php?t=327010
;Section "GTK+ Runtime" SEC04
;  TryAgain:
;    NSISdl::download "${GTK_INSTALLER_URL}" "$INSTDIR\${GTK_INSTALLER_EXE}"
;    Pop $R0 ;Get the return value
;    StrCmp $R0 "success" +2
;    MessageBox MB_ICONQUESTION|MB_YESNO "Setup was unable to download GTK+.$\nRetry?" IDYES TryAgain IDNO Done
;    ExecWait '"$INSTDIR\${GTK_INSTALLER_EXE}" /L=1033 /S /NOUI'
;  Done:
;    Delete "$INSTDIR\${GTK_INSTALLER_EXE}"
;SectionEnd

!macro RemoveFileExtensions
  !insertmacro APP_UNASSOCIATE "cbz" "${SHORT_NAME}.cbzfile"
  !insertmacro APP_UNASSOCIATE "cbr" "${SHORT_NAME}.cbrfile"
  DeleteRegKey HKLM "${HKLM_SOFTWARE}"
  DeleteRegValue HKLM "Software\RegisteredApplications" "${PRODUCT_NAME}"
!macroend

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\Comic Viewer\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\comic_viewer.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\comic_viewer.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Main program"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Associate file extensions for cbz and cbr files"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Installs a thumbnail preview provider for cbz and cbr files. Requires Windows Vista or 7."
;  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "Downloads the GTK+ runtime and installs it"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onInit
  ; Disables the Thumbnail Provider on unsupported windows versions
  ${If} ${AtMostWin2003}
    SectionSetFlags ${SEC03} ${SF_RO}
  ${EndIf}
FunctionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ExecWait 'regsvr32.exe /s /u "$INSTDIR\ComicThumbnailProvider.dll"'
  RMDir /r $INSTDIR

  Delete "$SMPROGRAMS\Comic Viewer\Uninstall.lnk"
  RMDir "$SMPROGRAMS\Comic Viewer"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  !insertmacro RemoveFileExtensions
  SetAutoClose true
SectionEnd
