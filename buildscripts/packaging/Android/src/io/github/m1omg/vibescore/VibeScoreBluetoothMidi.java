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

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.companion.AssociationInfo;
import android.companion.AssociationRequest;
import android.companion.BluetoothLeDeviceFilter;
import android.companion.CompanionDeviceManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageManager;
import android.media.midi.MidiDevice;
import android.media.midi.MidiManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

/**
 * Bluetooth LE MIDI devices. Android shows them to apps only while an app keeps them open
 * (MidiManager.openBluetoothDevice); then they are MIDI devices like the others (see VibeScoreMidi).
 *
 * The user chooses a device in the system's companion device chooser, which needs no location or scanning
 * permission; the chosen devices stay associated with the app and are connected again when it starts.
 */
public class VibeScoreBluetoothMidi
{
    private static final String TAG = "VibeScore";

    /** The BLE MIDI service (MIDI over Bluetooth Low Energy specification) */
    private static final ParcelUuid MIDI_SERVICE = ParcelUuid.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");

    static final int REQUEST_CHOOSE_DEVICE = 0x564d; // results come to VibeScoreActivity
    static final int REQUEST_CONNECT_PERMISSION = 0x564e;

    private static final Object s_lock = new Object();
    private static final List<MidiDevice> s_openDevices = new ArrayList<>();
    private static final List<String> s_openAddresses = new ArrayList<>();
    private static BluetoothDevice s_waitingForPermission;

    /** Whether the device can connect Bluetooth MIDI devices */
    public static boolean isAvailable(Context context)
    {
        return companionDeviceManager(context) != null && midiManager(context) != null;
    }

    /** Shows the system's chooser of nearby Bluetooth MIDI devices; the chosen one is connected */
    public static void chooseDevice(final Activity activity)
    {
        activity.runOnUiThread(() -> {
            CompanionDeviceManager companions = companionDeviceManager(activity);
            if (companions == null || midiManager(activity) == null) {
                return;
            }

            ScanFilter scanFilter = new ScanFilter.Builder().setServiceUuid(MIDI_SERVICE).build();
            BluetoothLeDeviceFilter deviceFilter = new BluetoothLeDeviceFilter.Builder().setScanFilter(scanFilter).build();
            AssociationRequest request = new AssociationRequest.Builder()
                                         .addDeviceFilter(deviceFilter)
                                         .setSingleDevice(false)
                                         .build();

            companions.associate(request, new CompanionDeviceManager.Callback() {
                @Override
                public void onAssociationPending(IntentSender chooser)
                {
                    showChooser(activity, chooser);
                }

                @Override
                @SuppressWarnings("deprecation") // before API 33, the chooser comes here
                public void onDeviceFound(IntentSender chooser)
                {
                    showChooser(activity, chooser);
                }

                @Override
                public void onFailure(CharSequence error)
                {
                    Log.w(TAG, "No Bluetooth MIDI device chosen: " + error);
                }
            }, new Handler(Looper.getMainLooper()));
        });
    }

    /** From VibeScoreActivity.onActivityResult: the device chosen in the chooser */
    @SuppressWarnings("deprecation") // the typed getParcelableExtra needs API 33
    static void onDeviceChosen(Activity activity, int resultCode, Intent data)
    {
        if (resultCode != Activity.RESULT_OK || data == null) {
            return;
        }

        BluetoothDevice device = null;
        Object extra = data.getParcelableExtra(CompanionDeviceManager.EXTRA_DEVICE);
        if (extra instanceof ScanResult) {
            device = ((ScanResult) extra).getDevice();
        } else if (extra instanceof BluetoothDevice) {
            device = (BluetoothDevice) extra;
        } else if (Build.VERSION.SDK_INT >= 33) {
            AssociationInfo association = data.getParcelableExtra(CompanionDeviceManager.EXTRA_ASSOCIATION);
            if (association != null && association.getDeviceMacAddress() != null) {
                device = remoteDevice(activity, association.getDeviceMacAddress().toString().toUpperCase());
            }
        }

        if (device == null) {
            Log.e(TAG, "The chooser gave no Bluetooth device");
            return;
        }

        connect(activity, device);
    }

    /** Connects the devices chosen earlier (when the app starts) */
    @SuppressWarnings("deprecation") // getMyAssociations() needs API 33
    public static void connectChosenDevices(final Activity activity)
    {
        CompanionDeviceManager companions = companionDeviceManager(activity);
        if (companions == null || midiManager(activity) == null) {
            return;
        }

        List<String> addresses = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 33) {
            for (AssociationInfo association : companions.getMyAssociations()) {
                if (association.getDeviceMacAddress() != null) {
                    addresses.add(association.getDeviceMacAddress().toString().toUpperCase());
                }
            }
        } else {
            addresses.addAll(companions.getAssociations());
        }

        for (String address : addresses) {
            BluetoothDevice device = remoteDevice(activity, address);
            if (device != null) {
                connect(activity, device);
            }
        }
    }

    /** From VibeScoreActivity.onRequestPermissionsResult */
    static void onConnectPermissionResult(Activity activity, boolean granted)
    {
        BluetoothDevice device;
        synchronized (s_lock) {
            device = s_waitingForPermission;
            s_waitingForPermission = null;
        }

        if (granted && device != null) {
            connect(activity, device);
        }
    }

    private static void connect(final Activity activity, final BluetoothDevice device)
    {
        MidiManager manager = midiManager(activity);
        if (manager == null) {
            return;
        }

        final String address = device.getAddress();
        synchronized (s_lock) {
            if (s_openAddresses.contains(address)) {
                return; // already connected
            }
        }

        try {
            manager.openBluetoothDevice(device, midiDevice -> {
                if (midiDevice == null) {
                    Log.w(TAG, "Can not connect the Bluetooth MIDI device " + address);
                    return;
                }

                synchronized (s_lock) {
                    s_openDevices.add(midiDevice);
                    s_openAddresses.add(address);
                }
                Log.i(TAG, "Connected the Bluetooth MIDI device " + address);
            }, new Handler(Looper.getMainLooper()));
        } catch (SecurityException e) {
            // Some Android versions want the app to have the "nearby devices" permission
            if (Build.VERSION.SDK_INT >= 31
                && activity.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                synchronized (s_lock) {
                    s_waitingForPermission = device;
                }
                activity.runOnUiThread(() -> activity.requestPermissions(
                                           new String[] { Manifest.permission.BLUETOOTH_CONNECT }, REQUEST_CONNECT_PERMISSION));
            } else {
                Log.e(TAG, "Can not connect the Bluetooth MIDI device " + address + ": " + e);
            }
        }
    }

    private static void showChooser(Activity activity, IntentSender chooser)
    {
        try {
            activity.startIntentSenderForResult(chooser, REQUEST_CHOOSE_DEVICE, null, 0, 0, 0);
        } catch (IntentSender.SendIntentException e) {
            Log.e(TAG, "Can not show the Bluetooth device chooser: " + e);
        }
    }

    private static BluetoothDevice remoteDevice(Context context, String address)
    {
        BluetoothManager bluetooth = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = bluetooth != null ? bluetooth.getAdapter() : null;
        if (adapter == null || !BluetoothAdapter.checkBluetoothAddress(address)) {
            return null;
        }

        return adapter.getRemoteDevice(address);
    }

    private static CompanionDeviceManager companionDeviceManager(Context context)
    {
        if (!context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_COMPANION_DEVICE_SETUP)
            || !context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            return null;
        }

        return (CompanionDeviceManager) context.getSystemService(Context.COMPANION_DEVICE_SERVICE);
    }

    private static MidiManager midiManager(Context context)
    {
        if (!context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MIDI)) {
            return null;
        }

        return (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
    }
}
