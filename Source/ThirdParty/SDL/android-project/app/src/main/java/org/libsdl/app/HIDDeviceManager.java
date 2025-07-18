// Modified by Yao Wei Tjong for Urho3D

package org.libsdl.app;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.util.Log;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.hardware.usb.*;
import android.os.Handler;
import android.os.Looper;

import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;

public class HIDDeviceManager {
    private static final String TAG = "hidapi";
    private static final String ACTION_USB_PERMISSION = "org.libsdl.app.USB_PERMISSION";

    private static HIDDeviceManager sManager;
    private static int sManagerRefCount = 0;

    public static HIDDeviceManager acquire(Context context) {
        if (sManagerRefCount == 0) {
            sManager = new HIDDeviceManager(context);
        }
        ++sManagerRefCount;
        return sManager;
    }

    public static void release(HIDDeviceManager manager) {
        if (manager == sManager) {
            --sManagerRefCount;
            if (sManagerRefCount == 0) {
                sManager.close();
                sManager = null;
            }
        }
    }

    private Context mContext;
    private HashMap<Integer, HIDDevice> mDevicesById = new HashMap<Integer, HIDDevice>();
    private HashMap<UsbDevice, HIDDeviceUSB> mUSBDevices = new HashMap<UsbDevice, HIDDeviceUSB>();
    private HashMap<BluetoothDevice, HIDDeviceBLESteamController> mBluetoothDevices = new HashMap<BluetoothDevice, HIDDeviceBLESteamController>();
    private int mNextDeviceId = 0;
    private SharedPreferences mSharedPreferences = null;
    private boolean mIsChromebook = false;
    private UsbManager mUsbManager;
    private Handler mHandler;
    private BluetoothManager mBluetoothManager;
    private List<BluetoothDevice> mLastBluetoothDevices;

    private final BroadcastReceiver mUsbBroadcast = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                handleUsbDeviceAttached(usbDevice);
            } else if (action.equals(UsbManager.ACTION_USB_DEVICE_DETACHED)) {
                UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                handleUsbDeviceDetached(usbDevice);
            } else if (action.equals(HIDDeviceManager.ACTION_USB_PERMISSION)) {
                UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                handleUsbDevicePermission(usbDevice, intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false));
            }
        }
    };

    private final BroadcastReceiver mBluetoothBroadcast = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            // Bluetooth device was connected. If it was a Steam Controller, handle it
            if (action.equals(BluetoothDevice.ACTION_ACL_CONNECTED)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                Log.d(TAG, "Bluetooth device connected: " + device);

                if (isSteamController(device)) {
                    connectBluetoothDevice(device);
                }
            }

            // Bluetooth device was disconnected, remove from controller manager (if any)
            if (action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                Log.d(TAG, "Bluetooth device disconnected: " + device);

                disconnectBluetoothDevice(device);
            }
        }
    };

    private HIDDeviceManager(final Context context) {
        mContext = context;

        // Dry - HIDAPI is built-in inside our custom SDL library, so no need to load libhidapi.so

        HIDDeviceRegisterCallback();

        mSharedPreferences = mContext.getSharedPreferences("hidapi", Context.MODE_PRIVATE);
        mIsChromebook = mContext.getPackageManager().hasSystemFeature("org.chromium.arc.device_management");

//        if (shouldClear) {
//            SharedPreferences.Editor spedit = mSharedPreferences.edit();
//            spedit.clear();
//            spedit.commit();
//        }
//        else
        {
            mNextDeviceId = mSharedPreferences.getInt("next_device_id", 0);
        }

        initializeUSB();

        // Dry - check first if Bluetooth is available
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            initializeBluetooth();
        }
    }

    public Context getContext() {
        return mContext;
    }

    public int getDeviceIDForIdentifier(String identifier) {
        SharedPreferences.Editor spedit = mSharedPreferences.edit();

        int result = mSharedPreferences.getInt(identifier, 0);
        if (result == 0) {
            result = mNextDeviceId++;
            spedit.putInt("next_device_id", mNextDeviceId);
        }

        spedit.putInt(identifier, result);
        spedit.commit();
        return result;
    }

    private void initializeUSB() {
        mUsbManager = (UsbManager)mContext.getSystemService(Context.USB_SERVICE);

        /*
        // Logging
        for (UsbDevice device : mUsbManager.getDeviceList().values()) {
            Log.i(TAG,"Path: " + device.getDeviceName());
            Log.i(TAG,"Manufacturer: " + device.getManufacturerName());
            Log.i(TAG,"Product: " + device.getProductName());
            Log.i(TAG,"ID: " + device.getDeviceId());
            Log.i(TAG,"Class: " + device.getDeviceClass());
            Log.i(TAG,"Protocol: " + device.getDeviceProtocol());
            Log.i(TAG,"Vendor ID " + device.getVendorId());
            Log.i(TAG,"Product ID: " + device.getProductId());
            Log.i(TAG,"Interface count: " + device.getInterfaceCount());
            Log.i(TAG,"---------------------------------------");

            // Get interface details
            for (int index = 0; index < device.getInterfaceCount(); index++) {
                UsbInterface mUsbInterface = device.getInterface(index);
                Log.i(TAG,"  *****     *****");
                Log.i(TAG,"  Interface index: " + index);
                Log.i(TAG,"  Interface ID: " + mUsbInterface.getId());
                Log.i(TAG,"  Interface class: " + mUsbInterface.getInterfaceClass());
                Log.i(TAG,"  Interface subclass: " + mUsbInterface.getInterfaceSubclass());
                Log.i(TAG,"  Interface protocol: " + mUsbInterface.getInterfaceProtocol());
                Log.i(TAG,"  Endpoint count: " + mUsbInterface.getEndpointCount());

                // Get endpoint details 
                for (int epi = 0; epi < mUsbInterface.getEndpointCount(); epi++)
                {
                    UsbEndpoint mEndpoint = mUsbInterface.getEndpoint(epi);
                    Log.i(TAG,"    ++++   ++++   ++++");
                    Log.i(TAG,"    Endpoint index: " + epi);
                    Log.i(TAG,"    Attributes: " + mEndpoint.getAttributes());
                    Log.i(TAG,"    Direction: " + mEndpoint.getDirection());
                    Log.i(TAG,"    Number: " + mEndpoint.getEndpointNumber());
                    Log.i(TAG,"    Interval: " + mEndpoint.getInterval());
                    Log.i(TAG,"    Packet size: " + mEndpoint.getMaxPacketSize());
                    Log.i(TAG,"    Type: " + mEndpoint.getType());
                }
            }
        }
        Log.i(TAG," No more devices connected.");
        */

        // Register for USB broadcasts and permission completions
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(HIDDeviceManager.ACTION_USB_PERMISSION);
        mContext.registerReceiver(mUsbBroadcast, filter);

        for (UsbDevice usbDevice : mUsbManager.getDeviceList().values()) {
            handleUsbDeviceAttached(usbDevice);
        }
    }

    UsbManager getUSBManager() {
        return mUsbManager;
    }

    private void shutdownUSB() {
        try {
            mContext.unregisterReceiver(mUsbBroadcast);
        } catch (Exception e) {
            // We may not have registered, that's okay
        }
    }

    private boolean isHIDDeviceUSB(UsbDevice usbDevice) {
        for (int interface_number = 0; interface_number < usbDevice.getInterfaceCount(); ++interface_number) {
            if (isHIDDeviceInterface(usbDevice, interface_number)) {
                return true;
            }
        }
        return false;
    }

    private boolean isHIDDeviceInterface(UsbDevice usbDevice, int interface_number) {
        UsbInterface usbInterface = usbDevice.getInterface(interface_number);
        if (usbInterface.getInterfaceClass() == UsbConstants.USB_CLASS_HID) {
            return true;
        }
        if (interface_number == 0) {
            if (isXbox360Controller(usbDevice, usbInterface) || isXboxOneController(usbDevice, usbInterface)) {
                return true;
            }
        }
        return false;
    }

    private boolean isXbox360Controller(UsbDevice usbDevice, UsbInterface usbInterface) {
        final int XB360_IFACE_SUBCLASS = 93;
        final int XB360_IFACE_PROTOCOL = 1; // Wired only
        final int[] SUPPORTED_VENDORS = {
            0x0079, // GPD Win 2
            0x044f, // Thrustmaster
            0x045e, // Microsoft
            0x046d, // Logitech
            0x056e, // Elecom
            0x06a3, // Saitek
            0x0738, // Mad Catz
            0x07ff, // Mad Catz
            0x0e6f, // Unknown
            0x0f0d, // Hori
            0x11c9, // Nacon
            0x12ab, // Unknown
            0x1430, // RedOctane
            0x146b, // BigBen
            0x1532, // Razer Sabertooth
            0x15e4, // Numark
            0x162e, // Joytech
            0x1689, // Razer Onza
            0x1bad, // Harmonix
            0x24c6, // PowerA
        };

        if (usbInterface.getInterfaceClass() == UsbConstants.USB_CLASS_VENDOR_SPEC &&
            usbInterface.getInterfaceSubclass() == XB360_IFACE_SUBCLASS &&
            usbInterface.getInterfaceProtocol() == XB360_IFACE_PROTOCOL) {
            int vendor_id = usbDevice.getVendorId();
            for (int supportedVid : SUPPORTED_VENDORS) {
                if (vendor_id == supportedVid) {
                    return true;
                }
            }
        }
        return false;
    }

    private boolean isXboxOneController(UsbDevice usbDevice, UsbInterface usbInterface) {
        final int XB1_IFACE_SUBCLASS = 71;
        final int XB1_IFACE_PROTOCOL = 208;
        final int[] SUPPORTED_VENDORS = {
            0x045e, // Microsoft
            0x0738, // Mad Catz
            0x0e6f, // Unknown
            0x0f0d, // Hori
            0x1532, // Razer Wildcat
            0x24c6, // PowerA
        };

        if (usbInterface.getInterfaceClass() == UsbConstants.USB_CLASS_VENDOR_SPEC &&
            usbInterface.getInterfaceSubclass() == XB1_IFACE_SUBCLASS &&
            usbInterface.getInterfaceProtocol() == XB1_IFACE_PROTOCOL) {
            int vendor_id = usbDevice.getVendorId();
            for (int supportedVid : SUPPORTED_VENDORS) {
                if (vendor_id == supportedVid) {
                    return true;
                }
            }
        }
        return false;
    }

    private void handleUsbDeviceAttached(UsbDevice usbDevice) {
        if (isHIDDeviceUSB(usbDevice)) {
            connectHIDDeviceUSB(usbDevice);
        }
    }

    private void handleUsbDeviceDetached(UsbDevice usbDevice) {
        HIDDeviceUSB device = mUSBDevices.get(usbDevice);
        if (device == null)
            return;

        int id = device.getId();
        mUSBDevices.remove(usbDevice);
        mDevicesById.remove(id);
        device.shutdown();
        HIDDeviceDisconnected(id);
    }

    private void handleUsbDevicePermission(UsbDevice usbDevice, boolean permission_granted) {
        HIDDeviceUSB device = mUSBDevices.get(usbDevice);
        if (device == null)
            return;

        boolean opened = false;
        if (permission_granted) {
            opened = device.open();
        }
        HIDDeviceOpenResult(device.getId(), opened);
    }

    private void connectHIDDeviceUSB(UsbDevice usbDevice) {
        synchronized (this) {
            for (int interface_number = 0; interface_number < usbDevice.getInterfaceCount(); interface_number++) {
                if (isHIDDeviceInterface(usbDevice, interface_number)) {
                    HIDDeviceUSB device = new HIDDeviceUSB(this, usbDevice, interface_number);
                    int id = device.getId();
                    mUSBDevices.put(usbDevice, device);
                    mDevicesById.put(id, device);
                    HIDDeviceConnected(id, device.getIdentifier(), device.getVendorId(), device.getProductId(), device.getSerialNumber(), device.getVersion(), device.getManufacturerName(), device.getProductName(), interface_number);
                    break;
                }
            }
        }
    }

    // Dry - suppress lint error
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void initializeBluetooth() {
        Log.d(TAG, "Initializing Bluetooth");

        if (mContext.getPackageManager().checkPermission(android.Manifest.permission.BLUETOOTH, mContext.getPackageName()) != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, "Couldn't initialize Bluetooth, missing android.permission.BLUETOOTH");
            return;
        }

        // Find bonded bluetooth controllers and create SteamControllers for them
        mBluetoothManager = (BluetoothManager)mContext.getSystemService(Context.BLUETOOTH_SERVICE);
        if (mBluetoothManager == null) {
            // This device doesn't support Bluetooth.
            return;
        }

        BluetoothAdapter btAdapter = mBluetoothManager.getAdapter();
        if (btAdapter == null) {
            // This device has Bluetooth support in the codebase, but has no available adapters.
            return;
        }

        // Get our bonded devices.
        for (BluetoothDevice device : btAdapter.getBondedDevices()) {

            Log.d(TAG, "Bluetooth device available: " + device);
            if (isSteamController(device)) {
                connectBluetoothDevice(device);
            }

        }

        // NOTE: These don't work on Chromebooks, to my undying dismay.
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        mContext.registerReceiver(mBluetoothBroadcast, filter);

        if (mIsChromebook) {
            mHandler = new Handler(Looper.getMainLooper());
            mLastBluetoothDevices = new ArrayList<BluetoothDevice>();

            // final HIDDeviceManager finalThis = this;
            // mHandler.postDelayed(new Runnable() {
            //     @Override
            //     public void run() {
            //         finalThis.chromebookConnectionHandler();
            //     }
            // }, 5000);
        }
    }

    private void shutdownBluetooth() {
        try {
            mContext.unregisterReceiver(mBluetoothBroadcast);
        } catch (Exception e) {
            // We may not have registered, that's okay
        }
    }

    // Chromebooks do not pass along ACTION_ACL_CONNECTED / ACTION_ACL_DISCONNECTED properly.
    // This function provides a sort of dummy version of that, watching for changes in the
    // connected devices and attempting to add controllers as things change.
    // Dry - suppress lint error
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void chromebookConnectionHandler() {
        if (!mIsChromebook) {
            return;
        }

        ArrayList<BluetoothDevice> disconnected = new ArrayList<BluetoothDevice>();
        ArrayList<BluetoothDevice> connected = new ArrayList<BluetoothDevice>();

        List<BluetoothDevice> currentConnected = mBluetoothManager.getConnectedDevices(BluetoothProfile.GATT);

        for (BluetoothDevice bluetoothDevice : currentConnected) {
            if (!mLastBluetoothDevices.contains(bluetoothDevice)) {
                connected.add(bluetoothDevice);
            }
        }
        for (BluetoothDevice bluetoothDevice : mLastBluetoothDevices) {
            if (!currentConnected.contains(bluetoothDevice)) {
                disconnected.add(bluetoothDevice);
            }
        }

        mLastBluetoothDevices = currentConnected;

        for (BluetoothDevice bluetoothDevice : disconnected) {
            disconnectBluetoothDevice(bluetoothDevice);
        }
        for (BluetoothDevice bluetoothDevice : connected) {
            connectBluetoothDevice(bluetoothDevice);
        }

        final HIDDeviceManager finalThis = this;
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                finalThis.chromebookConnectionHandler();
            }
        }, 10000);
    }

    public boolean connectBluetoothDevice(BluetoothDevice bluetoothDevice) {
        Log.v(TAG, "connectBluetoothDevice device=" + bluetoothDevice);
        synchronized (this) {
            if (mBluetoothDevices.containsKey(bluetoothDevice)) {
                Log.v(TAG, "Steam controller with address " + bluetoothDevice + " already exists, attempting reconnect");

                HIDDeviceBLESteamController device = mBluetoothDevices.get(bluetoothDevice);
                device.reconnect();

                return false;
            }
            HIDDeviceBLESteamController device = new HIDDeviceBLESteamController(this, bluetoothDevice);
            int id = device.getId();
            mBluetoothDevices.put(bluetoothDevice, device);
            mDevicesById.put(id, device);

            // The Steam Controller will mark itself connected once initialization is complete
        }
        return true;
    }

    public void disconnectBluetoothDevice(BluetoothDevice bluetoothDevice) {
        synchronized (this) {
            HIDDeviceBLESteamController device = mBluetoothDevices.get(bluetoothDevice);
            if (device == null)
                return;

            int id = device.getId();
            mBluetoothDevices.remove(bluetoothDevice);
            mDevicesById.remove(id);
            device.shutdown();
            HIDDeviceDisconnected(id);
        }
    }

    // Dry - suppress lint error
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public boolean isSteamController(BluetoothDevice bluetoothDevice) {
        // Sanity check.  If you pass in a null device, by definition it is never a Steam Controller.
        if (bluetoothDevice == null) {
            return false;
        }

        // If the device has no local name, we really don't want to try an equality check against it.
        if (bluetoothDevice.getName() == null) {
            return false;
        }

        return bluetoothDevice.getName().equals("SteamController") && ((bluetoothDevice.getType() & BluetoothDevice.DEVICE_TYPE_LE) != 0);
    }

    private void close() {
        shutdownUSB();
        shutdownBluetooth();
        synchronized (this) {
            for (HIDDevice device : mDevicesById.values()) {
                device.shutdown();
            }
            mDevicesById.clear();
            mBluetoothDevices.clear();
            HIDDeviceReleaseCallback();
        }
    }

    public void setFrozen(boolean frozen) {
        synchronized (this) {
            for (HIDDevice device : mDevicesById.values()) {
                device.setFrozen(frozen);
            }
        }        
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    private HIDDevice getDevice(int id) {
        synchronized (this) {
            HIDDevice result = mDevicesById.get(id);
            if (result == null) {
                Log.v(TAG, "No device for id: " + id);
                Log.v(TAG, "Available devices: " + mDevicesById.keySet());
            }
            return result;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////// JNI interface functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    public boolean openDevice(int deviceID) {
        // Look to see if this is a USB device and we have permission to access it
        for (HIDDeviceUSB device : mUSBDevices.values()) {
            if (deviceID == device.getId()) {
                UsbDevice usbDevice = device.getDevice();
                if (!mUsbManager.hasPermission(usbDevice)) {
                    HIDDeviceOpenPending(deviceID);
                    try {
                        mUsbManager.requestPermission(usbDevice, PendingIntent.getBroadcast(mContext, 0, new Intent(HIDDeviceManager.ACTION_USB_PERMISSION), 0));
                    } catch (Exception e) {
                        Log.v(TAG, "Couldn't request permission for USB device " + usbDevice);
                        HIDDeviceOpenResult(deviceID, false);
                    }
                    return false;
                }
                break;
            }
        }

        try {
            Log.v(TAG, "openDevice deviceID=" + deviceID);
            HIDDevice device;
            device = getDevice(deviceID);
            if (device == null) {
                HIDDeviceDisconnected(deviceID);
                return false;
            }

            return device.open();
        } catch (Exception e) {
            Log.e(TAG, "Got exception: " + Log.getStackTraceString(e));
        }
        return false;
    }

    public int sendOutputReport(int deviceID, byte[] report) {
        try {
            Log.v(TAG, "sendOutputReport deviceID=" + deviceID + " length=" + report.length);
            HIDDevice device;
            device = getDevice(deviceID);
            if (device == null) {
                HIDDeviceDisconnected(deviceID);
                return -1;
            }

            return device.sendOutputReport(report);
        } catch (Exception e) {
            Log.e(TAG, "Got exception: " + Log.getStackTraceString(e));
        }
        return -1;
    }

    public int sendFeatureReport(int deviceID, byte[] report) {
        try {
            Log.v(TAG, "sendFeatureReport deviceID=" + deviceID + " length=" + report.length);
            HIDDevice device;
            device = getDevice(deviceID);
            if (device == null) {
                HIDDeviceDisconnected(deviceID);
                return -1;
            }

            return device.sendFeatureReport(report);
        } catch (Exception e) {
            Log.e(TAG, "Got exception: " + Log.getStackTraceString(e));
        }
        return -1;
    }

    public boolean getFeatureReport(int deviceID, byte[] report) {
        try {
            Log.v(TAG, "getFeatureReport deviceID=" + deviceID);
            HIDDevice device;
            device = getDevice(deviceID);
            if (device == null) {
                HIDDeviceDisconnected(deviceID);
                return false;
            }

            return device.getFeatureReport(report);
        } catch (Exception e) {
            Log.e(TAG, "Got exception: " + Log.getStackTraceString(e));
        }
        return false;
    }

    public void closeDevice(int deviceID) {
        try {
            Log.v(TAG, "closeDevice deviceID=" + deviceID);
            HIDDevice device;
            device = getDevice(deviceID);
            if (device == null) {
                HIDDeviceDisconnected(deviceID);
                return;
            }

            device.close();
        } catch (Exception e) {
            Log.e(TAG, "Got exception: " + Log.getStackTraceString(e));
        }
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////// Native methods
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    private native void HIDDeviceRegisterCallback();
    private native void HIDDeviceReleaseCallback();

    native void HIDDeviceConnected(int deviceID, String identifier, int vendorId, int productId, String serial_number, int release_number, String manufacturer_string, String product_string, int interface_number);
    native void HIDDeviceOpenPending(int deviceID);
    native void HIDDeviceOpenResult(int deviceID, boolean opened);
    native void HIDDeviceDisconnected(int deviceID);

    native void HIDDeviceInputReport(int deviceID, byte[] report);
    native void HIDDeviceFeatureReport(int deviceID, byte[] report);
}
