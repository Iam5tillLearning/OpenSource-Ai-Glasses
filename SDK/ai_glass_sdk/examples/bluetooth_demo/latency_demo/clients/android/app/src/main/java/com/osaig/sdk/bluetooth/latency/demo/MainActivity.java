package com.osaig.sdk.bluetooth.latency.demo;

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
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.util.regex.Pattern;

@SuppressLint("MissingPermission")
public class MainActivity extends Activity {
    private static final String TAG = "OSAIG_BT_LATENCY";
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1001;
    private static final UUID OSAIG_SERVICE_UUID =
            UUID.fromString("00001910-0000-1000-8000-00805f9b34fb");
    private static final UUID OSAIG_CHARACTERISTIC_UUID =
            UUID.fromString("dfd4416e-1810-47f7-8248-eb8be3dc47f9");
    private static final UUID CLIENT_CHARACTERISTIC_CONFIG_UUID =
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final UUID SPP_UUID =
            UUID.fromString("00001911-0000-1000-8000-00805f9b34fb");
    private static final Pattern OSAIG_NAME_PATTERN = Pattern.compile("^OSAIG-[0-9A-F]{4}$");
    private static final int BLE_PACKET_LIMIT = 180;
    private static final int SAMPLE_COUNT = 50;
    private static final int RTT_TIMEOUT_MS = 2000;
    private static final int BLE_SAMPLE_GAP_MS = 120;
    private static final int SPP_SAMPLE_GAP_MS = 80;
    private static final int SPP_CHANNEL = 10;
    private static final long DISCOVERY_TIMEOUT_MS = 15000;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final AtomicBoolean sppTestRunning = new AtomicBoolean(false);
    private final LinkedBlockingQueue<SppLine> sppLines = new LinkedBlockingQueue<>();
    private final SimpleDateFormat timeFormat =
            new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic communicateCharacteristic;
    private BluetoothSocket sppSocket;
    private String targetAddress;
    private boolean scanning;
    private boolean notifyEnabled;

    private boolean bleLatencyRunning;
    private int bleTotal;
    private int bleSeq;
    private int bleWaitingSeq = -1;
    private long bleWaitingSendMs = -1;
    private int bleLost;
    private final List<Long> bleSamples = new ArrayList<>();

    private TextView statusText;
    private TextView summaryText;
    private TextView logText;
    private Button connectBleButton;
    private Button runBleButton;
    private Button runSppButton;
    private Button disconnectButton;

    private static final class SppLine {
        final String text;
        final long rxMs;

        SppLine(String text, long rxMs) {
            this.text = text;
            this.rxMs = rxMs;
        }
    }

    private static final class SppWaitResult {
        final long rttMs;
        final long rxMs;
        final long matchDelayMs;

        SppWaitResult(long rttMs, long rxMs, long matchDelayMs) {
            this.rttMs = rttMs;
            this.rxMs = rxMs;
            this.matchDelayMs = matchDelayMs;
        }
    }

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            String name = resolveDeviceName(result);
            if (!isTargetDevice(name, device.getAddress())) {
                return;
            }

            runOnUiThreadSafe(() -> {
                appendLog("BLE found " + name + " [" + device.getAddress() + "]");
                stopScan();
                connectBle(device);
            });
        }

        @Override
        public void onScanFailed(int errorCode) {
            runOnUiThreadSafe(() -> {
                scanning = false;
                updateButtons();
                setStatus("BLE scan failed: " + errorCode);
                appendLog("BLE scan failed error=" + errorCode);
            });
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                runOnUiThreadSafe(() -> {
                    setStatus("BLE connected. Discovering services...");
                    appendLog("BLE GATT connected");
                    updateButtons();
                });
                gatt.discoverServices();
                return;
            }

            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                runOnUiThreadSafe(() -> {
                    stopBleLatency("BLE disconnected");
                    communicateCharacteristic = null;
                    notifyEnabled = false;
                    setStatus("BLE disconnected");
                    appendLog("BLE GATT disconnected status=" + status);
                    closeGatt();
                    updateButtons();
                });
            }
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            runOnUiThreadSafe(() -> {
                appendLog("BLE MTU changed mtu=" + mtu + " status=" + status);
                setStatus("BLE ready");
                updateButtons();
            });
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                runOnUiThreadSafe(() -> {
                    setStatus("BLE service discovery failed: " + status);
                    appendLog("BLE service discovery failed status=" + status);
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
                    appendLog("BLE missing characteristic " + OSAIG_CHARACTERISTIC_UUID);
                    updateButtons();
                });
                return;
            }

            communicateCharacteristic = characteristic;
            runOnUiThreadSafe(() -> {
                setStatus("BLE characteristic ready. Enabling notify...");
                appendLog("BLE characteristic properties=" + formatProperties(characteristic));
                updateButtons();
            });
            enableNotify(gatt, characteristic);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor,
                                      int status) {
            runOnUiThreadSafe(() -> {
                if (CLIENT_CHARACTERISTIC_CONFIG_UUID.equals(descriptor.getUuid())
                        && status == BluetoothGatt.GATT_SUCCESS) {
                    notifyEnabled = true;
                    appendLog("BLE notify enabled");
                    setStatus("BLE ready");
                    if (!gatt.requestMtu(247)) {
                        appendLog("BLE requestMtu returned false");
                    }
                } else {
                    notifyEnabled = false;
                    setStatus("BLE notify setup failed: " + status);
                    appendLog("BLE descriptor write status=" + status);
                }
                updateButtons();
            });
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic,
                                          int status) {
            handleBleWriteCallback(status);
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
        targetAddress = normalizeAddress(getIntent().getStringExtra("target_address"));
        buildUi();

        BluetoothManager manager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager == null ? null : manager.getAdapter();
        setStatus("Idle" + (targetAddress == null ? "" : ", target=" + targetAddress));
        updateButtons();
    }

    @Override
    protected void onDestroy() {
        stopScan();
        closeGatt();
        closeSppSocket();
        executor.shutdownNow();
        super.onDestroy();
    }

    private void buildUi() {
        int padding = dp(16);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("OSAIG BT Latency Demo");
        title.setTextSize(22);
        title.setPadding(0, 0, 0, dp(12));
        root.addView(title);

        statusText = new TextView(this);
        statusText.setTextSize(16);
        statusText.setPadding(0, 0, 0, dp(8));
        root.addView(statusText);

        summaryText = new TextView(this);
        summaryText.setText("Result: --");
        summaryText.setTextSize(15);
        summaryText.setPadding(0, 0, 0, dp(12));
        root.addView(summaryText);

        LinearLayout row1 = new LinearLayout(this);
        row1.setOrientation(LinearLayout.HORIZONTAL);

        connectBleButton = new Button(this);
        connectBleButton.setText("Connect BLE");
        connectBleButton.setAllCaps(false);
        connectBleButton.setOnClickListener(v -> startBleScan());
        row1.addView(connectBleButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        runBleButton = new Button(this);
        runBleButton.setText("Run BLE RTT");
        runBleButton.setAllCaps(false);
        runBleButton.setOnClickListener(v -> startBleLatencyTest());
        row1.addView(runBleButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(row1);

        LinearLayout row2 = new LinearLayout(this);
        row2.setOrientation(LinearLayout.HORIZONTAL);

        runSppButton = new Button(this);
        runSppButton.setText("Run SPP RTT");
        runSppButton.setAllCaps(false);
        runSppButton.setOnClickListener(v -> runSppLatencyTest());
        row2.addView(runSppButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectButton = new Button(this);
        disconnectButton.setText("Disconnect");
        disconnectButton.setAllCaps(false);
        disconnectButton.setOnClickListener(v -> disconnectAll());
        row2.addView(disconnectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(row2);

        Button clearButton = new Button(this);
        clearButton.setText("Clear Log");
        clearButton.setAllCaps(false);
        clearButton.setOnClickListener(v -> logText.setText(""));
        root.addView(clearButton, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(48)));

        logText = new TextView(this);
        logText.setTextSize(13);
        logText.setTextIsSelectable(true);

        ScrollView scrollView = new ScrollView(this);
        scrollView.addView(logText);
        root.addView(scrollView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1));

        setContentView(root);
    }

    private void startBleScan() {
        if (!ensureBluetoothReady()) {
            return;
        }
        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
            return;
        }

        stopScan();
        closeGatt();
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
        setStatus("Scanning BLE OSAIG...");
        appendLog("BLE scan start target=" + (targetAddress == null ? "*" : targetAddress));
        updateButtons();
        scanner.startScan(Collections.singletonList(filter), settings, scanCallback);

        mainHandler.postDelayed(() -> {
            if (scanning) {
                stopScan();
                setStatus("BLE scan timeout");
                appendLog("BLE scan timeout");
            }
        }, DISCOVERY_TIMEOUT_MS);
    }

    private void stopScan() {
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
        setStatus("BLE connecting " + safeDeviceName(device) + "...");
        appendLog("BLE connect address=" + device.getAddress());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            gatt = device.connectGatt(this, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
        } else {
            gatt = device.connectGatt(this, false, gattCallback);
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
                setStatus("BLE set notification failed");
                appendLog("BLE setCharacteristicNotification failed");
                updateButtons();
            });
            return;
        }

        BluetoothGattDescriptor descriptor =
                characteristic.getDescriptor(CLIENT_CHARACTERISTIC_CONFIG_UUID);
        if (descriptor == null) {
            runOnUiThreadSafe(() -> {
                setStatus("BLE CCCD not found");
                appendLog("BLE CCCD descriptor not found");
                updateButtons();
            });
            return;
        }

        byte[] value = chooseCccdValue(characteristic);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            int result = gatt.writeDescriptor(descriptor, value);
            if (result != 0) {
                runOnUiThreadSafe(() -> appendLog("BLE writeDescriptor returned " + result));
            }
        } else {
            descriptor.setValue(value);
            if (!gatt.writeDescriptor(descriptor)) {
                runOnUiThreadSafe(() -> appendLog("BLE writeDescriptor returned false"));
            }
        }
    }

    private void startBleLatencyTest() {
        if (!isBleReady()) {
            setStatus("Connect BLE first");
            appendLog("BLE RTT skipped: not ready");
            return;
        }
        if (bleLatencyRunning) {
            appendLog("BLE RTT already running");
            return;
        }

        bleLatencyRunning = true;
        bleTotal = SAMPLE_COUNT;
        bleSeq = 0;
        bleWaitingSeq = -1;
        bleWaitingSendMs = -1;
        bleLost = 0;
        bleSamples.clear();
        summaryText.setText("Result: BLE running...");
        appendLog("BLE RTT start samples=" + bleTotal);
        setStatus("BLE RTT running");
        updateButtons();
        sendNextBleLatencySample();
    }

    private void sendNextBleLatencySample() {
        if (!bleLatencyRunning) {
            return;
        }
        if (bleSeq >= bleTotal) {
            finishBleLatencyTest();
            return;
        }

        bleSeq++;
        int seq = bleSeq;
        long sendMs = SystemClock.elapsedRealtime();
        String data = "lat|" + seq + "|" + sendMs;
        bleWaitingSeq = seq;
        bleWaitingSendMs = sendMs;
        appendLog("BLE TRACE seq=" + seq + " stage=android_tx_prepare client_send_ms="
                + sendMs + " data=" + data);

        if (!writeBleJson("sdk.demo.ping", data, seq, sendMs)) {
            appendLog("BLE TX failed seq=" + seq);
            bleLost++;
            bleWaitingSeq = -1;
            bleWaitingSendMs = -1;
            mainHandler.postDelayed(this::sendNextBleLatencySample, BLE_SAMPLE_GAP_MS);
            return;
        }

        mainHandler.postDelayed(() -> {
            if (bleLatencyRunning && bleWaitingSeq == seq) {
                appendLog("BLE timeout seq=" + seq);
                bleLost++;
                bleWaitingSeq = -1;
                bleWaitingSendMs = -1;
                sendNextBleLatencySample();
            }
        }, RTT_TIMEOUT_MS);
    }

    private boolean writeBleJson(String datatype, String data, int seq, long clientSendMs) {
        if (gatt == null || communicateCharacteristic == null) {
            return false;
        }

        String json = "{\"datatype\":\"" + datatype + "\",\"data\":\"" + data + "\"}";
        byte[] payload = json.getBytes(StandardCharsets.UTF_8);
        if (payload.length > BLE_PACKET_LIMIT) {
            appendLog("BLE payload too large len=" + payload.length);
            return false;
        }

        int writeType = chooseWriteType(communicateCharacteristic);
        long writeStartMs = SystemClock.elapsedRealtime();
        boolean accepted;
        int result = BluetoothGatt.GATT_SUCCESS;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            result = gatt.writeCharacteristic(communicateCharacteristic, payload, writeType);
            accepted = result == BluetoothGatt.GATT_SUCCESS;
        } else {
            communicateCharacteristic.setWriteType(writeType);
            communicateCharacteristic.setValue(payload);
            accepted = gatt.writeCharacteristic(communicateCharacteristic);
        }
        long writeEndMs = SystemClock.elapsedRealtime();
        appendLog("BLE TRACE seq=" + seq + " stage=android_gatt_write_call client_send_ms="
                + clientSendMs + " local_ms=" + writeEndMs + " elapsed_ms="
                + (writeEndMs - writeStartMs) + " write_type=" + formatWriteType(writeType)
                + " payload_len=" + payload.length + " accepted=" + accepted
                + " result=" + result);
        return accepted;
    }

    private void handleBleNotify(byte[] value) {
        long notifyCallbackMs = SystemClock.elapsedRealtime();
        String text = value == null ? "" : new String(value, StandardCharsets.UTF_8);
        runOnUiThreadSafe(() -> {
            long uiMs = SystemClock.elapsedRealtime();
            appendLog("BLE TRACE stage=android_notify_callback local_ms=" + notifyCallbackMs
                    + " callback_to_ui_ms=" + (uiMs - notifyCallbackMs)
                    + " payload_len=" + (value == null ? 0 : value.length));
            appendLog("BLE RX " + text);
            try {
                JSONObject object = new JSONObject(text);
                String datatype = object.optString("datatype", "");
                String data = object.optString("data", "");
                if ("sdk.demo.pong".equals(datatype)) {
                    handleBlePong(data, notifyCallbackMs);
                }
            } catch (JSONException e) {
                appendLog("BLE RX invalid json");
            }
        });
    }

    private void handleBleWriteCallback(int status) {
        long callbackMs = SystemClock.elapsedRealtime();
        int seq = bleWaitingSeq;
        long clientSendMs = bleWaitingSendMs;
        long elapsedMs = clientSendMs >= 0 ? callbackMs - clientSendMs : -1;
        appendLog("BLE TRACE seq=" + seq + " stage=android_write_callback client_send_ms="
                + clientSendMs + " local_ms=" + callbackMs + " elapsed_ms=" + elapsedMs
                + " status=" + status);
    }

    private void handleBlePong(String data, long notifyCallbackMs) {
        if (!bleLatencyRunning || !data.startsWith("ack:lat|")) {
            return;
        }

        String[] parts = data.substring("ack:".length()).split("\\|");
        if (parts.length != 3) {
            appendLog("BLE RTT invalid pong data=" + data);
            return;
        }

        int seq;
        long sendMs;
        try {
            seq = Integer.parseInt(parts[1]);
            sendMs = Long.parseLong(parts[2]);
        } catch (NumberFormatException e) {
            appendLog("BLE RTT parse failed data=" + data);
            return;
        }

        if (seq != bleWaitingSeq) {
            appendLog("BLE RTT ignored seq=" + seq + " waiting=" + bleWaitingSeq);
            return;
        }

        long rtt = notifyCallbackMs - sendMs;
        bleSamples.add(rtt);
        bleWaitingSeq = -1;
        bleWaitingSendMs = -1;
        appendLog("BLE TRACE seq=" + seq + " stage=android_pong_matched client_send_ms="
                + sendMs + " local_ms=" + notifyCallbackMs + " rtt_ms=" + rtt);
        appendLog("BLE RTT seq=" + seq + " rtt_ms=" + rtt);
        mainHandler.postDelayed(this::sendNextBleLatencySample, BLE_SAMPLE_GAP_MS);
    }

    private void finishBleLatencyTest() {
        bleLatencyRunning = false;
        bleWaitingSeq = -1;
        bleWaitingSendMs = -1;
        String result = formatStats("BLE", bleTotal, bleLost, bleSamples);
        summaryText.setText("Result: " + result);
        appendLog(result);
        setStatus("BLE RTT finished");
        updateButtons();
    }

    private void stopBleLatency(String reason) {
        if (!bleLatencyRunning) {
            return;
        }
        bleLatencyRunning = false;
        bleWaitingSeq = -1;
        bleWaitingSendMs = -1;
        appendLog("BLE RTT stopped: " + reason);
        updateButtons();
    }

    private void runSppLatencyTest() {
        if (!ensureBluetoothReady()) {
            return;
        }
        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
            return;
        }
        if (!sppTestRunning.compareAndSet(false, true)) {
            appendLog("SPP RTT already running");
            return;
        }

        summaryText.setText("Result: SPP running...");
        setStatus("SPP RTT running");
        updateButtons();
        executor.execute(() -> {
            try {
                executeSppLatencyTest();
            } finally {
                sppTestRunning.set(false);
                runOnUiThreadSafe(this::updateButtons);
            }
        });
    }

    private void executeSppLatencyTest() {
        closeSppSocket();
        BluetoothDevice target = findSppTargetDevice();
        if (target == null) {
            appendLog("SPP no target device");
            setStatusOnUi("SPP no target device");
            return;
        }

        BluetoothSocket activeSocket = null;
        try {
            activeSocket = connectSppSocket(target);
            synchronized (this) {
                sppSocket = activeSocket;
            }
            appendLog("SPP connected " + describe(target));
            startSppReader(activeSocket);
            runSppRtt(activeSocket);
        } catch (Exception e) {
            appendLog("SPP test failed: " + e.getClass().getSimpleName() + ": " + e.getMessage());
            setStatusOnUi("SPP RTT failed");
        } finally {
            closeSppSocket();
        }
    }

    private BluetoothDevice findSppTargetDevice() {
        if (targetAddress != null) {
            try {
                appendLog("SPP use target_address " + targetAddress);
                return bluetoothAdapter.getRemoteDevice(targetAddress);
            } catch (IllegalArgumentException e) {
                appendLog("SPP invalid target_address " + targetAddress);
            }
        }

        BluetoothDevice discovered = discoverClassicDevice();
        if (discovered != null) {
            appendLog("SPP selected discovered " + describe(discovered));
            return discovered;
        }

        Set<BluetoothDevice> bondedDevices = bluetoothAdapter.getBondedDevices();
        appendLog("SPP bonded devices=" + bondedDevices.size());
        for (BluetoothDevice device : bondedDevices) {
            String name = safeDeviceName(device);
            appendLog("SPP bonded " + name + " " + device.getAddress());
            if (isTargetDevice(name, device.getAddress())) {
                return device;
            }
        }
        return null;
    }

    private BluetoothDevice discoverClassicDevice() {
        AtomicReference<BluetoothDevice> foundDevice = new AtomicReference<>();
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
                    appendLog("SPP found " + name + " " + device.getAddress());
                    if (isTargetDevice(name, device.getAddress())
                            && foundDevice.compareAndSet(null, device)) {
                        bluetoothAdapter.cancelDiscovery();
                        latch.countDown();
                    }
                } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                    appendLog("SPP discovery finished");
                    latch.countDown();
                }
            }
        };

        registerReceiverCompat(receiver, filter);
        try {
            if (bluetoothAdapter.isDiscovering()) {
                bluetoothAdapter.cancelDiscovery();
            }
            appendLog("SPP discovery start");
            if (!bluetoothAdapter.startDiscovery()) {
                appendLog("SPP startDiscovery returned false");
                return null;
            }
            try {
                latch.await(DISCOVERY_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            return foundDevice.get();
        } finally {
            try {
                unregisterReceiver(receiver);
            } catch (IllegalArgumentException ignored) {
                // Activity may be shutting down.
            }
            if (bluetoothAdapter.isDiscovering()) {
                bluetoothAdapter.cancelDiscovery();
            }
        }
    }

    private BluetoothSocket connectSppSocket(BluetoothDevice device) throws IOException {
        appendLog("SPP connecting " + describe(device));
        IOException firstError = null;

        try {
            Method method = device.getClass().getMethod("createInsecureRfcommSocket", int.class);
            BluetoothSocket channelSocket = (BluetoothSocket) method.invoke(device, SPP_CHANNEL);
            channelSocket.connect();
            appendLog("SPP connected by channel=" + SPP_CHANNEL);
            return channelSocket;
        } catch (Exception e) {
            firstError = new IOException("channel connect failed: " + e.getMessage(), e);
            appendLog("SPP channel connect failed, fallback UUID: "
                    + e.getClass().getSimpleName() + ": " + e.getMessage());
        }

        BluetoothSocket uuidSocket = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
        try {
            uuidSocket.connect();
            appendLog("SPP connected by UUID");
            return uuidSocket;
        } catch (IOException e) {
            if (firstError != null) {
                e.addSuppressed(firstError);
            }
            throw e;
        }
    }

    private void startSppReader(BluetoothSocket activeSocket) {
        sppLines.clear();
        Thread reader = new Thread(() -> readSppLines(activeSocket), "spp-latency-reader");
        reader.start();
    }

    private void readSppLines(BluetoothSocket activeSocket) {
        ByteArrayOutputStream line = new ByteArrayOutputStream();
        byte[] buffer = new byte[512];
        try {
            InputStream in = activeSocket.getInputStream();
            while (activeSocket.isConnected()) {
                int len = in.read(buffer);
                if (len < 0) {
                    appendLog("SPP RX EOF");
                    return;
                }
                long socketReadMs = SystemClock.elapsedRealtime();
                appendLog("SPP TRACE stage=android_socket_read local_ms=" + socketReadMs
                        + " len=" + len);
                for (int i = 0; i < len; i++) {
                    byte b = buffer[i];
                    if (b == '\n') {
                        String text = line.toString(StandardCharsets.UTF_8.name());
                        line.reset();
                        if (!TextUtils.isEmpty(text)) {
                            appendLog("SPP TRACE stage=android_line_received local_ms="
                                    + socketReadMs + " line=" + text);
                            appendLog("SPP RX " + text);
                            sppLines.offer(new SppLine(text, socketReadMs));
                        }
                    } else if (b != '\r') {
                        line.write(b);
                    }
                }
            }
        } catch (IOException e) {
            appendLog("SPP reader stopped: " + e.getClass().getSimpleName() + ": "
                    + e.getMessage());
        }
    }

    private void runSppRtt(BluetoothSocket activeSocket) throws IOException {
        List<Long> samples = new ArrayList<>();
        int lost = 0;
        OutputStream out = activeSocket.getOutputStream();

        appendLog("SPP RTT start samples=" + SAMPLE_COUNT);
        for (int seq = 1; seq <= SAMPLE_COUNT; seq++) {
            long sendMs = SystemClock.elapsedRealtime();
            String payload = "OSAIG_LATENCY seq=" + seq + " t=" + sendMs + "\n";
            long writeStartMs = SystemClock.elapsedRealtime();
            out.write(payload.getBytes(StandardCharsets.UTF_8));
            out.flush();
            long writeEndMs = SystemClock.elapsedRealtime();
            appendLog("SPP TRACE seq=" + seq + " stage=android_write_flush client_send_ms="
                    + sendMs + " local_ms=" + writeEndMs + " elapsed_ms="
                    + (writeEndMs - writeStartMs) + " payload_len="
                    + payload.getBytes(StandardCharsets.UTF_8).length);
            appendLog("SPP TX seq=" + seq + " t=" + sendMs);

            SppWaitResult result = waitForSppEcho(seq, sendMs);
            if (result == null) {
                lost++;
                appendLog("SPP timeout seq=" + seq);
            } else {
                samples.add(result.rttMs);
                appendLog("SPP TRACE seq=" + seq + " stage=android_echo_matched client_send_ms="
                        + sendMs + " local_ms=" + result.rxMs + " rtt_ms=" + result.rttMs
                        + " queue_match_delay_ms=" + result.matchDelayMs);
                appendLog("SPP RTT seq=" + seq + " rtt_ms=" + result.rttMs);
            }

            try {
                Thread.sleep(SPP_SAMPLE_GAP_MS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }

        String result = formatStats("SPP", SAMPLE_COUNT, lost, samples);
        appendLog(result);
        runOnUiThreadSafe(() -> {
            summaryText.setText("Result: " + result);
            setStatus("SPP RTT finished");
        });
    }

    private SppWaitResult waitForSppEcho(int seq, long sendMs) {
        long deadline = SystemClock.elapsedRealtime() + RTT_TIMEOUT_MS;
        while (SystemClock.elapsedRealtime() < deadline) {
            long remaining = deadline - SystemClock.elapsedRealtime();
            SppLine item;
            try {
                item = sppLines.poll(Math.max(1, remaining), TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return null;
            }
            if (item == null) {
                continue;
            }
            String line = item.text;
            if (!line.startsWith("OSAIG_LATENCY ")) {
                appendLog("SPP ignored line=" + line);
                continue;
            }
            Integer rxSeq = parseSppSeq(line);
            if (rxSeq == null || rxSeq != seq) {
                appendLog("SPP ignored seq line=" + line);
                continue;
            }
            long matchMs = SystemClock.elapsedRealtime();
            return new SppWaitResult(item.rxMs - sendMs, item.rxMs, matchMs - item.rxMs);
        }
        return null;
    }

    private Integer parseSppSeq(String line) {
        String[] parts = line.split(" ");
        for (String part : parts) {
            if (part.startsWith("seq=")) {
                try {
                    return Integer.parseInt(part.substring("seq=".length()));
                } catch (NumberFormatException ignored) {
                    return null;
                }
            }
        }
        return null;
    }

    private synchronized void closeSppSocket() {
        if (sppSocket != null) {
            try {
                sppSocket.close();
            } catch (IOException ignored) {
                // Best-effort cleanup for a demo client.
            }
            sppSocket = null;
        }
    }

    private void disconnectAll() {
        stopBleLatency("disconnect requested");
        stopScan();
        if (gatt != null) {
            gatt.disconnect();
        }
        closeGatt();
        closeSppSocket();
        setStatus("Disconnected");
        updateButtons();
    }

    private boolean ensureBluetoothReady() {
        if (bluetoothAdapter == null) {
            setStatus("Bluetooth unavailable");
            appendLog("BluetoothAdapter is null");
            return false;
        }
        if (!bluetoothAdapter.isEnabled()) {
            setStatus("Turn on Bluetooth first");
            appendLog("Bluetooth disabled");
            return false;
        }
        return true;
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
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_BLUETOOTH_PERMISSIONS) {
            return;
        }
        if (hasBluetoothPermissions()) {
            appendLog("Bluetooth permissions granted");
            updateButtons();
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

    private boolean isBleReady() {
        return gatt != null && communicateCharacteristic != null && notifyEnabled;
    }

    private int chooseWriteType(BluetoothGattCharacteristic characteristic) {
        int properties = characteristic.getProperties();
        if ((properties & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) {
            return BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT;
        }
        return BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE;
    }

    private String formatWriteType(int writeType) {
        if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) {
            return "default";
        }
        if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) {
            return "no_response";
        }
        if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_SIGNED) {
            return "signed";
        }
        return String.valueOf(writeType);
    }

    private byte[] chooseCccdValue(BluetoothGattCharacteristic characteristic) {
        int properties = characteristic.getProperties();
        boolean canNotify = (properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0;
        boolean canIndicate = (properties & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0;
        if (canNotify) {
            return BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
        }
        if (canIndicate) {
            return BluetoothGattDescriptor.ENABLE_INDICATION_VALUE;
        }
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

    private String formatStats(String label, int sent, int lost, List<Long> samples) {
        if (samples.isEmpty()) {
            return label + " RESULT sent=" + sent + " received=0 lost=" + lost;
        }

        List<Long> sorted = new ArrayList<>(samples);
        Collections.sort(sorted);
        long sum = 0;
        for (Long sample : sorted) {
            sum += sample;
        }

        long min = sorted.get(0);
        long max = sorted.get(sorted.size() - 1);
        long p50 = sorted.get(percentileIndex(sorted.size(), 0.50));
        long p95 = sorted.get(percentileIndex(sorted.size(), 0.95));
        double avg = sum / (double) sorted.size();
        return String.format(Locale.US,
                "%s RESULT sent=%d received=%d lost=%d min_ms=%d avg_ms=%.1f p50_ms=%d p95_ms=%d max_ms=%d",
                label, sent, sorted.size(), lost, min, avg, p50, p95, max);
    }

    private int percentileIndex(int size, double percentile) {
        int index = (int) Math.ceil(size * percentile) - 1;
        if (index < 0) {
            return 0;
        }
        if (index >= size) {
            return size - 1;
        }
        return index;
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

    private boolean isTargetDevice(String name, String address) {
        if (targetAddress != null) {
            return targetAddress.equalsIgnoreCase(address);
        }
        return name != null && OSAIG_NAME_PATTERN.matcher(name.trim()).matches();
    }

    private String normalizeAddress(String address) {
        if (TextUtils.isEmpty(address)) {
            return null;
        }
        String trimmed = address.trim().toUpperCase(Locale.US);
        return BluetoothAdapter.checkBluetoothAddress(trimmed) ? trimmed : null;
    }

    private String describe(BluetoothDevice device) {
        return safeDeviceName(device) + " " + device.getAddress();
    }

    private String safeDeviceName(BluetoothDevice device) {
        String name = device.getName();
        return TextUtils.isEmpty(name) ? "Unnamed" : name;
    }

    private void setStatus(String status) {
        statusText.setText("Status: " + status);
    }

    private void setStatusOnUi(String status) {
        runOnUiThreadSafe(() -> setStatus(status));
    }

    private void updateButtons() {
        boolean bleReady = isBleReady();
        connectBleButton.setEnabled(!scanning && !bleLatencyRunning);
        runBleButton.setEnabled(bleReady && !bleLatencyRunning);
        runSppButton.setEnabled(!sppTestRunning.get());
        disconnectButton.setEnabled(gatt != null || scanning || sppSocket != null);
    }

    private void appendLog(String message) {
        Log.i(TAG, message);
        String line = timeFormat.format(new Date()) + "  " + message + "\n";
        runOnUiThreadSafe(() -> {
            logText.append(line);
            int scrollAmount = logText.getLayout() == null ? 0 :
                    logText.getLayout().getLineTop(logText.getLineCount()) - logText.getHeight();
            if (scrollAmount > 0) {
                logText.scrollTo(0, scrollAmount);
            }
        });
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
