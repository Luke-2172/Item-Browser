LUKE'S ITEM BROWSER - VERSION 1.0
Fallout: New Vegas 1.4.0.525, 32-bit Windows

FEATURES
- F11 opens/closes an amber, scan-lined item browser inspired by the supplied reference.
- Select an ESP/ESM, search item names, Editor IDs or file-local hexadecimal IDs.
- Separate plugin-name search and nine inventory categories.
- Defaults to items introduced by the selected plugin. Toggle Overrides to include
  records that it overrides from its masters.
- After loading a save, the JIP bridge supplies the actual active plugin list.
  Without the bridge, the browser falls back to installed plugin files and labels
  that list INSTALLED PLUGINS; some of those files may be inactive.
- Select quantity 1 / 10 / 100. Double-click an item, click Add, or press Enter when
  neither search field is focused. Requires the JIP bridge and a loaded save.
  Forms are resolved from their ORIGINAL plugin and low six hexadecimal digits,
  so the request is not tied to a particular load-order index.
- Browser and optional MCM settings are ESP-free. No load-order slot is used.

REQUIREMENTS
- Fallout: New Vegas runtime 1.4.0.525.
- xNVSE 6.3.5 or newer: https://github.com/xNVSE/NVSE/releases
- JIP LN NVSE 56.95 or newer for active plugin enumeration and adding items:
  https://www.nexusmods.com/newvegas/mods/58277
- Native Direct3D 9 / DirectInput 8. DXVK, ENB and other rendering wrappers have
  not been tested and may prevent the overlay from appearing.

INSTALL WITH MOD ORGANIZER 2
1. Install the mod ZIP as a normal archive and enable it in the left pane.
   The archive root contains NVSE and MCM; it belongs directly inside the virtual Data folder.
2. Ensure xNVSE and JIP LN are installed. Launch New Vegas through MO2 with xNVSE.
3. Load a save, then press F11. Select your plugin in the left column.
4. If the list says INSTALLED PLUGINS after loading a save, the JIP bridge has not
   initialized. Check requirements and nvse.log for script compilation errors.

MANUAL INSTALL
Copy the NVSE and MCM folders into Fallout New Vegas\Data.

CHANGE HOTKEY
Edit Data\NVSE\Plugins\LukesItemBrowser.ini:
[Controls] FunctionKey=11 means F11. Supported numbers: 1-24.
MO2: double-click the mod -> INI Files. Avoid keys assigned to other mods.
Legacy Hotkey virtual-key values are read only if FunctionKey is absent.
Esc backs out of Settings first, then closes the browser.

OPTIONAL MOD CONFIGURATION MENU (ESP-FREE)
The included MCM\LukesItemBrowser.json is automatically discovered by
MCM Extender 1.63 or newer. Load a save, open Pause > Mod Configuration >
Luke's Item Browser. No browser ESP or additional plugin slot is needed.

Install the frameworks separately if you want this optional page:
- The Mod Configuration Menu: https://www.nexusmods.com/newvegas/mods/42507
- MCM Extender: https://github.com/Stentorious/MCMExtender
Follow MCM Extender's installation instructions and dependencies, including
JohnnyGuitar NVSE, ShowOff xNVSE and UIO, as well as xNVSE and JIP LN.
The framework authors do not permit bundling their core files here.
This archive includes all of this browser's runtime and integration files.

Options: function key (F1-F24), mouse speed, background opacity, menu sounds,
pickup sounds, include overrides and text resolution.
The page and native browser share NVSE\Plugins\LukesItemBrowser.ini.
MCM Extender saves changes when you leave the pause menu. The browser reads
them within one second of returning to gameplay. Text resolution and
FontFace changes require restarting New Vegas.
The function-key slider displays the F prefix; its slider popup uses the number.

Without MCM or MCM Extender, use the browser Settings tab or the INI.
There are no MCM calls in the browser's startup scripts.
The new JSON integration has been checked against the installed Extender 1.63
scripts; its actual in-game display and save/reopen behavior still need testing.

CONTROLS
Use upper-right search. ITEMS / PLUGINS selects the target. Tab switches scope.
Click a plugin, category or item. Wheel over a list or drag its scrollbar.
Click outside a search box before using Enter to add an item.
Settings contains Clear search, Refresh plugins and the Overrides toggle.
When opening again, the plugin list is refreshed and item selection is cleared.

CURRENT LIMITATIONS
- No rotating 3D model preview. The right panel displays record details.
- The game continues running. Open in a safe location; this build does not pause it.
- Mouse/keyboard only. Controllers are not captured by this overlay.
- Names and Editor IDs come from the selected file. They are not a merged view
  of all later winning overrides or runtime changes. The addition request does
  resolve to the live form through JIP.
- Displayed Form IDs are FILE-LOCAL. Do not paste them directly into console commands.
- Includes non-playable/test inventory records where present in a plugin.
- Inactive plugin browsing cannot add items from an unloaded source plugin.
- Long names are ellipsized. No item-icon or 3D-model extraction is implemented.
- This release has not been independently retested inside New Vegas or with
  other overlay/input mods. Earlier versions were reported working in-game by the user.

VERIFICATION PERFORMED
Compiled as an x86 DLL with MSVC 2019; dependencies checked.
Read FalloutNV.esm: 2692 items introduced by the base master.
Read GunRunnersArsenal.esm: 108 introduced items and 3 override records.
Read the provided local Vegas Overhaul.esp: 94 introduced items, 3 masters.
Synthetic fixture: compressed record, own/override filtering, deleted-record
exclusion, multiword search and corrupt-length rejection.
Rendered and inspected the same menu drawing code with real base-game records.
Checked the local DirectInput mouse/keyboard shared-vtable assumption.
Standalone GPU test was blocked: Direct3DCreate9 returned null in the execution
environment. No GPU rendering or device-reset test passed here.
These checks do not replace an in-game test of hooks, input capture or inventory requests.

DIAGNOSTICS / UNINSTALL
Log: Data\NVSE\Plugins\LukesItemBrowser.log
Runtime bridge: Data\config\LukesItemBrowserRuntime.ini (may be in MO2 Overwrite).
For a problem report, include the log, nvse.log, and whether you use DXVK/ENB.
Close New Vegas before disabling or uninstalling the mod in MO2.
Manual uninstall: remove LukesItemBrowser.dll, LukesItemBrowser.ini, the LukesItemBrowser
subfolder, scripts\ln_LukesItemBrowser.txt and scripts\xm_LukesItemBrowser.txt. Also remove
NVSE\user_defined_functions\LukesItemBrowser and MCM\LukesItemBrowser.json.
The runtime INI and log may also be removed. Items already added remain in your save.

UPGRADING FROM AN EARLIER BUILD
Disable or remove the previous mod in MO2, then install this archive as a fresh mod.
Disable/remove the old LukesItemBrowserMCM.esp and MCM.gek/MCMReset.gek integration.
For an existing custom INI, migrate Hotkey=122 to FunctionKey=11 (subtract 111).
Do not merge with earlier builds: the DLL, scripts, folders and INI paths have all
been renamed. Only enable one version. Reapply custom settings in LukesItemBrowser.ini.
Native NVSE version is 100 (1.0).

MENU SOUNDS
Open: ui_vats_move.wav. Close: ui_vats_ready.wav.

CREDITS
Original browser and parser source provided in the separate GitHub source archive.
Public xNVSE plugin query ABI; JIP LN runtime functions and script runner.
zlib by Jean-loup Gailly and Mark Adler, used for compressed TES4 records.
Included zlib1.dll is a custom static wrapper built from the local MSYS2 libz.a;
it is not the unmodified upstream DLL. License is in NVSE\Plugins\LukesItemBrowser.
Reference styling supplied by the user; no game art was copied into the package.
