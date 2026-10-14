# ![VibeScore](src/appshell/qml/MuseScore/AppShell/resources/vibescore_logo.svg) VibeScore

Music notation for phones, tablets and the web browser.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](LICENSE.txt)
[![Build: Android](https://github.com/m1omg/vibescore/actions/workflows/build_android.yml/badge.svg?branch=vibescore)](https://github.com/m1omg/vibescore/actions/workflows/build_android.yml)
[![Build: iOS](https://github.com/m1omg/vibescore/actions/workflows/build_ios.yml/badge.svg?branch=vibescore)](https://github.com/m1omg/vibescore/actions/workflows/build_ios.yml)
[![Build: Web](https://github.com/m1omg/vibescore/actions/workflows/build_web.yml/badge.svg?branch=vibescore)](https://github.com/m1omg/vibescore/actions/workflows/build_web.yml)

VibeScore is [MuseScore Studio](https://github.com/musescore/MuseScore) 4.7, the free and open source music
notation program, made for touch screens: Android phones and tablets, iPhone and iPad, and web browsers. It is the
whole score editor, not a viewer: note input, palettes, properties, parts, layout and style settings, playback with
the mixer, import and export.

VibeScore is an independent project. It is not affiliated with or endorsed by Muse Group or MuseScore Limited.
Scores are compatible with MuseScore Studio 4.7 both ways.

> **Status: alpha.** Most of the app works, but expect rough edges. Please report problems in the
> [issues](https://github.com/m1omg/vibescore/issues).

## Get it

| Platform | How |
|---|---|
| **Web browser** | Open **<https://m1omg.github.io/vibescore/>** in a current Chrome, Edge, Firefox or Safari. The first start downloads about 100 MB; your scores stay in the browser's storage. |
| **Android** 9 or later | Download `VibeScore-arm64.apk` from the latest [Android build](https://github.com/m1omg/vibescore/actions/workflows/build_android.yml) (Artifacts, needs a GitHub login) or from [Releases](https://github.com/m1omg/vibescore/releases), and open it on the device. Android asks you to allow installing apps from the browser or file manager. |
| **iPhone / iPad**, iOS 17 or later | Download `VibeScore-unsigned.ipa` from the latest [iOS build](https://github.com/m1omg/vibescore/actions/workflows/build_ios.yml) or from [Releases](https://github.com/m1omg/vibescore/releases) and sideload it (see below). |

### Sideloading on iPhone and iPad

The app is not in the App Store. An unsigned IPA can be installed with a free Apple ID through a sideloading tool,
which signs it for your device:

- [SideStore](https://sidestore.io) or [AltStore](https://altstore.io) on the device, set up from a computer once
  (Windows, macOS, or Linux with [iloader](https://github.com/nab138/iloader)); then open the IPA with it.
- Apps signed with a free Apple ID run for 7 days and have to be refreshed by the tool (SideStore does it on the
  device).
- Turn on **Developer Mode** (Settings → Privacy & Security) when iOS asks for it.

## Using it on a touch screen

| On the score | |
|---|---|
| Tap | Select (with **Shift** or **Ctrl** turned on in the toolbar over the score: add to the selection, select a range) |
| Double tap | Edit the element (text, lines, ...) |
| Drag | Scroll the score. A drag that starts on a selected element or a handle moves it; with **Shift**: select an area |
| Long press | The context menu; in note input: a preview of the note, put in when you lift the finger |
| Two fingers | Zoom and scroll |
| Pen / stylus | Works like a mouse |

- **Tablets**: the panels (palettes, properties, layout, ...) open beside the score, the keypad, mixer, piano
  keyboard and timeline below it.
- **Phones**: panels open over the score, one at a time; the note input keypad opens with note input.
- **Menu** (⋯ in the top bar): all the menus of MuseScore Studio. Keyboard shortcuts work with a hardware keyboard.
- **Settings → Appearance → Interface scale** makes everything bigger or smaller.
- **Back** (Android) closes the open dialog or panel, then goes Home.

### Where scores are kept

- Android: in the app's folder on the device's storage (`Android/data/io.github.m1omg.vibescore/files/Documents/VibeScore/Scores`),
  reachable over USB.
- iPhone / iPad: in the Files app, *On My iPhone / iPad → VibeScore*.
- Web: in the browser's storage for the site. Download scores you want to keep elsewhere (File → Export, or
  Share in the Library). Clearing the site's data deletes them.

Home → **Library** lists all scores, with rename, duplicate, delete and share. **Open...** imports files from the
device (MuseScore, MusicXML, MIDI, Guitar Pro, MEI, ...). Exports (PDF, PNG, SVG, MusicXML, MIDI, audio, ...) open
the share sheet; on the web they are downloaded.

## What is different from MuseScore Studio

Not available:

- Muse Sounds, Muse Hub and VST instruments and effects (playback uses the MS Basic sound font and the
  SoundFonts you add).
- MuseScore.com and Audio.com (publishing, cloud scores), the online score import, update checks, crash reports
  and telemetry. VibeScore does not collect any data.
- Video export and AAC export.
- On the web: MIDI keyboards, plugins, Learn, language packs and audio export (for now).

## Building

See [docs/vibescore-porting.md](docs/vibescore-porting.md): building for Android, iOS (on GitHub Actions) and the web,
and how the port works.

## License

VibeScore is licensed under the [GNU General Public License version 3](LICENSE.txt), as MuseScore Studio is.
MuseScore Studio is © MuseScore Limited and its contributors; see the
[MuseScore repository](https://github.com/musescore/MuseScore) for the original program.
"MuseScore" is a trademark of MuseScore Limited; VibeScore does not use it as its name.
