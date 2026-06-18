package com.osaig.sdk.ble.demo;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import java.util.regex.Pattern;

@SuppressLint("MissingPermission")
public class MainActivity extends Activity {
    private static final int REQUEST_BLE_PERMISSIONS = 1001;
    private static final UUID OSAIG_SERVICE_UUID =
            UUID.fromString("00001910-0000-1000-8000-00805f9b34fb");
    private static final UUID OSAIG_CHARACTERISTIC_UUID =
            UUID.fromString("dfd4416e-1810-47f7-8248-eb8be3dc47f9");
    private static final UUID CLIENT_CHARACTERISTIC_CONFIG_UUID =
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final Pattern OSAIG_NAME_PATTERN = Pattern.compile("^OSAIG-[0-9A-F]{4}$");
    private static final int BLE_PACKET_LIMIT = 180;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final SimpleDateFormat timeFormat =
            new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic communicateCharacteristic;
    private boolean scanning;
    private boolean notifyEnabled;

    private TextView statusText;
    private TextView responseText;
    private TextView logText;
    private Button scanButton;
    private Button sendButton;
    private Button disconnectButton;

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            String name = resolveDeviceName(result);
            if (!isTargetName(name)) {
                return;
            }

            runOnUiThreadSafe(() -> {
                appendLog("Found " + name + " [" + device.getAddress() + "]");
                stopScan();
                connect(device);
            });
        }

        @Override
        public void onScanFailed(int errorCode) {
            runOnUiThreadSafe(() -> {
                scanning = false;
                updateButtons();
                setStatus("Scan failed: " + errorCode);
                appendLog("Scan failed: " + errorCode);
            });
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                runOnUiThreadSafe(() -> {
                    setStatus("Connected. Discovering services...");
                    appendLog("GATT connected");
                    updateButtons();
                });
                gatt.discoverServices();
                return;
            }

            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                runOnUiThreadSafe(() -> {
                    communicateCharacteristic = null;
                    notifyEnabled = false;
                    setStatus("Disconnected");
                    appendLog("GATT disconnected, status=" + status);
                    closeGatt();
                    updateButtons();
                });
            }
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            runOnUiThreadSafe(() -> {
                appendLog("MTU changed: mtu=" + mtu + " status=" + status);
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    setStatus("Ready. Send a ping.");
                } else {
                    setStatus("Ready. MTU request failed: " + status);
                }
                updateButtons();
            });
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                runOnUiThreadSafe(() -> {
                    setStatus("Service discovery failed: " + status);
                    appendLog("Service discovery failed: " + status);
                    updateButtons();
                });
                return;
            }

            BluetoothGattService service = gatt.getService(OSAIG_SERVICE_UUID);
            BluetoothGattCharacteristic characteristic = service == null
                    ? null
                    : service.getCharacteristic(OSAIG_CHARACTERISTIC_UUID);

            if (characteristic == null) {
                runOnUiThreadSafe(() -> {
                    setStatus("OSAIG BLE characteristic not found");
                    appendLog("Missing characteristic " + OSAIG_CHARACTERISTIC_UUID);
                    updateButtons();
                });
                return;
            }

            communicateCharacteristic = characteristic;
            runOnUiThreadSafe(() -> {
                setStatus("Characteristic found. Enabling notify/indicate...");
                appendLog("Characteristic ready, properties=" + formatProperties(characteristic));
                updateButtons();
            });
            enableNotify(gatt, characteristic);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            runOnUiThreadSafe(() -> {
                if (CLIENT_CHARACTERISTIC_CONFIG_UUID.equals(descriptor.getUuid())
                        && status == BluetoothGatt.GATT_SUCCESS) {
                    notifyEnabled = true;
                    appendLog("Notify enabled");
                    setStatus("Notify enabled. Requesting MTU...");
                    if (!gatt.requestMtu(247)) {
                        appendLog("requestMtu returned false");
                        setStatus("Ready. MTU request did not start.");
                    }
                } else {
                    notifyEnabled = false;
                    setStatus("Notify setup failed: " + status);
                    appendLog("Descriptor write status=" + status);
                }
                updateButtons();
            });
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic,
                                          int status) {
            runOnUiThreadSafe(() -> appendLog("Write status=" + status));
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            handleNotify(characteristic.getValue());
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic,
                                            byte[] value) {
            handleNotify(value);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        buildUi();

        BluetoothManager manager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager == null ? null : manager.getAdapter();
        setStatus("Idle");
        updateButtons();
    }

    @Override
    protected void onDestroy() {
        stopScan();
        closeGatt();
        super.onDestroy();
    }

    private void buildUi() {
        int padding = dp(16);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("OSAIG BLE SDK Demo");
        title.setTextSize(22);
        title.setPadding(0, 0, 0, dp(12));
        root.addView(title);

        statusText = new TextView(this);
        statusText.setTextSize(16);
        statusText.setPadding(0, 0, 0, dp(12));
        root.addView(statusText);

        responseText = new TextView(this);
        responseText.setText("Last response: --");
        responseText.setTextSize(16);
        responseText.setPadding(0, 0, 0, dp(12));
        root.addView(responseText);

        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);

        scanButton = new Button(this);
        scanButton.setText("Scan OSAIG");
        scanButton.setOnClickListener(v -> startScan());
        buttons.addView(scanButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        sendButton = new Button(this);
        sendButton.setText("Send Ping");
        sendButton.setOnClickListener(v -> sendPing());
        buttons.addView(sendButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectButton = new Button(this);
        disconnectButton.setText("Disconnect");
        disconnectButton.setOnClickListener(v -> disconnect());
        buttons.addView(disconnectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(buttons);

        logText = new TextView(this);
        logText.setTextSize(13);
        logText.setTextIsSelectable(true);

        ScrollView scrollView = new ScrollView(this);
        scrollView.addView(logText);
        root.addView(scrollView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1));

        setContentView(root);
    }

    private void startScan() {
        if (bluetoothAdapter == null) {
            setStatus("Bluetooth is not available");
            appendLog("Bluetooth adapter unavailable");
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            setStatus("Turn on Bluetooth first");
            appendLog("Bluetooth is disabled");
            return;
        }

        if (!hasBlePermissions()) {
            requestBlePermissions();
            return;
        }

        stopScan();
        scanner = bluetoothAdapter.getBluetoothLeScanner();
        if (scanner == null) {
            setStatus("BLE scanner unavailable");
            appendLog("BLE scanner unavailable");
            return;
        }

        ScanFilter filter = new ScanFilter.Builder()
                .setServiceUuid(new ParcelUuid(OSAIG_SERVICE_UUID))
                .build();
        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();

        scanning = true;
        setStatus("Scanning for OSAIG-XXXX...");
        appendLog("Start scan");
        updateButtons();
        scanner.startScan(Collections.singletonList(filter), settings, scanCallback);

        mainHandler.postDelayed(() -> {
            if (scanning) {
                stopScan();
                setStatus("Scan timeout");
                appendLog("Scan timeout");
            }
        }, 12000);
    }

    private void stopScan() {
        if (!scanning) {
            return;
        }

        scanning = false;
        if (scanner != null && hasBlePermissions()) {
            scanner.stopScan(scanCallback);
        }
        appendLog("Stop scan");
        updateButtons();
    }

    private void connect(BluetoothDevice device) {
        closeGatt();
        setStatus("Connecting " + safeDeviceName(device) + "...");
        appendLog("Connect " + device.getAddress());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            gatt = device.connectGatt(this, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
        } else {
            gatt = device.connectGatt(this, false, gattCallback);
        }
        updateButtons();
    }

    private void disconnect() {
        stopScan();
        if (gatt != null) {
            appendLog("Disconnect requested");
            gatt.disconnect();
        } else {
            setStatus("Idle");
        }
        updateButtons();
    }

    private void closeGatt() {
        communicateCharacteristic = null;
        notifyEnabled = false;
        if (gatt != null) {
            gatt.close();
            gatt = null;
        }
    }

    private void enableNotify(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
        boolean localNotify = gatt.setCharacteristicNotification(characteristic, true);
        if (!localNotify) {
            runOnUiThreadSafe(() -> {
                setStatus("setCharacteristicNotification failed");
                appendLog("setCharacteristicNotification failed");
                updateButtons();
            });
            return;
        }

        BluetoothGattDescriptor descriptor =
                characteristic.getDescriptor(CLIENT_CHARACTERISTIC_CONFIG_UUID);
        if (descriptor == null) {
            runOnUiThreadSafe(() -> {
                setStatus("CCCD descriptor not found");
                appendLog("CCCD descriptor not found");
                updateButtons();
            });
            return;
        }

        byte[] cccdValue = chooseCccdValue(characteristic);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            int result = gatt.writeDescriptor(descriptor, cccdValue);
            if (result != 0) {
                runOnUiThreadSafe(() -> appendLog("writeDescriptor returned " + result));
            }
        } else {
            descriptor.setValue(cccdValue);
            if (!gatt.writeDescriptor(descriptor)) {
                runOnUiThreadSafe(() -> appendLog("writeDescriptor returned false"));
            }
        }
    }

    private void sendPing() {
        if (gatt == null || communicateCharacteristic == null) {
            setStatus("Connect first");
            return;
        }

        String json = "{\"datatype\":\"sdk.demo.ping\",\"data\":\"hello from android\"}";
        byte[] payload = json.getBytes(StandardCharsets.UTF_8);
        if (payload.length > BLE_PACKET_LIMIT) {
            appendLog("Payload too large: " + payload.length);
            return;
        }

        int writeType = chooseWriteType(communicateCharacteristic);
        appendLog("Send " + json);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            int result = gatt.writeCharacteristic(communicateCharacteristic, payload, writeType);
            appendLog("writeCharacteristic returned " + result);
        } else {
            communicateCharacteristic.setWriteType(writeType);
            communicateCharacteristic.setValue(payload);
            boolean accepted = gatt.writeCharacteristic(communicateCharacteristic);
            appendLog("writeCharacteristic accepted=" + accepted);
        }
    }

    private int chooseWriteType(BluetoothGattCharacteristic characteristic) {
        int properties = characteristic.getProperties();
        if ((properties & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) {
            return BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT;
        }
        return BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE;
    }

    private byte[] chooseCccdValue(BluetoothGattCharacteristic characteristic) {
        int properties = characteristic.getProperties();
        boolean canNotify = (properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0;
        boolean canIndicate = (properties & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0;
        if (canNotify) {
            appendLog("Enable CCCD for notify");
            return BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
        }
        if (canIndicate) {
            appendLog("Enable CCCD for indicate");
            return BluetoothGattDescriptor.ENABLE_INDICATION_VALUE;
        }
        appendLog("Enable CCCD for notify");
        return BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
    }

    private String formatProperties(BluetoothGattCharacteristic characteristic) {
        int properties = characteristic.getProperties();
        List<String> names = new ArrayList<>();
        if ((properties & BluetoothGattCharacteristic.PROPERTY_READ) != 0) {
            names.add("read");
        }
        if ((properties & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) {
            names.add("write");
        }
        if ((properties & BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0) {
            names.add("write-no-response");
        }
        if ((properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) {
            names.add("notify");
        }
        if ((properties & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0) {
            names.add("indicate");
        }
        return names.toString();
    }

    private void handleNotify(byte[] value) {
        String text = value == null ? "" : new String(value, StandardCharsets.UTF_8);
        runOnUiThreadSafe(() -> {
            appendLog("Notify " + text);
            updateResponse(text);
        });
    }

    private void updateResponse(String payload) {
        try {
            JSONObject object = new JSONObject(payload);
            String datatype = object.optString("datatype", "");
            String data = object.optString("data", "");
            if ("sdk.demo.pong".equals(datatype)) {
                responseText.setText("Last response: " + data);
            }
        } catch (JSONException ignored) {
            responseText.setText("Last response: " + payload);
        }
    }

    private String resolveDeviceName(ScanResult result) {
        if (result.getScanRecord() != null) {
            String scanName = result.getScanRecord().getDeviceName();
            if (!TextUtils.isEmpty(scanName)) {
                return scanName;
            }
        }
        return safeDeviceName(result.getDevice());
    }

    private String safeDeviceName(BluetoothDevice device) {
        String name = device.getName();
        return TextUtils.isEmpty(name) ? "Unnamed" : name;
    }

    private boolean isTargetName(String name) {
        return name != null && OSAIG_NAME_PATTERN.matcher(name.trim()).matches();
    }

    private boolean hasBlePermissions() {
        for (String permission : requiredPermissions()) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    private void requestBlePermissions() {
        List<String> permissions = requiredPermissions();
        requestPermissions(permissions.toArray(new String[0]), REQUEST_BLE_PERMISSIONS);
    }

    private List<String> requiredPermissions() {
        List<String> permissions = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN);
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT);
        } else {
            permissions.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        return permissions;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_BLE_PERMISSIONS) {
            return;
        }

        if (hasBlePermissions()) {
            appendLog("BLE permissions granted");
            startScan();
        } else {
            setStatus("BLE permissions denied");
            appendLog("BLE permissions denied");
        }
    }

    private void updateButtons() {
        boolean connected = gatt != null && communicateCharacteristic != null && notifyEnabled;
        scanButton.setEnabled(!scanning);
        sendButton.setEnabled(connected);
        disconnectButton.setEnabled(gatt != null || scanning);
    }

    private void setStatus(String status) {
        statusText.setText("Status: " + status);
    }

    private void appendLog(String message) {
        String line = timeFormat.format(new Date()) + "  " + message + "\n";
        logText.append(line);
    }

    private int dp(int value) {
        float density = getResources().getDisplayMetrics().density;
        return Math.round(value * density);
    }

    private void runOnUiThreadSafe(Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
        } else {
            runOnUiThread(runnable);
        }
    }
}
