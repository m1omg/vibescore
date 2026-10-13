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
package io.github.m1omg.vibescore.miditest;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * Plays notes on the test keyboard:
 * adb shell am broadcast -n io.github.m1omg.vibescore.miditest/.PlayReceiver --eia notes 60,62,64
 */
public class PlayReceiver extends BroadcastReceiver
{
    private static final String TAG = "VibeScoreMidiTest";

    @Override
    public void onReceive(Context context, Intent intent)
    {
        final int[] notes = intent.getIntArrayExtra("notes");
        if (notes == null) {
            return;
        }

        final PendingResult result = goAsync();
        new Thread(() -> {
            for (int note : notes) {
                byte key = (byte) (note & 0x7F);
                TestKeyboardService.send(new byte[] { (byte) 0x90, key, 100 });
                sleep(150);
                TestKeyboardService.send(new byte[] { (byte) 0x80, key, 0 });
                sleep(150);
                Log.i(TAG, "Played " + note);
            }
            result.finish();
        }).start();
    }

    private static void sleep(long ms)
    {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}
