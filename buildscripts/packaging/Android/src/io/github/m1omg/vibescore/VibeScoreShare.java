/*
 * SPDX-License-Identifier: GPL-3.0-only
 * VibeScore - MuseScore Studio for mobile devices
 *
 * Copyright (C) 2026 VibeScore contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
package io.github.m1omg.vibescore;

import android.app.Activity;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.webkit.MimeTypeMap;

import androidx.core.content.FileProvider;

import java.io.File;
import java.util.ArrayList;

/**
 * Sends files to other apps through the system share sheet (mail, messengers, cloud storage,
 * "Save to device" of file managers, ...). Called from native code.
 */
public class VibeScoreShare
{
    private static final String TAG = "VibeScore";

    // Declared by Qt in the manifest; it covers the app's internal and external files and caches
    private static final String PROVIDER_SUFFIX = ".qtprovider";

    public static void shareFiles(final Context context, final String[] paths, final String chooserTitle)
    {
        if (context == null || paths == null || paths.length == 0) {
            return;
        }

        Runnable share = () -> {
            try {
                startShareActivity(context, paths, chooserTitle);
            } catch (Exception e) {
                Log.e(TAG, "Failed to share files: " + e);
            }
        };

        if (context instanceof Activity) {
            ((Activity) context).runOnUiThread(share);
        } else {
            share.run();
        }
    }

    private static void startShareActivity(Context context, String[] paths, String chooserTitle)
    {
        String authority = context.getPackageName() + PROVIDER_SUFFIX;

        ArrayList<Uri> uris = new ArrayList<>();
        String mimeType = null;

        for (String path : paths) {
            File file = new File(path);
            uris.add(FileProvider.getUriForFile(context, authority, file));

            String fileMimeType = mimeTypeOf(file.getName());
            if (mimeType == null) {
                mimeType = fileMimeType;
            } else if (!mimeType.equals(fileMimeType)) {
                mimeType = "*/*";
            }
        }

        Intent intent;
        if (uris.size() == 1) {
            intent = new Intent(Intent.ACTION_SEND);
            intent.putExtra(Intent.EXTRA_STREAM, uris.get(0));
        } else {
            intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris);
        }

        intent.setType(mimeType);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        // The clip data carries the read permission for all the files to the chosen app
        ClipData clip = ClipData.newRawUri(null, uris.get(0));
        for (int i = 1; i < uris.size(); ++i) {
            clip.addItem(new ClipData.Item(uris.get(i)));
        }
        intent.setClipData(clip);

        Intent chooser = Intent.createChooser(intent, chooserTitle);
        if (!(context instanceof Activity)) {
            chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }

        context.startActivity(chooser);
    }

    private static String mimeTypeOf(String fileName)
    {
        int dot = fileName.lastIndexOf('.');
        String extension = dot >= 0 ? fileName.substring(dot + 1).toLowerCase() : "";

        switch (extension) {
        case "mscz":
            return "application/x-musescore";
        case "mscx":
            return "application/x-musescore+xml";
        case "musicxml":
        case "xml":
            return "application/vnd.recordare.musicxml+xml";
        case "mxl":
            return "application/vnd.recordare.musicxml";
        default:
            break;
        }

        String mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
        return mimeType != null ? mimeType : "application/octet-stream";
    }
}
