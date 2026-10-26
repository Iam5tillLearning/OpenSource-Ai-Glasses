package com.osaig.sdk.classicbt.demo;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

public class MainActivity extends Activity {
    private static final String TAG = "OSAIG_SPP_WIFI_DEMO";
    private static final UUID SPP_UUID =
            UUID.fromString("00001911-0000-1000-8000-00805f9b34fb");
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1001;
    private static final long DISCOVERY_TIMEOUT_MS = 15000;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final AtomicBoolean connectRunning = new AtomicBoolean(false);

    private TextView deviceStatusView;
    private TextView wifiStatusView;
    private TextView logView;
    private EditText ssidInput;
    private EditText passwordInput;
    private Button connectDeviceButton;
    private Button disconnectDeviceButton;
    private Button connectWifiButton;
    private Button disconnectWifiButton;
    private Button refreshWifiButton;

    private BluetoothSocket socket;
    private Thread readerThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(createContentView());
        updateConnectionStatus("Disconnected");
        updateWifiStatus("Wi-Fi status: --");
        updateButtons();

        if (hasBluetoothPermissions()) {
            connectDevice();
        } else {
            requestBluetoothPermissions();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSIONS && hasBluetoothPermissions()) {
            appendLog("Bluetooth permissions granted");
            connectDevice();
        } else {
            appendLog("Bluetooth permissions denied");
            updateConnectionStatus("Permissions denied");
        }
    }

    @Override
    protected void onDestroy() {
        closeSocket();
        executor.shutdownNow();
        super.onDestroy();
    }

    private View createContentView() {
        int padding = dp(16);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("OSAIG SPP Wi-Fi Demo");
        title.setTextSize(22);
        title.setPadding(0, 0, 0, dp(12));
        root.addView(title);

        deviceStatusView = new TextView(this);
        deviceStatusView.setTextSize(16);
        deviceStatusView.setPadding(0, 0, 0, dp(8));
        root.addView(deviceStatusView);

        wifiStatusView = new TextView(this);
        wifiStatusView.setTextSize(14);
        wifiStatusView.setPadding(0, 0, 0, dp(12));
        root.addView(wifiStatusView);

        LinearLayout deviceButtons = new LinearLayout(this);
        deviceButtons.setOrientation(LinearLayout.HORIZONTAL);

        connectDeviceButton = new Button(this);
        connectDeviceButton.setText("Connect SPP");
        connectDeviceButton.setAllCaps(false);
        connectDeviceButton.setOnClickListener(v -> connectDevice());
        deviceButtons.addView(connectDeviceButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectDeviceButton = new Button(this);
        disconnectDeviceButton.setText("Disconnect");
        disconnectDeviceButton.setAllCaps(false);
        disconnectDeviceButton.setOnClickListener(v -> disconnectDevice());
        deviceButtons.addView(disconnectDeviceButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(deviceButtons);

        ssidInput = new EditText(this);
        ssidInput.setHint("Wi-Fi SSID");
        root.addView(ssidInput);

        passwordInput = new EditText(this);
        passwordInput.setHint("Wi-Fi Password");
        passwordInput.setInputType(InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        root.addView(passwordInput);

        LinearLayout wifiButtons = new LinearLayout(this);
        wifiButtons.setOrientation(LinearLayout.HORIZONTAL);

        connectWifiButton = new Button(this);
        connectWifiButton.setText("Connect Wi-Fi");
        connectWifiButton.setAllCaps(false);
        connectWifiButton.setOnClickListener(v -> sendWifiConnect());
        wifiButtons.addView(connectWifiButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectWifiButton = new Button(this);
        disconnectWifiButton.setText("Disconnect Wi-Fi");
        disconnectWifiButton.setAllCaps(false);
        disconnectWifiButton.setOnClickListener(v -> sendWifiDisconnect());
        wifiButtons.addView(disconnectWifiButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(wifiButtons);

        LinearLayout utilButtons = new LinearLayout(this);
        utilButtons.setOrientation(LinearLayout.HORIZONTAL);

        refreshWifiButton = new Button(this);
        refreshWifiButton.setText("Refresh Status");
        refreshWifiButton.setAllCaps(false);
        refreshWifiButton.setOnClickListener(v -> sendWifiStatus());
        utilButtons.addView(refreshWifiButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        Button clearLogButton = new Button(this);
        clearLogButton.setText("Clear Log");
        clearLogButton.setAllCaps(false);
        clearLogButton.setOnClickListener(v -> logView.setText(""));
        utilButtons.addView(clearLogButton,
                new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(utilButtons);

        ScrollView scrollView = new ScrollView(this);
        logView = new TextView(this);
        logView.setTextSize(13);
        logView.setTextIsSelectable(true);
        scrollView.addView(logView);
        root.addView(scrollView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1));

        return root;
    }

    private boolean hasBluetoothPermissions() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return true;
        }
        return checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)
                == PackageManager.PERMISSION_GRANTED;
    }

    private void requestBluetoothPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            requestPermissions(new String[]{
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Manifest.permission.BLUETOOTH_SCAN
            }, REQUEST_BLUETOOTH_PERMISSIONS);
        }
    }

    private void connectDevice() {
        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
            return;
        }
        if (!connectRunning.compareAndSet(false, true)) {
            appendLog("connection already running");
            return;
        }

        updateConnectionStatus("Connecting...");
        updateButtons();
        executor.execute(() -> {
            try {
                doConnectDevice();
            } finally {
                connectRunning.set(false);
                runOnUiThread(this::updateButtons);
            }
        });
    }

    private void doConnectDevice() {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            appendLog("BluetoothAdapter is null");
            updateConnectionStatus("Bluetooth unavailable");
            return;
        }
        if (!adapter.isEnabled()) {
            appendLog("Bluetooth is disabled");
            updateConnectionStatus("Bluetooth disabled");
            return;
        }

        closeSocket();
        BluetoothDevice target = discoverOsaigDevice(adapter);
        if (target == null) {
            List<BluetoothDevice> bondedDevices = findPairedOsaigDevices(adapter);
            if (!bondedDevices.isEmpty()) {
                target = bondedDevices.get(0);
                appendLog("fallback to bonded " + describe(target));
            }
        }
        if (target == null) {
            appendLog("no OSAIG device found");
            updateConnectionStatus("No OSAIG device");
            return;
        }

        appendLog("connecting to " + describe(target));
        try {
            adapter.cancelDiscovery();
            BluetoothSocket activeSocket =
                    target.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
            activeSocket.connect();
            socket = activeSocket;
            appendLog("SPP connected");
            updateConnectionStatus("SPP connected: " + safeName(target));
            startReader(activeSocket);
            sendWifiStatus();
        } catch (IOException e) {
            appendLog("connect failed: " + e.getClass().getSimpleName()
                    + ": " + e.getMessage());
            updateConnectionStatus("Connect failed");
            closeSocket();
        }
    }

    private BluetoothDevice discoverOsaigDevice(BluetoothAdapter adapter) {
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
                    String name = safeName(device);
                    appendLog("found " + name + " " + device.getAddress());
                    if (isOsaigName(name) && foundDevice.compareAndSet(null, device)) {
                        adapter.cancelDiscovery();
                        latch.countDown();
                    }
                } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                    appendLog("discovery finished");
                    latch.countDown();
                }
            }
        };

        registerReceiverCompat(receiver, filter);
        try {
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
            appendLog("starting discovery");
            if (!adapter.startDiscovery()) {
                appendLog("startDiscovery returned false");
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
                // Receiver may already be unregistered.
            }
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
        }
    }

    private List<BluetoothDevice> findPairedOsaigDevices(BluetoothAdapter adapter) {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        List<BluetoothDevice> result = new ArrayList<>();
        appendLog("bonded devices=" + bondedDevices.size());
        for (BluetoothDevice device : bondedDevices) {
            String name = safeName(device);
            if (isOsaigName(name)) {
                appendLog("bonded " + describe(device));
                result.add(device);
            }
        }
        return result;
    }

    private void startReader(BluetoothSocket activeSocket) {
        readerThread = new Thread(() -> readLoop(activeSocket), "spp-wifi-reader");
        readerThread.start();
    }

    private void readLoop(BluetoothSocket activeSocket) {
        StringBuilder pending = new StringBuilder();
        byte[] buffer = new byte[512];

        try {
            InputStream in = activeSocket.getInputStream();
            while (activeSocket.isConnected()) {
                int len = in.read(buffer);
                if (len < 0) {
                    appendLog("RX EOF");
                    break;
                }
                String text = new String(buffer, 0, len, StandardCharsets.UTF_8);
                pending.append(text);

                int newlineIndex;
                while ((newlineIndex = pending.indexOf("\n")) >= 0) {
                    String line = pending.substring(0, newlineIndex).trim();
                    pending.delete(0, newlineIndex + 1);
                    if (!line.isEmpty()) {
                        handleResponseLine(line);
                    }
                }
            }
        } catch (IOException e) {
            appendLog("reader stopped: " + e.getClass().getSimpleName()
                    + ": " + e.getMessage());
        } finally {
            closeSocket();
            updateConnectionStatus("Disconnected");
            runOnUiThread(this::updateButtons);
        }
    }

    private void handleResponseLine(String line) {
        appendLog("RX " + line);
        try {
            JSONObject object = new JSONObject(line);
            renderWifiStatus(object);
        } catch (Exception e) {
            appendLog("response parse failed: " + e.getClass().getSimpleName());
        }
    }

    private void renderWifiStatus(JSONObject object) {
        boolean ok = object.optBoolean("ok", false);
        String action = object.optString("action", "unknown");
        String state = object.optString("state", "unknown");
        String ssid = object.optString("ssid", "--");
        String ip = object.optString("ip", "--");
        int signalDbm = object.has("signal_dbm") ? object.optInt("signal_dbm") : 0;
        int frequencyMhz = object.has("frequency_mhz") ? object.optInt("frequency_mhz") : 0;
        String message = object.optString("message", "");

        StringBuilder builder = new StringBuilder();
        builder.append("Wi-Fi state: ").append(state)
                .append("\nSSID: ").append(ssid)
                .append("\nIP: ").append(ip);
        if (frequencyMhz > 0) {
            builder.append("\nFrequency: ").append(frequencyMhz).append(" MHz");
        }
        if (signalDbm != 0 || "connected".equals(state)) {
            builder.append("\nSignal: ").append(signalDbm).append(" dBm");
        }
        if (!TextUtils.isEmpty(message)) {
            builder.append("\nMessage: ").append(message);
        }
        updateWifiStatus(builder.toString());

        if ("status".equals(action) && ok && !"--".equals(ssid)
                && (ssidInput.getText() == null || ssidInput.getText().length() == 0)) {
            runOnUiThread(() -> ssidInput.setText(ssid));
        }
    }

    private void sendWifiConnect() {
        String ssid = ssidInput.getText() == null
                ? ""
                : ssidInput.getText().toString().trim();
        String password = passwordInput.getText() == null
                ? ""
                : passwordInput.getText().toString();
        if (ssid.isEmpty()) {
            appendLog("ssid is empty");
            return;
        }
        JSONObject object = new JSONObject();
        try {
            object.put("action", "connect");
            object.put("ssid", ssid);
            object.put("password", password);
            appendLog("TX connect ssid=" + ssid);
            sendJsonCommand(object);
            updateWifiStatus("Wi-Fi state: connecting\nSSID: " + ssid);
        } catch (Exception e) {
            appendLog("failed to build connect command");
        }
    }

    private void sendWifiDisconnect() {
        JSONObject object = new JSONObject();
        try {
            object.put("action", "disconnect");
            appendLog("TX disconnect");
            sendJsonCommand(object);
        } catch (Exception e) {
            appendLog("failed to build disconnect command");
        }
    }

    private void sendWifiStatus() {
        JSONObject object = new JSONObject();
        try {
            object.put("action", "status");
            appendLog("TX status");
            sendJsonCommand(object);
        } catch (Exception e) {
            appendLog("failed to build status command");
        }
    }

    private synchronized void sendJsonCommand(JSONObject object) {
        if (socket == null || !socket.isConnected()) {
            appendLog("SPP not connected");
            return;
        }

        try {
            OutputStream out = socket.getOutputStream();
            String line = object.toString() + "\n";
            out.write(line.getBytes(StandardCharsets.UTF_8));
            out.flush();
        } catch (IOException e) {
            appendLog("write failed: " + e.getClass().getSimpleName()
                    + ": " + e.getMessage());
            closeSocket();
            updateConnectionStatus("Disconnected");
            runOnUiThread(this::updateButtons);
        }
    }

    private void disconnectDevice() {
        appendLog("disconnect requested");
        closeSocket();
        updateConnectionStatus("Disconnected");
        updateButtons();
    }

    private synchronized void closeSocket() {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ignored) {
                // Best-effort cleanup.
            }
            socket = null;
        }
    }

    private void updateButtons() {
        boolean connected = socket != null && socket.isConnected();
        boolean busy = connectRunning.get();
        connectDeviceButton.setEnabled(!connected && !busy);
        disconnectDeviceButton.setEnabled(connected || busy);
        connectWifiButton.setEnabled(connected && !busy);
        disconnectWifiButton.setEnabled(connected && !busy);
        refreshWifiButton.setEnabled(connected && !busy);
    }

    private void updateConnectionStatus(String status) {
        runOnUiThread(() -> deviceStatusView.setText("SPP: " + status));
    }

    private void updateWifiStatus(String status) {
        runOnUiThread(() -> wifiStatusView.setText(status));
    }

    private void registerReceiverCompat(BroadcastReceiver receiver, IntentFilter filter) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(receiver, filter);
        }
    }

    private boolean isOsaigName(String name) {
        return name != null && (name.startsWith("OSAIG-") || name.equals("OSAIG"));
    }

    private String describe(BluetoothDevice device) {
        return safeName(device) + " " + device.getAddress();
    }

    private String safeName(BluetoothDevice device) {
        String name = device.getName();
        return name == null ? "" : name;
    }

    private void appendLog(String line) {
        Log.i(TAG, line);
        runOnUiThread(() -> {
            logView.append(line + "\n");
            int scrollAmount = logView.getLayout() == null ? 0
                    : logView.getLayout().getLineTop(logView.getLineCount()) - logView.getHeight();
            if (scrollAmount > 0) {
                logView.scrollTo(0, scrollAmount);
            }
        });
    }

    private int dp(int value) {
        float density = getResources().getDisplayMetrics().density;
        return Math.round(value * density);
    }
}
