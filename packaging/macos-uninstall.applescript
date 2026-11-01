-- Compiled into "Uninstall MX400 Controller.app" by the release workflow (osacompile). Installed to /Applications by the .pkg.
-- Removes everything the installer put on the Mac. Saved presets are kept unless you choose to delete them.
set productName to "MX400 Controller"
set dataName to "MX400"
set presetsDir to (POSIX path of (path to application support folder from user domain)) & "Studio 87/" & dataName
set oldPresetsDir to (POSIX path of (path to application support folder from user domain)) & "SpeakingColors/" & dataName
display dialog "Remove " & productName & " from this Mac?" & return & return & "This deletes the plugin, the standalone app, the manual and this uninstaller." buttons {"Cancel", "Remove"} default button "Cancel" with icon caution
set keepPresets to true
tell application "System Events" to set hasPresets to (exists folder presetsDir) or (exists folder oldPresetsDir)
if hasPresets then
	set r to display dialog "Also delete your saved presets?" & return & return & presetsDir buttons {"Keep presets", "Delete presets"} default button "Keep presets" with icon caution
	if button returned of r is "Delete presets" then set keepPresets to false
end if
set cmds to "rm -rf '/Library/Audio/Plug-Ins/VST3/" & productName & ".vst3' '/Applications/" & productName & ".app' '/Library/Application Support/Studio 87/" & dataName & "' '/Library/Application Support/SpeakingColors/" & dataName & "' '/Applications/Uninstall " & productName & ".app'; "
set cmds to cmds & "rmdir '/Library/Application Support/Studio 87' '/Library/Application Support/SpeakingColors' 2>/dev/null; for p in $(pkgutil --pkgs | grep '^io.github.speakingcolors.mx400controller'); do pkgutil --forget $p; done"
do shell script cmds with administrator privileges
-- standalone app settings (audio device choice only); exact file name unverified, removed if present
do shell script "rm -rf ~/Library/Application\\ Support/" & productName & " ~/Library/Preferences/" & productName & ".settings 2>/dev/null; true"
if keepPresets is false then
	do shell script "rm -rf " & quoted form of presetsDir & " " & quoted form of oldPresetsDir
	do shell script "rmdir " & quoted form of (POSIX path of (path to application support folder from user domain) & "Studio 87") & " " & quoted form of (POSIX path of (path to application support folder from user domain) & "SpeakingColors") & " 2>/dev/null; true"
end if
set doneMsg to productName & " was removed."
if keepPresets then set doneMsg to doneMsg & return & "Your saved presets were kept."
display dialog doneMsg buttons {"OK"} default button "OK"
