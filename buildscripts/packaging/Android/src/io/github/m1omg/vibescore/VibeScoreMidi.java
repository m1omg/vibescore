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

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * MIDI devices (USB, Bluetooth once paired, virtual devices of other apps) through android.media.midi.
 * Port ids are "deviceId:portIndex". A source is a device's output port, which sends MIDI to the app;
 * a destination is a device's input port, which the app sends MIDI to. Called from native code.
 */
public class VibeScoreMidi
{
    private static final String TAG = "VibeScore";

    private static native void nativeMidiReceived(byte[] data);

    private static final Object s_lock = new Object();
    private static Handler s_handler;

    private static MidiDevice s_sourceDevice;
    private static MidiOutputPort s_sourcePort;
    private static MidiDevice s_destinationDevice;
    private static MidiInputPort s_destinationPort;

    /** "id\nname" for each port that sends MIDI to the app */
    public static String[] sources(Context context)
    {
        return ports(context, true);
    }

    /** "id\nname" for each port that the app can send MIDI to */
    public static String[] destinations(Context context)
    {
        return ports(context, false);
    }

    public static boolean openSource(Context context, String id)
    {
        closeSource();

        return openDevice(context, id, (device, port) -> {
            MidiOutputPort outputPort = device.openOutputPort(port);
            if (outputPort == null) {
                Log.e(TAG, "Can not open MIDI source " + id);
                closeQuietly(device);
                return;
            }

            outputPort.connect(new MidiReceiver() {
                @Override
                public void onSend(byte[] msg, int offset, int count, long timestamp)
                {
                    nativeMidiReceived(Arrays.copyOfRange(msg, offset, offset + count));
                }
            });

            synchronized (s_lock) {
                s_sourceDevice = device;
                s_sourcePort = outputPort;
            }
            Log.i(TAG, "Opened MIDI source " + id);
        });
    }

    public static void closeSource()
    {
        synchronized (s_lock) {
            closeQuietly(s_sourcePort);
            closeQuietly(s_sourceDevice);
            s_sourcePort = null;
            s_sourceDevice = null;
        }
    }

    public static boolean openDestination(Context context, String id)
    {
        closeDestination();

        return openDevice(context, id, (device, port) -> {
            MidiInputPort inputPort = device.openInputPort(port);
            if (inputPort == null) {
                Log.e(TAG, "Can not open MIDI destination " + id);
                closeQuietly(device);
                return;
            }

            synchronized (s_lock) {
                s_destinationDevice = device;
                s_destinationPort = inputPort;
            }
            Log.i(TAG, "Opened MIDI destination " + id);
        });
    }

    public static void closeDestination()
    {
        synchronized (s_lock) {
            closeQuietly(s_destinationPort);
            closeQuietly(s_destinationDevice);
            s_destinationPort = null;
            s_destinationDevice = null;
        }
    }

    public static boolean send(byte[] data)
    {
        synchronized (s_lock) {
            if (s_destinationPort == null) {
                return false;
            }

            try {
                s_destinationPort.send(data, 0, data.length);
                return true;
            } catch (Exception e) {
                Log.e(TAG, "Failed to send MIDI: " + e);
                return false;
            }
        }
    }

    private interface PortOpener
    {
        void open(MidiDevice device, int port);
    }

    private static boolean openDevice(Context context, String id, PortOpener opener)
    {
        MidiManager manager = manager(context);
        if (manager == null || id == null) {
            return false;
        }

        int separator = id.indexOf(':');
        if (separator < 0) {
            return false;
        }

        final int deviceId;
        final int port;
        try {
            deviceId = Integer.parseInt(id.substring(0, separator));
            port = Integer.parseInt(id.substring(separator + 1));
        } catch (NumberFormatException e) {
            return false;
        }

        for (MidiDeviceInfo info : devices(manager)) {
            if (info.getId() != deviceId) {
                continue;
            }

            manager.openDevice(info, device -> {
                if (device == null) {
                    Log.e(TAG, "Can not open MIDI device " + id);
                    return;
                }
                opener.open(device, port);
            }, handler());
            return true;
        }

        return false;
    }

    private static String[] ports(Context context, boolean sources)
    {
        MidiManager manager = manager(context);
        if (manager == null) {
            return new String[0];
        }

        ArrayList<String> result = new ArrayList<>();
        for (MidiDeviceInfo info : devices(manager)) {
            int count = sources ? info.getOutputPortCount() : info.getInputPortCount();
            String name = deviceName(info);

            for (int i = 0; i < count; ++i) {
                String portName = count > 1 ? name + " " + (i + 1) : name;
                result.add(info.getId() + ":" + i + "\n" + portName);
            }
        }

        return result.toArray(new String[0]);
    }

    @SuppressWarnings("deprecation") // getDevicesForTransport() needs API 33
    private static MidiDeviceInfo[] devices(MidiManager manager)
    {
        return manager.getDevices();
    }

    private static MidiManager manager(Context context)
    {
        if (context == null || !context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MIDI)) {
            return null;
        }

        return (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
    }

    private static String deviceName(MidiDeviceInfo info)
    {
        Bundle properties = info.getProperties();

        String name = properties.getString(MidiDeviceInfo.PROPERTY_NAME);
        if (name == null || name.isEmpty()) {
            String manufacturer = properties.getString(MidiDeviceInfo.PROPERTY_MANUFACTURER);
            String product = properties.getString(MidiDeviceInfo.PROPERTY_PRODUCT);
            name = ((manufacturer != null ? manufacturer + " " : "") + (product != null ? product : "")).trim();
        }

        return name.isEmpty() ? "MIDI device " + info.getId() : name;
    }

    private static Handler handler()
    {
        synchronized (s_lock) {
            if (s_handler == null) {
                HandlerThread thread = new HandlerThread("VibeScoreMidi");
                thread.start();
                s_handler = new Handler(thread.getLooper());
            }
            return s_handler;
        }
    }

    private static void closeQuietly(java.io.Closeable closeable)
    {
        if (closeable == null) {
            return;
        }

        try {
            closeable.close();
        } catch (Exception e) {
            Log.w(TAG, "Failed to close MIDI port or device: " + e);
        }
    }
}
