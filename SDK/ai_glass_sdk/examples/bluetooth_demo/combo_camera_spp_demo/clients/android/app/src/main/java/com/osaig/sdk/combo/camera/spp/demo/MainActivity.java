package com.osaig.sdk.combo.camera.spp.demo;

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
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

@SuppressLint("MissingPermission")
public class MainActivity extends Activity {
    private static final String TAG = "ComboCameraSppDemo";
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1001;
    private static final UUID OSAIG_BLE_SERVICE_UUID =
            UUID.fromString("00001910-0000-1000-8000-00805f9b34fb");
    private static final UUID OSAIG_BLE_CHARACTERISTIC_UUID =
            UUID.fromString("dfd4416e-1810-47f7-8248-eb8be3dc47f9");
    private static final UUID CLIENT_CHARACTERISTIC_CONFIG_UUID =
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final UUID OSAIG_SPP_UUID =
            UUID.fromString("00001911-0000-1000-8000-00805f9b34fb");
    private static final int OSAIG_SPP_RFCOMM_CHANNEL = 10;
    private static final String BLE_COMMAND_DATATYPE = "combo.camera.takephoto";
    private static final String BLE_STATUS_DATATYPE = "combo.camera.status";
    private static final int BLE_PACKET_LIMIT = 180;
    private static final int DISCOVERY_TIMEOUT_MS = 15000;
    private static final int BLE_SPP_CANDIDATE_SCAN_MS = 6000;
    private static final int BLE_SCAN_TIMEOUT_MS = 12000;
    private static final int BLE_SCAN_MAX_ATTEMPTS = 4;
    private static final int BLE_GATT_MAX_ATTEMPTS = 3;
    private static final int MAX_HEADER_BYTES = 512;
    private static final int MAX_JPG_BYTES = 12 * 1024 * 1024;
    private static final int TRANSFER_PROGRESS_STEP_BYTES = 16 * 1024;
    private static final long TRANSFER_PROGRESS_STEP_MS = 250;

    private final ExecutorService executor = Executors.newCachedThreadPool();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final AtomicBoolean connectRunning = new AtomicBoolean(false);
    private final AtomicBoolean photoRunning = new AtomicBoolean(false);
    private final SimpleDateFormat timeFormat =
            new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic communicateCharacteristic;
    private BluetoothSocket sppSocket;
    private volatile boolean scanning;
    private volatile boolean bleReady;
    private volatile boolean sppReady;
    private volatile String activeRequestId;
    private volatile String targetDeviceAddress;
    private volatile String targetAddressOverride;
    private volatile BluetoothDevice selectedClassicDevice;
    private volatile String connectedSppAddress;
    private List<BluetoothDevice> connectionCandidates = Collections.emptyList();
    private int bleSkipCount;
    private int bleScanAttempts;
    private int bleGattConnectAttempts;

    private TextView statusText;
    private TextView logText;
    private TextView resultText;
    private TextView transferProgressText;
    private ProgressBar transferProgressBar;
    private ImageView imageView;
    private Button connectButton;
    private Button takePhotoButton;
    private Button disconnectButton;

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (!scanning) {
                return;
            }
            BluetoothDevice device = result.getDevice();
            String name = resolveBleDeviceName(result);
            String targetAddress = activeBleTargetAddress();
            if (targetAddress != null && !targetAddress.equals(device.getAddress())) {
                bleSkipCount++;
                if (bleSkipCount <= 8 || bleSkipCount % 100 == 0) {
                    appendLog("BLE skip " + name + " [" + device.getAddress()
                            + "], waiting " + targetAddress
                            + ", count=" + bleSkipCount);
                }
                return;
            }
            if (targetAddress == null && !isOsaigName(name)) {
                return;
            }

            runOnUiThreadSafe(() -> {
                appendLog("BLE found " + name + " [" + device.getAddress() + "]");
                stopBleScan();
                connectBle(device);
            });
        }

        @Override
        public void onScanFailed(int errorCode) {
            runOnUiThreadSafe(() -> {
                scanning = false;
                setStatus("BLE scan failed: " + errorCode);
                appendLog("BLE scan failed: " + errorCode);
                updateButtons();
            });
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                runOnUiThreadSafe(() -> {
                    setStatus("BLE connected, discovering services...");
                    appendLog("BLE GATT connected");
                    updateButtons();
                });
                gatt.discoverServices();
                return;
            }

            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                runOnUiThreadSafe(() -> {
                    boolean wasReady = bleReady;
                    bleReady = false;
                    communicateCharacteristic = null;
                    appendLog("BLE disconnected, status=" + status);
                    if (!wasReady && retryBleGatt("disconnect status=" + status)) {
                        updateButtons();
                        return;
                    }
                    if (!wasReady && connectRunning.get()) {
                        finishBleConnectFailure("disconnect status=" + status);
                        return;
                    }
                    setStatus("BLE disconnected");
                    closeGatt();
                    updateButtons();
                });
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                runOnUiThreadSafe(() -> {
                    appendLog("BLE service discovery failed: " + status);
                    if (retryBleGatt("service discovery status=" + status)) {
                        updateButtons();
                        return;
                    }
                    finishBleConnectFailure("service discovery status=" + status);
                    updateButtons();
                });
                return;
            }

            BluetoothGattService service = gatt.getService(OSAIG_BLE_SERVICE_UUID);
            BluetoothGattCharacteristic characteristic = service == null
                    ? null
                    : service.getCharacteristic(OSAIG_BLE_CHARACTERISTIC_UUID);

            if (characteristic == null) {
                runOnUiThreadSafe(() -> {
                    appendLog("Missing BLE characteristic " + OSAIG_BLE_CHARACTERISTIC_UUID);
                    if (retryBleGatt("missing characteristic")) {
                        updateButtons();
                        return;
                    }
                    finishBleConnectFailure("missing characteristic");
                    updateButtons();
                });
                return;
            }

            communicateCharacteristic = characteristic;
            runOnUiThreadSafe(() -> {
                setStatus("BLE characteristic found, enabling notify...");
                appendLog("BLE characteristic ready");
                updateButtons();
            });
            enableNotify(gatt, characteristic);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            runOnUiThreadSafe(() -> {
                if (CLIENT_CHARACTERISTIC_CONFIG_UUID.equals(descriptor.getUuid())
                        && status == BluetoothGatt.GATT_SUCCESS) {
                    bleReady = true;
                    connectRunning.set(false);
                    setStatus("BLE ready");
                    appendLog("BLE notify enabled");
                    if (!gatt.requestMtu(247)) {
                        appendLog("BLE requestMtu returned false");
                    }
                } else {
                    bleReady = false;
                    appendLog("BLE descriptor write status=" + status);
                    if (retryBleGatt("descriptor status=" + status)) {
                        updateButtons();
                        return;
                    }
                    finishBleConnectFailure("descriptor status=" + status);
                }
                updateButtons();
            });
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            runOnUiThreadSafe(() -> appendLog("BLE MTU changed mtu=" + mtu + " status=" + status));
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic,
                                          int status) {
            runOnUiThreadSafe(() -> appendLog("BLE write status=" + status));
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            handleBleNotify(characteristic.getValue());
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic,
                                            byte[] value) {
            handleBleNotify(value);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        buildUi();

        BluetoothManager manager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager == null ? null : manager.getAdapter();
        targetAddressOverride = normalizeAddress(getIntent().getStringExtra("target_address"));
        if (targetAddressOverride != null) {
            appendLog("Target address override " + targetAddressOverride);
        }
        setStatus("Idle");
        updateButtons();
    }

    @Override
    protected void onDestroy() {
        disconnectAll();
        executor.shutdownNow();
        super.onDestroy();
    }

    private void buildUi() {
        int padding = dp(16);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("OSAIG BLE + SPP Camera Demo");
        title.setTextSize(22);
        title.setPadding(0, 0, 0, dp(10));
        root.addView(title);

        statusText = new TextView(this);
        statusText.setTextSize(16);
        statusText.setPadding(0, 0, 0, dp(8));
        root.addView(statusText);

        resultText = new TextView(this);
        resultText.setText("Image: --");
        resultText.setTextSize(14);
        resultText.setPadding(0, 0, 0, dp(8));
        root.addView(resultText);

        transferProgressText = new TextView(this);
        transferProgressText.setText("Transfer: --");
        transferProgressText.setTextSize(14);
        transferProgressText.setPadding(0, 0, 0, dp(4));
        root.addView(transferProgressText);

        transferProgressBar = new ProgressBar(
                this, null, android.R.attr.progressBarStyleHorizontal);
        transferProgressBar.setMax(100);
        transferProgressBar.setProgress(0);
        transferProgressBar.setVisibility(View.GONE);
        root.addView(transferProgressBar, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(8)));

        imageView = new ImageView(this);
        imageView.setBackgroundColor(Color.rgb(238, 238, 238));
        imageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        root.addView(imageView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(260)));

        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);
        buttons.setPadding(0, dp(8), 0, dp(8));

        connectButton = new Button(this);
        connectButton.setText("Connect BLE + SPP");
        connectButton.setAllCaps(false);
        connectButton.setOnClickListener(v -> connectAll());
        buttons.addView(connectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        takePhotoButton = new Button(this);
        takePhotoButton.setText("Take Photo");
        takePhotoButton.setAllCaps(false);
        takePhotoButton.setOnClickListener(v -> takePhoto());
        buttons.addView(takePhotoButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectButton = new Button(this);
        disconnectButton.setText("Disconnect");
        disconnectButton.setAllCaps(false);
        disconnectButton.setOnClickListener(v -> disconnectAll());
        buttons.addView(disconnectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(buttons);

        Button clearButton = new Button(this);
        clearButton.setText("Clear Log");
        clearButton.setAllCaps(false);
        clearButton.setOnClickListener(v -> logText.setText(""));
        root.addView(clearButton, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(44)));

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

    private void connectAll() {
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
        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
            return;
        }
        if (!connectRunning.compareAndSet(false, true)) {
            appendLog("Connect already running");
            return;
        }

        stopBleScan();
        closeGatt();
        closeSppSocket();
        photoRunning.set(false);
        targetDeviceAddress = null;
        selectedClassicDevice = null;
        connectionCandidates = Collections.emptyList();
        bleScanAttempts = 0;
        bleGattConnectAttempts = 0;
        bleSkipCount = 0;
        imageView.setImageDrawable(null);
        resultText.setText("Image: --");
        resetTransferProgress();
        setStatus("Finding OSAIG device...");
        updateButtons();

        executor.execute(() -> {
            try {
                List<BluetoothDevice> candidates = discoverConnectionCandidates();
                if (candidates.isEmpty()) {
                    throw new IOException("no OSAIG/RK962 Bluetooth device");
                }
                connectionCandidates = candidates;
                connectFirstAvailableSppCandidate();
            } catch (Exception e) {
                runOnUiThreadSafe(() -> {
                    connectRunning.set(false);
                    setStatus("Device discovery failed");
                    appendLog("Device discovery failed: " + e.getClass().getSimpleName()
                            + ": " + e.getMessage());
                    updateButtons();
                });
            }
        });
    }

    private List<BluetoothDevice> discoverConnectionCandidates() {
        List<BluetoothDevice> candidates = discoverClassicCandidateDevices(bluetoothAdapter);
        for (BluetoothDevice discovered : discoverBleSppCandidateDevices(bluetoothAdapter)) {
            addUniqueDevice(candidates, discovered);
        }
        for (BluetoothDevice paired : findPairedClassicCandidateDevices(bluetoothAdapter)) {
            addUniqueDevice(candidates, paired);
        }

        return orderClassicCandidates(candidates);
    }

    private void connectSppSocket(BluetoothDevice device) throws IOException {
        appendLog("SPP connecting " + describe(device));
        IOException directError = null;
        BluetoothSocket socket = null;
        try {
            socket = createDirectRfcommSocket(device);
            socket.connect();
        } catch (IOException e) {
            directError = e;
            closeSocketQuietly(socket);
            appendLog("SPP direct channel failed: " + e.getMessage());
            socket = device.createInsecureRfcommSocketToServiceRecord(OSAIG_SPP_UUID);
            try {
                socket.connect();
            } catch (IOException uuidError) {
                closeSocketQuietly(socket);
                throw new IOException("direct channel failed: " + directError.getMessage()
                        + "; uuid connect failed: " + uuidError.getMessage(), uuidError);
            }
        }
        sppSocket = socket;
        connectedSppAddress = device.getAddress();
        sppReady = true;
        appendLog("SPP connected");
    }

    private BluetoothSocket createDirectRfcommSocket(BluetoothDevice device) throws IOException {
        try {
            Method method = device.getClass()
                    .getMethod("createInsecureRfcommSocket", int.class);
            Object socket = method.invoke(device, OSAIG_SPP_RFCOMM_CHANNEL);
            if (socket instanceof BluetoothSocket) {
                appendLog("SPP using RFCOMM channel " + OSAIG_SPP_RFCOMM_CHANNEL);
                return (BluetoothSocket) socket;
            }
            throw new IOException("createInsecureRfcommSocket returned "
                    + (socket == null ? "null" : socket.getClass().getName()));
        } catch (ReflectiveOperationException e) {
            throw new IOException("createInsecureRfcommSocket reflection failed", e);
        }
    }

    private void closeSocketQuietly(BluetoothSocket socket) {
        if (socket == null) {
            return;
        }
        try {
            socket.close();
        } catch (IOException ignored) {
            // Best-effort cleanup for a failed connection attempt.
        }
    }

    private void connectFirstAvailableSppCandidate() throws IOException {
        IOException lastError = null;
        for (int i = 0; i < connectionCandidates.size(); i++) {
            BluetoothDevice target = connectionCandidates.get(i);
            selectedClassicDevice = target;
            targetDeviceAddress = target.getAddress();
            try {
                appendLog((i == 0 ? "Selected target " : "Trying next target ")
                        + describe(target));
                connectSppSocket(target);
                runOnUiThreadSafe(() -> {
                    bleScanAttempts = 0;
                    bleGattConnectAttempts = 0;
                    bleSkipCount = 0;
                    setStatus("SPP ready, connecting BLE...");
                    updateButtons();
                    startBleScan();
                });
                return;
            } catch (IOException e) {
                lastError = e;
                appendLog("Candidate SPP failed " + describe(target) + ": " + e.getMessage());
                closeSppSocket();
            }
        }
        throw lastError == null ? new IOException("no SPP candidate") : lastError;
    }

    private List<BluetoothDevice> discoverClassicCandidateDevices(BluetoothAdapter adapter) {
        List<BluetoothDevice> devices = Collections.synchronizedList(new ArrayList<>());
        CountDownLatch latch = new CountDownLatch(1);
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);

        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    if (device == null) {
                        return;
                    }
                    String name = safeDeviceName(device);
                    appendLog("Classic found " + name + " " + device.getAddress());
                    if (isClassicCandidateName(name) && !containsDevice(devices, device)) {
                        devices.add(device);
                        appendLog("Classic candidate " + describe(device));
                    }
                    return;
                }
                if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                    appendLog("Classic discovery finished");
                    latch.countDown();
                }
            }
        };

        registerReceiverCompat(receiver, filter);
        try {
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
            appendLog("Classic discovery start");
            if (!adapter.startDiscovery()) {
                appendLog("Classic startDiscovery returned false");
                return new ArrayList<>(devices);
            }
            try {
                latch.await(DISCOVERY_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            return new ArrayList<>(devices);
        } finally {
            try {
                unregisterReceiver(receiver);
            } catch (IllegalArgumentException ignored) {
                // Receiver may already be unregistered during Activity shutdown.
            }
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
        }
    }

    private List<BluetoothDevice> discoverBleSppCandidateDevices(BluetoothAdapter adapter) {
        List<BluetoothDevice> devices = Collections.synchronizedList(new ArrayList<>());
        BluetoothLeScanner leScanner = adapter.getBluetoothLeScanner();
        CountDownLatch latch = new CountDownLatch(1);

        if (leScanner == null) {
            appendLog("BLE pre-scan unavailable");
            return new ArrayList<>();
        }

        ScanFilter filter = new ScanFilter.Builder()
                .setServiceUuid(new ParcelUuid(OSAIG_BLE_SERVICE_UUID))
                .build();
        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();
        ScanCallback callback = new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                BluetoothDevice device = result.getDevice();
                String name = resolveBleDeviceName(result);
                if (isClassicCandidateName(name) && addUniqueDevice(devices, device)) {
                    appendLog("BLE SPP candidate " + name + " " + device.getAddress());
                }
            }

            @Override
            public void onScanFailed(int errorCode) {
                appendLog("BLE pre-scan failed: " + errorCode);
                latch.countDown();
            }
        };

        appendLog("BLE pre-scan start for SPP candidates");
        try {
            leScanner.startScan(Collections.singletonList(filter), settings, callback);
            try {
                latch.await(BLE_SPP_CANDIDATE_SCAN_MS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            appendLog("BLE pre-scan candidates=" + devices.size());
            return new ArrayList<>(devices);
        } finally {
            leScanner.stopScan(callback);
        }
    }

    private boolean containsDevice(List<BluetoothDevice> devices, BluetoothDevice device) {
        for (BluetoothDevice candidate : devices) {
            if (candidate.getAddress().equals(device.getAddress())) {
                return true;
            }
        }
        return false;
    }

    private boolean addUniqueDevice(List<BluetoothDevice> devices, BluetoothDevice device) {
        if (containsDevice(devices, device)) {
            return false;
        }
        devices.add(device);
        return true;
    }

    private List<BluetoothDevice> findPairedClassicCandidateDevices(BluetoothAdapter adapter) {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        List<BluetoothDevice> result = new ArrayList<>();
        appendLog("Classic bonded devices=" + bondedDevices.size());
        for (BluetoothDevice device : bondedDevices) {
            String name = safeDeviceName(device);
            appendLog("Classic bonded " + name + " " + device.getAddress());
            if (isClassicCandidateName(name)) {
                result.add(device);
            }
        }
        return result;
    }

    private List<BluetoothDevice> orderClassicCandidates(List<BluetoothDevice> devices) {
        List<BluetoothDevice> ordered = new ArrayList<>();
        appendClassicCandidatesByRank(devices, ordered, 0);
        appendClassicCandidatesByRank(devices, ordered, 1);
        appendClassicCandidatesByRank(devices, ordered, 2);
        ordered = applyTargetAddressOverride(ordered);
        appendLog("Classic ordered candidates=" + ordered.size());
        return ordered;
    }

    private List<BluetoothDevice> applyTargetAddressOverride(List<BluetoothDevice> devices) {
        String override = targetAddressOverride;
        if (override == null) {
            return devices;
        }

        List<BluetoothDevice> result = new ArrayList<>();
        for (BluetoothDevice device : devices) {
            if (override.equals(normalizeAddress(device.getAddress()))) {
                result.add(device);
                appendLog("Classic target override matched " + describe(device));
            }
        }
        if (!result.isEmpty()) {
            return result;
        }
        appendLog("Classic target override not found " + override + ", using scanned candidates");
        for (BluetoothDevice device : devices) {
            if (!containsDevice(result, device)) {
                result.add(device);
            }
        }
        return result;
    }

    private void appendClassicCandidatesByRank(List<BluetoothDevice> source,
                                               List<BluetoothDevice> target,
                                               int rank) {
        for (BluetoothDevice device : source) {
            if (classicCandidateRank(device) == rank && !containsDevice(target, device)) {
                target.add(device);
                appendLog("Classic ordered " + describe(device));
            }
        }
    }

    private int classicCandidateRank(BluetoothDevice device) {
        String name = safeDeviceName(device);
        if (isLegacyClassicName(name)) {
            return 0;
        }
        if (isOsaigName(name)) {
            return 1;
        }
        return 2;
    }

    private void startBleScan() {
        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
            return;
        }
        stopBleScan();
        scanner = bluetoothAdapter.getBluetoothLeScanner();
        if (scanner == null) {
            setStatus("BLE scanner unavailable");
            appendLog("BLE scanner unavailable");
            return;
        }

        bleScanAttempts++;
        boolean useServiceFilter = bleScanAttempts == 1;
        String targetAddress = activeBleTargetAddress();
        List<ScanFilter> filters = null;
        if (useServiceFilter) {
            ScanFilter filter = new ScanFilter.Builder()
                    .setServiceUuid(new ParcelUuid(OSAIG_BLE_SERVICE_UUID))
                    .build();
            filters = Collections.singletonList(filter);
        }
        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();

        scanning = true;
        bleSkipCount = 0;
        setStatus("Scanning BLE " + (targetAddress == null ? "OSAIG" : targetAddress)
                + " (" + bleScanAttempts + "/" + BLE_SCAN_MAX_ATTEMPTS + ")");
        appendLog("BLE scan start attempt=" + bleScanAttempts
                + " filter=" + (useServiceFilter ? "service" : "address"));
        updateButtons();
        scanner.startScan(filters, settings, scanCallback);

        int attempt = bleScanAttempts;
        mainHandler.postDelayed(() -> {
            if (!scanning || attempt != bleScanAttempts) {
                return;
            }
            stopBleScan();
            String activeTarget = activeBleTargetAddress();
            if (!bleReady && activeTarget != null
                    && bleScanAttempts < BLE_SCAN_MAX_ATTEMPTS) {
                appendLog("BLE retry " + (bleScanAttempts + 1)
                        + "/" + BLE_SCAN_MAX_ATTEMPTS);
                startBleScan();
                return;
            }
            if (!bleReady && activeTarget != null) {
                appendLog("BLE direct connect fallback " + activeTarget);
                connectBle(bluetoothAdapter.getRemoteDevice(activeTarget));
                return;
            }
            if (connectRunning.get()) {
                finishBleConnectFailure("scan timeout");
                return;
            }
            setStatus(sppReady ? "SPP ready, BLE scan timeout" : "BLE scan timeout");
            appendLog("BLE scan timeout");
        }, BLE_SCAN_TIMEOUT_MS);
    }

    private void stopBleScan() {
        if (!scanning) {
            return;
        }
        scanning = false;
        if (scanner != null && hasBluetoothPermissions()) {
            scanner.stopScan(scanCallback);
        }
        appendLog("BLE scan stop");
        updateButtons();
    }

    private void connectBle(BluetoothDevice device) {
        closeGatt();
        bleGattConnectAttempts++;
        setStatus("BLE connecting " + safeDeviceName(device) + "...");
        appendLog("BLE connect " + device.getAddress()
                + " attempt=" + bleGattConnectAttempts + "/" + BLE_GATT_MAX_ATTEMPTS);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            gatt = device.connectGatt(this, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
        } else {
            gatt = device.connectGatt(this, false, gattCallback);
        }
        updateButtons();
    }

    private boolean retryBleGatt(String reason) {
        String address = activeBleTargetAddress();
        if (address == null || bleReady
                || bleGattConnectAttempts >= BLE_GATT_MAX_ATTEMPTS) {
            return false;
        }
        appendLog("BLE GATT retry " + (bleGattConnectAttempts + 1)
                + "/" + BLE_GATT_MAX_ATTEMPTS + " reason=" + reason);
        closeGatt();
        mainHandler.postDelayed(() ->
                connectBle(bluetoothAdapter.getRemoteDevice(address)), 1500);
        return true;
    }

    private String activeBleTargetAddress() {
        return targetDeviceAddress != null ? targetDeviceAddress : connectedSppAddress;
    }

    private void finishBleConnectFailure(String reason) {
        appendLog("BLE connect failed: " + reason);
        connectRunning.set(false);
        closeGatt();
        setStatus("BLE connect failed");
        updateButtons();
    }

    private void enableNotify(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
        if (!gatt.setCharacteristicNotification(characteristic, true)) {
            runOnUiThreadSafe(() -> {
                setStatus("BLE setCharacteristicNotification failed");
                appendLog("BLE setCharacteristicNotification failed");
                updateButtons();
            });
            return;
        }

        BluetoothGattDescriptor descriptor =
                characteristic.getDescriptor(CLIENT_CHARACTERISTIC_CONFIG_UUID);
        if (descriptor == null) {
            runOnUiThreadSafe(() -> {
                setStatus("BLE CCCD descriptor not found");
                appendLog("BLE CCCD descriptor not found");
                updateButtons();
            });
            return;
        }

        byte[] cccdValue = chooseCccdValue(characteristic);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            int result = gatt.writeDescriptor(descriptor, cccdValue);
            if (result != 0) {
                runOnUiThreadSafe(() -> appendLog("BLE writeDescriptor returned " + result));
            }
        } else {
            descriptor.setValue(cccdValue);
            if (!gatt.writeDescriptor(descriptor)) {
                runOnUiThreadSafe(() -> appendLog("BLE writeDescriptor returned false"));
            }
        }
    }

    private void takePhoto() {
        if (!bleReady || communicateCharacteristic == null || !sppReady || sppSocket == null) {
            setStatus("Connect BLE and SPP first");
            return;
        }
        if (!photoRunning.compareAndSet(false, true)) {
            appendLog("Photo request already running");
            return;
        }

        String requestId = "req_" + System.currentTimeMillis();
        activeRequestId = requestId;
        resultText.setText("Image: waiting for " + requestId);
        setTransferProgress(0, 0, "Waiting for SPP image header");
        setStatus("Taking photo...");
        updateButtons();

        executor.execute(() -> receivePhoto(requestId));
        if (!sendBleTakePhoto(requestId)) {
            photoRunning.set(false);
            activeRequestId = null;
            setStatus("BLE write failed");
            updateButtons();
            return;
        }
    }

    private boolean sendBleTakePhoto(String requestId) {
        if (gatt == null || communicateCharacteristic == null) {
            return false;
        }

        String json = "{\"datatype\":\"" + BLE_COMMAND_DATATYPE
                + "\",\"data\":\"" + requestId + "\"}";
        byte[] payload = json.getBytes(StandardCharsets.UTF_8);
        if (payload.length > BLE_PACKET_LIMIT) {
            appendLog("BLE payload too large: " + payload.length);
            return false;
        }

        int writeType = chooseWriteType(communicateCharacteristic);
        appendLog("BLE TX " + json);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            int result = gatt.writeCharacteristic(communicateCharacteristic, payload, writeType);
            appendLog("BLE writeCharacteristic returned " + result);
            return result == 0;
        }

        communicateCharacteristic.setWriteType(writeType);
        communicateCharacteristic.setValue(payload);
        boolean accepted = gatt.writeCharacteristic(communicateCharacteristic);
        appendLog("BLE writeCharacteristic accepted=" + accepted);
        return accepted;
    }

    private void receivePhoto(String requestId) {
        try {
            InputStream inputStream = sppSocket.getInputStream();
            appendLog("SPP waiting image header for " + requestId);
            String header = readAsciiLine(inputStream);
            appendLog("SPP header " + header);

            Map<String, String> values = parseImageHeader(header);
            int size = parsePositiveSize(values.get("size"));
            String filename = values.get("name");
            if (size <= 0 || size > MAX_JPG_BYTES) {
                throw new IOException("invalid JPG size " + size);
            }

            setTransferProgress(0, size, "Receiving " + filename);
            byte[] jpg = readExact(inputStream, size, filename);
            Bitmap bitmap = BitmapFactory.decodeByteArray(jpg, 0, jpg.length);
            if (bitmap == null) {
                throw new IOException("BitmapFactory returned null");
            }

            runOnUiThreadSafe(() -> {
                imageView.setImageBitmap(bitmap);
                resultText.setText("Image: " + filename + " (" + jpg.length + " bytes)");
                setTransferProgress(jpg.length, jpg.length, "Received " + filename);
                setStatus("Image received");
                appendLog("SPP image decoded bytes=" + jpg.length);
            });
        } catch (Exception e) {
            runOnUiThreadSafe(() -> {
                setStatus("Receive failed");
                transferProgressText.setText("Transfer: failed");
                appendLog("SPP receive failed: " + e.getClass().getSimpleName()
                        + ": " + e.getMessage());
                closeSppSocket();
            });
        } finally {
            photoRunning.set(false);
            activeRequestId = null;
            runOnUiThreadSafe(this::updateButtons);
        }
    }

    private String readAsciiLine(InputStream inputStream) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        while (out.size() < MAX_HEADER_BYTES) {
            int value = inputStream.read();
            if (value < 0) {
                throw new IOException("SPP EOF before header");
            }
            if (value == '\n') {
                return out.toString(StandardCharsets.UTF_8.name()).trim();
            }
            if (value != '\r') {
                out.write(value);
            }
        }
        throw new IOException("header too long");
    }

    private Map<String, String> parseImageHeader(String header) throws IOException {
        String[] parts = header.split(" ");
        if (parts.length < 3 || !"OSAIG_JPG_V1".equals(parts[0])) {
            throw new IOException("unsupported header: " + header);
        }

        Map<String, String> values = new HashMap<>();
        for (int i = 1; i < parts.length; i++) {
            int eq = parts[i].indexOf('=');
            if (eq <= 0 || eq + 1 >= parts[i].length()) {
                continue;
            }
            values.put(parts[i].substring(0, eq), parts[i].substring(eq + 1));
        }
        if (!values.containsKey("request") || !values.containsKey("size")
                || !values.containsKey("name")) {
            throw new IOException("missing header fields: " + header);
        }
        appendLog("SPP image request=" + values.get("request")
                + " size=" + values.get("size")
                + " name=" + values.get("name"));
        return values;
    }

    private int parsePositiveSize(String text) {
        try {
            return Integer.parseInt(text);
        } catch (NumberFormatException e) {
            return -1;
        }
    }

    private byte[] readExact(InputStream inputStream, int size, String filename) throws IOException {
        byte[] data = new byte[size];
        int offset = 0;
        int nextProgressLogOffset = TRANSFER_PROGRESS_STEP_BYTES;
        long lastProgressMs = 0;
        setTransferProgress(0, size, "Receiving " + filename);
        while (offset < size) {
            int nread = inputStream.read(data, offset, size - offset);
            if (nread < 0) {
                throw new IOException("SPP EOF at " + offset + "/" + size);
            }
            offset += nread;
            long now = System.currentTimeMillis();
            boolean shouldUpdate = offset >= nextProgressLogOffset
                    || now - lastProgressMs >= TRANSFER_PROGRESS_STEP_MS
                    || offset == size;
            if (shouldUpdate) {
                setTransferProgress(offset, size, "Receiving " + filename);
                appendLog("SPP receive progress " + offset + "/" + size
                        + " (" + percent(offset, size) + "%)");
                while (nextProgressLogOffset <= offset) {
                    nextProgressLogOffset += TRANSFER_PROGRESS_STEP_BYTES;
                }
                lastProgressMs = now;
            }
        }
        return data;
    }

    private void resetTransferProgress() {
        transferProgressText.setText("Transfer: --");
        transferProgressBar.setProgress(0);
        transferProgressBar.setVisibility(View.GONE);
    }

    private void setTransferProgress(int transferred, int total, String label) {
        runOnUiThreadSafe(() -> {
            int progress = total > 0 ? percent(transferred, total) : 0;
            transferProgressBar.setVisibility(View.VISIBLE);
            transferProgressBar.setProgress(progress);
            StringBuilder text = new StringBuilder();
            text.append("Transfer: ").append(label == null ? "" : label);
            if (total > 0) {
                text.append("  ").append(progress).append("%  ")
                        .append(formatBytes(transferred)).append(" / ")
                        .append(formatBytes(total));
            }
            transferProgressText.setText(text.toString());
        });
    }

    private int percent(int transferred, int total) {
        if (total <= 0) {
            return 0;
        }
        long scaled = (long) transferred * 100L / (long) total;
        if (scaled < 0) {
            return 0;
        }
        if (scaled > 100) {
            return 100;
        }
        return (int) scaled;
    }

    private String formatBytes(int bytes) {
        if (bytes >= 1024 * 1024) {
            return String.format(Locale.US, "%.1f MiB", bytes / (1024.0 * 1024.0));
        }
        if (bytes >= 1024) {
            return String.format(Locale.US, "%.1f KiB", bytes / 1024.0);
        }
        return bytes + " B";
    }

    private void handleBleNotify(byte[] value) {
        String text = value == null ? "" : new String(value, StandardCharsets.UTF_8);
        runOnUiThreadSafe(() -> {
            appendLog("BLE Notify " + text);
            try {
                JSONObject object = new JSONObject(text);
                String datatype = object.optString("datatype", "");
                String data = object.optString("data", "");
                if (BLE_STATUS_DATATYPE.equals(datatype)) {
                    handleComboStatus(data);
                }
            } catch (JSONException ignored) {
                appendLog("BLE notify is not JSON");
            }
        });
    }

    private void handleComboStatus(String data) {
        setStatus("Camera status: " + data);
        if (data.startsWith("err|") || data.startsWith("busy|")) {
            if (activeRequestId != null && data.contains("|" + activeRequestId)) {
                appendLog("Camera request failed, closing SPP wait");
                photoRunning.set(false);
                closeSppSocket();
                updateButtons();
            }
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
        if ((properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) {
            return BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
        }
        if ((properties & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0) {
            return BluetoothGattDescriptor.ENABLE_INDICATION_VALUE;
        }
        return BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
    }

    private void disconnectAll() {
        stopBleScan();
        closeGatt();
        closeSppSocket();
        targetDeviceAddress = null;
        selectedClassicDevice = null;
        connectionCandidates = Collections.emptyList();
        connectRunning.set(false);
        photoRunning.set(false);
        setStatus("Disconnected");
        updateButtons();
    }

    private void closeGatt() {
        bleReady = false;
        communicateCharacteristic = null;
        if (gatt != null) {
            gatt.close();
            gatt = null;
        }
    }

    private synchronized void closeSppSocket() {
        sppReady = false;
        connectedSppAddress = null;
        if (sppSocket != null) {
            try {
                sppSocket.close();
            } catch (IOException ignored) {
                // Best-effort cleanup for a demo client.
            }
            sppSocket = null;
        }
    }

    private boolean hasBluetoothPermissions() {
        for (String permission : requiredPermissions()) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    private void requestBluetoothPermissions() {
        List<String> permissions = requiredPermissions();
        requestPermissions(permissions.toArray(new String[0]), REQUEST_BLUETOOTH_PERMISSIONS);
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
        if (requestCode != REQUEST_BLUETOOTH_PERMISSIONS) {
            return;
        }
        if (hasBluetoothPermissions()) {
            appendLog("Bluetooth permissions granted");
            connectAll();
        } else {
            setStatus("Bluetooth permissions denied");
            appendLog("Bluetooth permissions denied");
        }
    }

    private void registerReceiverCompat(BroadcastReceiver receiver, IntentFilter filter) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(receiver, filter);
        }
    }

    private String resolveBleDeviceName(ScanResult result) {
        if (result.getScanRecord() != null) {
            String scanName = result.getScanRecord().getDeviceName();
            if (!TextUtils.isEmpty(scanName)) {
                return scanName;
            }
        }
        return safeDeviceName(result.getDevice());
    }

    private boolean isOsaigName(String name) {
        return name != null && (name.startsWith("OSAIG-") || name.equals("OSAIG"));
    }

    private boolean isClassicCandidateName(String name) {
        return isOsaigName(name) || isLegacyClassicName(name);
    }

    private boolean isLegacyClassicName(String name) {
        return name != null && name.startsWith("RK962");
    }

    private String normalizeAddress(String address) {
        if (TextUtils.isEmpty(address)) {
            return null;
        }
        return address.trim().toUpperCase(Locale.US);
    }

    private String describe(BluetoothDevice device) {
        return safeDeviceName(device) + " " + device.getAddress();
    }

    private String safeDeviceName(BluetoothDevice device) {
        String name = device.getName();
        return TextUtils.isEmpty(name) ? "Unnamed" : name;
    }

    private void updateButtons() {
        boolean ready = bleReady && sppReady && !photoRunning.get();
        connectButton.setEnabled(!connectRunning.get() && !photoRunning.get() && !scanning);
        takePhotoButton.setEnabled(ready);
        disconnectButton.setEnabled(connectRunning.get() || bleReady || sppReady || scanning);
    }

    private void setStatus(String status) {
        statusText.setText("Status: " + status);
    }

    private void appendLog(String message) {
        String line = timeFormat.format(new Date()) + "  " + message + "\n";
        Log.d(TAG, message);
        runOnUiThreadSafe(() -> logText.append(line));
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
