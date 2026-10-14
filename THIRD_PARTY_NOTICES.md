# Third-party software in VibeScore

VibeScore is licensed under the GNU General Public License version 3 ([LICENSE.txt](LICENSE.txt)). It is based on
[MuseScore Studio](https://github.com/musescore/MuseScore) (© MuseScore Limited and others, GPL-3.0) and contains the
components below, under their own licenses. Where a license file is in this repository, its path is given; the full
texts of the licenses are also at <https://spdx.org/licenses/>.

Builds: **A** = Android app, **I** = iOS/iPadOS app, **W** = web app.

## Libraries compiled into the app

| Component | Version | License | License text | Builds |
|---|---|---|---|---|
| [Qt](https://www.qt.io) (Core, Gui, Widgets, Network, Qml, Quick, Quick Controls 2, Quick Widgets, Xml, Svg, Shader Tools, Core 5 Compat, OpenGL, Concurrent (A, I)) | 6.10.2 | LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only | [qt.io/licensing](https://www.qt.io/licensing/open-source-lgpl-obligations); the Qt sources: <https://code.qt.io> | A (shared libraries), I and W (static) |
| [FluidSynth](https://github.com/FluidSynth/fluidsynth) (modified: no GLib) | 2.3.3 | LGPL-2.1-or-later | `src/framework/audio/thirdparty/fluidsynth/fluidsynth-2.3.3/LICENSE` | A, I, W |
| [stb_vorbis](https://github.com/nothings/stb) | 1.22 | MIT OR Unlicense | end of `src/framework/audio/thirdparty/stb/stb_vorbis.c` | A, I, W |
| [FreeType](https://freetype.org) | 2.14.1 | FTL OR GPL-2.0-or-later | `src/framework/draw/thirdparty/freetype/freetype-2.14.1/LICENSE.TXT` | A, I, W |
| [HarfBuzz](https://github.com/harfbuzz/harfbuzz) (downloaded when the build is configured) | 12.3.0 | MIT ("Old MIT") | `COPYING` in the HarfBuzz sources | A, I, W |
| [KDDockWidgets](https://github.com/KDAB/KDDockWidgets) (modified by MuseScore) | 1.4.95 | GPL-2.0-only OR GPL-3.0-only | `src/framework/dockwindow/thirdparty/KDDockWidgets/LICENSE.txt` | A, I, W |
| [pugixml](https://pugixml.org) | 1.15 | MIT | `src/framework/global/thirdparty/pugixml/pugixml.hpp` | A, I, W |
| [utfcpp](https://github.com/nemtrif/utfcpp) | 4.0.9 | BSL-1.0 | `src/framework/global/thirdparty/utfcpp/LICENSE` | A, I, W |
| [picojson](https://github.com/kazuho/picojson) | | BSD-2-Clause | `src/framework/global/thirdparty/picojson/picojson.h` | A, I, W |
| kors_async, kors_logger, kors_modularity, kors_profiler, kors_msgpack, kors_rpcqueue (Igor Korsukov) | | MIT | `src/framework/global/thirdparty/kors_*/LICENSE` | A, I, W |
| [IntervalTree](https://github.com/ekg/intervaltree) | | MIT | `thirdparty/intervaltree/LICENSE` | A, I, W |
| [rtf2html](https://sourceforge.net/projects/rtf2html) (modified) | 0.2.0 | LGPL-2.1-or-later | `thirdparty/rtf2html/COPYING.LESSER` | A, I, W |
| [BeatRoot](https://code.soundsoftware.ac.uk/projects/beatroot-vamp) (MIDI import) | | GPL-2.0-or-later | `src/importexport/midi/thirdparty/beatroot/COPYING` | A, I, W |
| libmei (generated with Verovio's libmei; MEI import and export) | | see the file headers | `src/importexport/mei/thirdparty/libmei` | A, I, W |
| [mnxdom](https://github.com/rpatters1/mnxdom) (downloaded when the build is configured) | 2.1 | MIT | `LICENSE` in the mnxdom sources | A, I, W |
| [nlohmann/json](https://github.com/nlohmann/json) (used by mnxdom) | 3.12.0 | MIT | `LICENSE.MIT` in its sources | A, I, W |
| [json-schema-validator](https://github.com/pboettch/json-schema-validator) (used by mnxdom) | 2.4.0 | MIT | `LICENSE` in its sources | A, I, W |
| [W3C MNX](https://github.com/w3c-cg/mnx) schema (in mnxdom) | | W3C Community Group | the MNX repository | A, I, W |
| FFmpeg headers (no FFmpeg library is included: video export is off) | 4.4 – 8.0 | LGPL-2.1-or-later | the headers | A, I, W |
| Qt Quick Controls 1 TreeView code (LegacyTreeView), Qt labs Settings | | LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only | the file headers | A, I, W (Settings: A, I) |
| [LAME](https://lame.sourceforge.io) (MP3 export) | 3.100 | LGPL-2.0-or-later | `src/framework/audio/thirdparty/lame/COPYING` | A, I |
| [FLAC](https://xiph.org/flac) | 1.4.3 | BSD-3-Clause | `src/framework/audio/thirdparty/flac/flac-1.4.3/COPYING.Xiph` | A, I |
| [Opus](https://opus-codec.org) | 1.5.2 | BSD-3-Clause | `src/framework/audio/thirdparty/opus/opus-1.5.2/COPYING` | A, I |
| [libopusenc](https://github.com/xiph/libopusenc) | 0.2.1 | BSD-3-Clause | `src/framework/audio/thirdparty/opusenc/libopusenc-0.2.1/COPYING` | A, I |

Qt itself contains third-party code (PCRE2, libpng, libjpeg-turbo, zlib, double-conversion, tinycbor, md4c, Unicode
data, the public suffix list, glslang, SPIRV-Cross and others); Qt lists it, with versions and licenses, in its SBOM
files and at <https://doc.qt.io/qt-6/licenses-used-in-qt.html>. The web app's loader (`qtloader.js`) is
GPL-3.0-only.

## Platform parts

| Component | Version | License | Builds |
|---|---|---|---|
| [Oboe](https://github.com/google/oboe) (audio output) | 1.11.0 | Apache-2.0 | A |
| [OpenSSL](https://www.openssl.org), prebuilt by [KDAB/android_openssl](https://github.com/KDAB/android_openssl) | 3.1.8 | Apache-2.0 | A |
| LLVM libc++ (`libc++_shared.so`, Android NDK r27) | | Apache-2.0 WITH LLVM-exception | A |
| [AndroidX](https://developer.android.com/jetpack/androidx) libraries (core 1.16.0 and the libraries it uses), Kotlin standard library 1.8.22, kotlinx-coroutines 1.6.4, Guava listenablefuture 1.0, JetBrains annotations 13.0, JSpecify 1.0.0 | | Apache-2.0 | A |
| [Emscripten](https://emscripten.org) runtime (JavaScript glue, C and C++ libraries in the wasm) | 4.0.7 | MIT OR NCSA (its C library: MIT; libc++: Apache-2.0 WITH LLVM-exception) | W |
| [zlib](https://zlib.net) (Emscripten's port) | 1.3.1 | Zlib | W |

## Fonts

All fonts are inside the app.

| Font | Version | License | License text |
|---|---|---|---|
| [Leland](https://github.com/MuseScoreFonts/Leland), Leland Text | 0.80 | OFL-1.1 | `fonts/leland/LICENSE.txt` |
| [Bravura](https://github.com/steinbergmedia/bravura), Bravura Text | 1.392, 1.393 | OFL-1.1 | `fonts/bravura/OFL.txt` |
| [Petaluma](https://github.com/steinbergmedia/petaluma), Petaluma Text, Petaluma Script | 1.065, 1.10 | OFL-1.1 | `fonts/petaluma/OFL.txt` |
| [Edwin](https://github.com/MuseScoreFonts/Edwin) | 0.54 | OFL-1.1 | `fonts/edwin/LICENSE.txt` |
| Gootville, Gootville Text (based on [Gonville](https://www.chiark.greenend.org.uk/~sgtatham/gonville/)) | 1.3, 1.2 | OFL-1.1 | `fonts/gootville/readme.txt` |
| MuseJazz, MuseJazz Text | 1.0 | OFL-1.1 | `fonts/musejazz/OFL.txt` |
| [Campania](https://github.com/MarcSabatella/Campania) | 2.009 | OFL-1.1 | `fonts/campania/LICENSE` |
| Finale Maestro, Finale Broadway (and their Text fonts), MakeMusic | | OFL-1.1, as distributed with MuseScore Studio | `fonts/finalemaestro/OFL.txt`, `fonts/finalebroadway/OFL.txt` |
| [GNU FreeSerif, FreeSans](https://www.gnu.org/software/freefont/) | 0412.2263 – 0412.2268 | GPL-3.0-or-later WITH Font-exception-2.0 | in the fonts |
| MScore (based on Emmentaler and Parnassus) | 2.003 | GPL-3.0-or-later, with a font exception | in the font |
| MScore Text, MScoreBC, MScoreTabulature | | OFL-1.1 | in the fonts |
| [SMuFL](https://github.com/w3c/smufl) metadata (glyph names, classes, ranges) | | W3C Community Group | `fonts/smufl/README.md` |

## Other data

| Item | License | License text | Builds |
|---|---|---|---|
| Sound font "MS Basic" (MuseScore General, based on FluidR3Mono and FluidR3) | MIT | `share/sound/MS Basic_License.md` | A, I, W |
| MusicXML 4.0 schemas (W3C Music Notation Community Group) | W3C Community Final Specification Agreement | the schema files | A, I, W |
| Wallpaper `paper5.png` (Sun Microsystems, Toby Smithe) | LGPL-3.0 | `share/wallpapers/COPYRIGHT` | A, I, W |
| Plugins in `share/plugins` and `share/extensions` (several authors) | GPL-3.0-or-later; "Note names": GPL-2.0-only | the plugin files | A, I |
