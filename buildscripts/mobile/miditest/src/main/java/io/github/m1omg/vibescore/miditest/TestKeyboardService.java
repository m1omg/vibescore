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

import android.media.midi.MidiDeviceService;
import android.media.midi.MidiReceiver;
import android.util.Log;

/** A virtual MIDI keyboard with one output port, played by PlayReceiver */
public class TestKeyboardService extends MidiDeviceService
{
    private static final String TAG = "VibeScoreMidiTest";
    private static TestKeyboardService s_instance;

    @Override
    public void onCreate()
    {
        super.onCreate();
        s_instance = this;
        Log.i(TAG, "Test keyboard created");
    }

    @Override
    public void onDestroy()
    {
        s_instance = null;
        super.onDestroy();
    }

    @Override
    public MidiReceiver[] onGetInputPortReceivers()
    {
        return new MidiReceiver[0];
    }

    static boolean send(byte[] message)
    {
        TestKeyboardService service = s_instance;
        if (service == null) {
            Log.w(TAG, "No app has opened the test keyboard");
            return false;
        }

        try {
            service.getOutputPortReceivers()[0].send(message, 0, message.length);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Failed to send: " + e);
            return false;
        }
    }
}
