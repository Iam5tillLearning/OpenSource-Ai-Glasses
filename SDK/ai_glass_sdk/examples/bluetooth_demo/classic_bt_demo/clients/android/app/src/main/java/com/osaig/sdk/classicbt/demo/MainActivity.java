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
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
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
    private static final String TAG = "OSAIG_SPP_FD_TEST";
    private static final UUID SPP_UUID =
            UUID.fromString("00001911-0000-1000-8000-00805f9b34fb");
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1001;
    private static final long DISCOVERY_TIMEOUT_MS = 15000;
    private static final long BOND_TIMEOUT_MS = 30000;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final AtomicBoolean testRunning = new AtomicBoolean(false);

    private TextView logView;
    private BluetoothSocket socket;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(createContentView());

        if (hasBluetoothPermissions()) {
            runSppTest();
        } else {
            requestBluetoothPermissions();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSIONS && hasBluetoothPermissions()) {
            runSppTest();
        } else {
            appendLog("Bluetooth permissions denied");
        }
    }

    @Override
    protected void onDestroy() {
        closeSocket();
        executor.shutdownNow();
        super.onDestroy();
    }

    private View createContentView() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(24, 24, 24, 24);

        Button runButton = new Button(this);
        runButton.setText("Run SPP FD Test");
        runButton.setAllCaps(false);
        runButton.setOnClickListener(v -> runSppTest());
        root.addView(runButton, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

        Button clearButton = new Button(this);
        clearButton.setText("Clear Log");
        clearButton.setAllCaps(false);
        clearButton.setOnClickListener(v -> logView.setText(""));
        root.addView(clearButton, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

        ScrollView scrollView = new ScrollView(this);
        logView = new TextView(this);
        logView.setTextSize(13);
        logView.setTextIsSelectable(true);
        scrollView.addView(logView);
        root.addView(scrollView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1));

        return root;
    }

    private boolean hasBluetoothPermissions() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return true;
        }
        return checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED &&
                checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)
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

    private void runSppTest() {
        if (!hasBluetoothPermissions()) {
            appendLog("missing Bluetooth permissions");
            requestBluetoothPermissions();
            return;
        }
        if (!testRunning.compareAndSet(false, true)) {
            appendLog("test already running");
            return;
        }

        executor.execute(() -> {
            try {
                executeSppTest();
            } finally {
                testRunning.set(false);
            }
        });
    }

    private void executeSppTest() {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            appendLog("BluetoothAdapter is null");
            return;
        }
        if (!adapter.isEnabled()) {
            appendLog("Bluetooth is disabled");
            return;
        }

        closeSocket();

        BluetoothDevice discovered = discoverOsaigDevice(adapter);
        if (discovered != null) {
            appendLog("selected discovered device " + describe(discovered));
            connectAndSend(discovered, false);
            return;
        }

        List<BluetoothDevice> bondedDevices = findPairedOsaigDevices(adapter);
        if (bondedDevices.isEmpty()) {
            appendLog("no usable OSAIG-* classic Bluetooth device");
            return;
        }

        for (BluetoothDevice device : bondedDevices) {
            if (connectAndSend(device, true)) {
                return;
            }
        }
        appendLog("all bonded OSAIG devices failed");
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
                // Receiver may already be unregistered during Activity shutdown.
            }
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
        }
    }

    private boolean ensureBonded(BluetoothDevice device) {
        if (device.getBondState() == BluetoothDevice.BOND_BONDED) {
            return true;
        }

        CountDownLatch latch = new CountDownLatch(1);
        AtomicBoolean bonded = new AtomicBoolean(false);
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        filter.addAction(BluetoothDevice.ACTION_PAIRING_REQUEST);
        filter.setPriority(1000);
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                BluetoothDevice changed = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (changed == null || !changed.getAddress().equals(device.getAddress())) {
                    return;
                }
                if (BluetoothDevice.ACTION_PAIRING_REQUEST.equals(action)) {
                    int variant = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, -1);
                    appendLog("pairing request " + describe(changed) + " variant=" + variant);
                    boolean handled = confirmPairing(changed, variant);
                    if (handled && isOrderedBroadcast()) {
                        abortBroadcast();
                    }
                    return;
                }
                int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.ERROR);
                appendLog("bond state " + describe(changed) + " state=" + state);
                if (state == BluetoothDevice.BOND_BONDED) {
                    bonded.set(true);
                    latch.countDown();
                } else if (state == BluetoothDevice.BOND_NONE) {
                    latch.countDown();
                }
            }
        };

        registerReceiverCompat(receiver, filter);
        try {
            appendLog("createBond " + describe(device));
            if (!device.createBond()) {
                appendLog("createBond returned false");
                return false;
            }
            try {
                latch.await(BOND_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            return bonded.get() || device.getBondState() == BluetoothDevice.BOND_BONDED;
        } finally {
            try {
                unregisterReceiver(receiver);
            } catch (IllegalArgumentException ignored) {
                // Receiver may already be unregistered during Activity shutdown.
            }
        }
    }

    private boolean confirmPairing(BluetoothDevice device, int variant) {
        if (variant == 0) {
            try {
                Method setPin = device.getClass().getMethod("setPin", byte[].class);
                setPin.invoke(device, "1234".getBytes(StandardCharsets.UTF_8));
                appendLog("setPin invoked");
                return true;
            } catch (Exception e) {
                appendLog("setPin skipped: " + e.getClass().getSimpleName());
            }
        }
        try {
            Method setPairingConfirmation =
                    device.getClass().getMethod("setPairingConfirmation", boolean.class);
            setPairingConfirmation.invoke(device, true);
            appendLog("setPairingConfirmation invoked");
            return true;
        } catch (Exception e) {
            appendLog("setPairingConfirmation skipped: " + e.getClass().getSimpleName());
        }
        return false;
    }

    private List<BluetoothDevice> findPairedOsaigDevices(BluetoothAdapter adapter) {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        List<BluetoothDevice> result = new ArrayList<>();
        appendLog("bonded devices=" + bondedDevices.size());
        for (BluetoothDevice device : bondedDevices) {
            String name = safeName(device);
            appendLog("bonded " + name + " " + device.getAddress());
            if (isOsaigName(name)) {
                result.add(device);
            }
        }
        return result;
    }

    private boolean connectAndSend(BluetoothDevice device, boolean allowFailure) {
        appendLog("connecting to " + describe(device));

        try {
            socket = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
            socket.connect();
            appendLog("connected");

            Thread reader = new Thread(() -> readLoop(socket), "spp-reader");
            reader.start();

            OutputStream out = socket.getOutputStream();
            for (int i = 1; i <= 5; i++) {
                String payload = "OSAIG_SPP_FD_PROBE_RX_" + i + " from android\n";
                byte[] data = payload.getBytes(StandardCharsets.UTF_8);
                out.write(data);
                out.flush();
                appendLog("TX " + payload.trim());
                Thread.sleep(1200);
            }

            Thread.sleep(10000);
            closeSocket();
            appendLog("test finished");
            return true;
        } catch (Exception e) {
            appendLog("connect/test failed for " + describe(device) + ": "
                    + e.getClass().getSimpleName() + ": " + e.getMessage());
            closeSocket();
            return false;
        }
    }

    private void registerReceiverCompat(BroadcastReceiver receiver, IntentFilter filter) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(receiver, filter);
        }
    }

    private boolean isOsaigName(String name) {
        return name.startsWith("OSAIG-") || name.equals("OSAIG");
    }

    private String describe(BluetoothDevice device) {
        return safeName(device) + " " + device.getAddress();
    }

    private String safeName(BluetoothDevice device) {
        String name = device.getName();
        return name == null ? "" : name;
    }

    private void readLoop(BluetoothSocket activeSocket) {
        byte[] buffer = new byte[512];
        try {
            InputStream in = activeSocket.getInputStream();
            while (activeSocket.isConnected()) {
                int len = in.read(buffer);
                if (len < 0) {
                    appendLog("RX EOF");
                    return;
                }
                String text = new String(buffer, 0, len, StandardCharsets.UTF_8);
                appendLog("RX len=" + len + " text=" + text.replace("\n", "\\n"));
            }
        } catch (IOException e) {
            appendLog("reader stopped: " + e.getClass().getSimpleName() + ": " + e.getMessage());
        }
    }

    private synchronized void closeSocket() {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ignored) {
                // Best-effort cleanup for a test client.
            }
            socket = null;
        }
    }

    private void appendLog(String line) {
        Log.i(TAG, line);
        runOnUiThread(() -> {
            logView.append(line + "\n");
            int scrollAmount = logView.getLayout() == null ? 0 :
                    logView.getLayout().getLineTop(logView.getLineCount()) - logView.getHeight();
            if (scrollAmount > 0) {
                logView.scrollTo(0, scrollAmount);
            }
        });
    }
}
