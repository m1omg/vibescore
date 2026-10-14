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

import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;

import org.qtproject.qt.android.bindings.QtActivity;

/**
 * Main activity: Qt's activity plus handling of documents opened from other apps
 * (VIEW intents from file managers, SEND intents from the share sheet), and the results of
 * the Bluetooth MIDI device chooser (see VibeScoreBluetoothMidi).
 *
 * Incoming documents are copied into the app's cache, because content:// URIs are only
 * readable while the intent's permission lasts; the native side then imports them.
 */
public class VibeScoreActivity extends QtActivity
{
    private static final String TAG = "VibeScore";

    private static final Object s_lock = new Object();
    private static final ArrayList<String> s_pendingFiles = new ArrayList<>();
    private static boolean s_nativeReady = false;

    private static native void nativeIncomingFile(String path);

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        handleIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        setIntent(intent);
        handleIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == VibeScoreBluetoothMidi.REQUEST_CHOOSE_DEVICE) {
            VibeScoreBluetoothMidi.onDeviceChosen(this, resultCode, data);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == VibeScoreBluetoothMidi.REQUEST_CONNECT_PERMISSION) {
            boolean granted = grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
            VibeScoreBluetoothMidi.onConnectPermissionResult(this, granted);
        }
    }

    /** Called from native code once it can receive files; returns the files received before that. */
    public static String[] takePendingFiles()
    {
        synchronized (s_lock) {
            s_nativeReady = true;
            String[] files = s_pendingFiles.toArray(new String[0]);
            s_pendingFiles.clear();
            return files;
        }
    }

    @SuppressWarnings("deprecation") // the typed getParcelable*Extra variants need API 33
    private void handleIntent(Intent intent)
    {
        if (intent == null) {
            return;
        }

        ArrayList<Uri> uris = new ArrayList<>();
        String action = intent.getAction();

        if (Intent.ACTION_VIEW.equals(action) || Intent.ACTION_EDIT.equals(action)) {
            if (intent.getData() != null) {
                uris.add(intent.getData());
            }
        } else if (Intent.ACTION_SEND.equals(action)) {
            Uri uri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
            if (uri != null) {
                uris.add(uri);
            }
        } else if (Intent.ACTION_SEND_MULTIPLE.equals(action)) {
            ArrayList<Uri> list = intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM);
            if (list != null) {
                uris.addAll(list);
            }
        }

        if (uris.isEmpty()) {
            return;
        }

        // The intent has been consumed; do not open the files again on recreation
        intent.setAction(Intent.ACTION_MAIN);
        intent.setData(null);

        final ArrayList<Uri> toCopy = uris;
        new Thread(() -> {
            for (Uri uri : toCopy) {
                String path = copyToCache(uri);
                if (path != null) {
                    deliver(path);
                }
            }
        }, "VibeScoreIncoming").start();
    }

    private static void deliver(String path)
    {
        synchronized (s_lock) {
            if (!s_nativeReady) {
                s_pendingFiles.add(path);
                return;
            }
        }
        nativeIncomingFile(path);
    }

    private String copyToCache(Uri uri)
    {
        ContentResolver resolver = getContentResolver();
        String name = displayName(resolver, uri);

        File dir = new File(getCacheDir(), "incoming/" + System.currentTimeMillis());
        if (!dir.mkdirs() && !dir.isDirectory()) {
            Log.e(TAG, "Can not create " + dir);
            return null;
        }

        File target = new File(dir, name);
        try (InputStream in = resolver.openInputStream(uri);
             OutputStream out = new FileOutputStream(target)) {
            if (in == null) {
                Log.e(TAG, "Can not open " + uri);
                return null;
            }
            byte[] buffer = new byte[64 * 1024];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to copy " + uri + ": " + e);
            return null;
        }

        return target.getAbsolutePath();
    }

    private static String displayName(ContentResolver resolver, Uri uri)
    {
        String name = null;

        if (ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())) {
            try (Cursor cursor = resolver.query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    name = cursor.getString(0);
                }
            } catch (Exception e) {
                Log.w(TAG, "No display name for " + uri + ": " + e);
            }
        }

        if (name == null) {
            name = uri.getLastPathSegment();
        }

        if (name == null || name.isEmpty()) {
            name = "score";
        }

        // Keep only the file name part
        return name.replace('/', '_').replace('\\', '_');
    }
}
