package com.osaig.sdk.classicbt.demo;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.regex.Pattern;

@SuppressLint("MissingPermission")
public class MainActivity extends Activity {
    private static final int REQUEST_BT_PERMISSIONS = 2001;
    private static final UUID SPP_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
    private static final Pattern OSAIG_NAME_PATTERN = Pattern.compile("^OSAIG-[0-9A-F]{4}$");

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final SimpleDateFormat timeFormat =
            new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothDevice selectedDevice;
    private BluetoothSocket socket;
    private OutputStream outputStream;
    private Thread connectThread;
    private Thread readThread;
    private volatile boolean connected;

    private TextView statusText;
    private TextView deviceText;
    private TextView responseText;
    private TextView logText;
    private Button listButton;
    private Button connectButton;
    private Button sendButton;
    private Button disconnectButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        buildUi();

        BluetoothManager manager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager == null ? null : manager.getAdapter();
        setStatus("Idle");
        updateButtons();

        if (!hasRequiredPermissions()) {
            requestPermissions(requiredPermissions(), REQUEST_BT_PERMISSIONS);
        } else {
            refreshPairedDevices();
        }
    }

    @Override
    protected void onDestroy() {
        closeConnection();
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_BT_PERMISSIONS) {
            return;
        }
        if (hasRequiredPermissions()) {
            appendLog("Bluetooth permission granted");
            refreshPairedDevices();
        } else {
            setStatus("Bluetooth permission denied");
            appendLog("Bluetooth permission denied");
        }
        updateButtons();
    }

    private void buildUi() {
        int padding = dp(16);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("OSAIG Classic BT SPP Demo");
        title.setTextSize(22);
        title.setPadding(0, 0, 0, dp(12));
        root.addView(title);

        statusText = new TextView(this);
        statusText.setTextSize(16);
        statusText.setPadding(0, 0, 0, dp(8));
        root.addView(statusText);

        deviceText = new TextView(this);
        deviceText.setText("Device: --");
        deviceText.setTextSize(16);
        deviceText.setPadding(0, 0, 0, dp(8));
        root.addView(deviceText);

        responseText = new TextView(this);
        responseText.setText("Last response: --");
        responseText.setTextSize(16);
        responseText.setPadding(0, 0, 0, dp(12));
        root.addView(responseText);

        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);

        listButton = new Button(this);
        listButton.setText("List Paired OSAIG");
        listButton.setAllCaps(false);
        listButton.setOnClickListener(v -> refreshPairedDevices());
        buttons.addView(listButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        connectButton = new Button(this);
        connectButton.setText("Connect");
        connectButton.setAllCaps(false);
        connectButton.setOnClickListener(v -> connectSelectedDevice());
        buttons.addView(connectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(buttons);

        LinearLayout buttons2 = new LinearLayout(this);
        buttons2.setOrientation(LinearLayout.HORIZONTAL);
        buttons2.setPadding(0, dp(8), 0, dp(8));

        sendButton = new Button(this);
        sendButton.setText("Send Echo");
        sendButton.setAllCaps(false);
        sendButton.setOnClickListener(v -> sendEcho());
        buttons2.addView(sendButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        disconnectButton = new Button(this);
        disconnectButton.setText("Disconnect");
        disconnectButton.setAllCaps(false);
        disconnectButton.setOnClickListener(v -> closeConnection());
        buttons2.addView(disconnectButton, new LinearLayout.LayoutParams(0, dp(48), 1));

        root.addView(buttons2);

        ScrollView scrollView = new ScrollView(this);
        logText = new TextView(this);
        logText.setTextSize(13);
        logText.setTextIsSelectable(true);
        scrollView.addView(logText);
        root.addView(scrollView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1));

        setContentView(root);
    }

    private String[] requiredPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            return new String[]{Manifest.permission.BLUETOOTH_CONNECT};
        }
        return new String[]{Manifest.permission.BLUETOOTH};
    }

    private boolean hasRequiredPermissions() {
        for (String permission : requiredPermissions()) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    private boolean isBluetoothReady() {
        if (bluetoothAdapter == null) {
            setStatus("Bluetooth adapter not found");
            return false;
        }
        if (!hasRequiredPermissions()) {
            setStatus("Bluetooth permission required");
            requestPermissions(requiredPermissions(), REQUEST_BT_PERMISSIONS);
            return false;
        }
        if (!bluetoothAdapter.isEnabled()) {
            setStatus("Bluetooth is disabled");
            return false;
        }
        return true;
    }

    private void refreshPairedDevices() {
        if (!isBluetoothReady()) {
            updateButtons();
            return;
        }

        Set<BluetoothDevice> bondedDevices = bluetoothAdapter.getBondedDevices();
        List<BluetoothDevice> targets = new ArrayList<>();
        for (BluetoothDevice device : bondedDevices) {
            String name = device.getName();
            if (name != null && OSAIG_NAME_PATTERN.matcher(name).matches()) {
                targets.add(device);
            }
        }

        appendLog("Paired OSAIG devices: " + targets.size());
        for (BluetoothDevice device : targets) {
            appendLog("  " + formatDevice(device));
        }

        if (targets.isEmpty()) {
            selectedDevice = null;
            deviceText.setText("Device: --");
            setStatus("Pair OSAIG-XXXX in system Bluetooth settings first");
        } else {
            selectedDevice = targets.get(0);
            deviceText.setText("Device: " + formatDevice(selectedDevice));
            setStatus("Ready to connect");
        }
        updateButtons();
    }

    private void connectSelectedDevice() {
        if (selectedDevice == null || !isBluetoothReady() || connected) {
            updateButtons();
            return;
        }

        setStatus("Connecting...");
        appendLog("Connecting to " + formatDevice(selectedDevice));
        updateButtons();

        connectThread = new Thread(() -> {
            BluetoothSocket newSocket = null;
            try {
                newSocket = selectedDevice.createRfcommSocketToServiceRecord(SPP_UUID);
                newSocket.connect();

                synchronized (this) {
                    socket = newSocket;
                    outputStream = newSocket.getOutputStream();
                    connected = true;
                }

                runOnUiThreadSafe(() -> {
                    setStatus("Connected");
                    appendLog("SPP connected");
                    updateButtons();
                });
                startReadLoop(newSocket);
            } catch (IOException e) {
                closeQuietly(newSocket);
                runOnUiThreadSafe(() -> {
                    setStatus("Connect failed: " + e.getMessage());
                    appendLog("Connect failed: " + e.getMessage());
                    updateButtons();
                });
            }
        }, "classic-bt-connect");
        connectThread.start();
    }

    private void startReadLoop(BluetoothSocket activeSocket) {
        readThread = new Thread(() -> {
            byte[] buffer = new byte[1024];
            try {
                InputStream inputStream = activeSocket.getInputStream();
                while (connected) {
                    int read = inputStream.read(buffer);
                    if (read <= 0) {
                        break;
                    }
                    String text = new String(buffer, 0, read, StandardCharsets.UTF_8);
                    runOnUiThreadSafe(() -> {
                        responseText.setText("Last response: " + text);
                        appendLog("RX: " + text);
                    });
                }
            } catch (IOException e) {
                runOnUiThreadSafe(() -> appendLog("Read stopped: " + e.getMessage()));
            } finally {
                closeConnection();
            }
        }, "classic-bt-read");
        readThread.start();
    }

    private void sendEcho() {
        final OutputStream stream;
        synchronized (this) {
            stream = outputStream;
        }

        if (!connected || stream == null) {
            setStatus("Not connected");
            updateButtons();
            return;
        }

        String message = "hello from android spp";
        byte[] payload = message.getBytes(StandardCharsets.UTF_8);
        new Thread(() -> {
            try {
                stream.write(payload);
                stream.flush();
                runOnUiThreadSafe(() -> appendLog("TX: " + message));
            } catch (IOException e) {
                runOnUiThreadSafe(() -> {
                    setStatus("Send failed: " + e.getMessage());
                    appendLog("Send failed: " + e.getMessage());
                    updateButtons();
                });
            }
        }, "classic-bt-send").start();
    }

    private void closeConnection() {
        BluetoothSocket oldSocket;
        synchronized (this) {
            oldSocket = socket;
            socket = null;
            outputStream = null;
            connected = false;
        }
        closeQuietly(oldSocket);
        runOnUiThreadSafe(() -> {
            setStatus(selectedDevice == null ? "Idle" : "Disconnected");
            updateButtons();
        });
    }

    private void closeQuietly(BluetoothSocket target) {
        if (target == null) {
            return;
        }
        try {
            target.close();
        } catch (IOException ignored) {
        }
    }

    private String formatDevice(BluetoothDevice device) {
        String name = device.getName();
        return (name == null ? "<unnamed>" : name) + " [" + device.getAddress() + "]";
    }

    private void setStatus(String status) {
        statusText.setText("Status: " + status);
    }

    private void appendLog(String message) {
        String line = timeFormat.format(new Date()) + "  " + message + "\n";
        logText.append(line);
    }

    private void updateButtons() {
        boolean hasPermission = hasRequiredPermissions();
        boolean hasAdapter = false;
        if (hasPermission && bluetoothAdapter != null) {
            hasAdapter = bluetoothAdapter.isEnabled();
        }
        listButton.setEnabled(hasPermission && hasAdapter && !connected);
        connectButton.setEnabled(hasPermission && hasAdapter && selectedDevice != null && !connected);
        sendButton.setEnabled(connected);
        disconnectButton.setEnabled(connected);
    }

    private void runOnUiThreadSafe(Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
        } else {
            mainHandler.post(runnable);
        }
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
}
