package com.osaig.sdk.transport.benchmark.demo;

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
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.Closeable;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
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
import java.util.concurrent.atomic.AtomicReference;
import java.util.zip.CRC32;

public class MainActivity extends Activity {
    private static final String TAG = "TransportBenchmark";
    private static final UUID SPP_UUID =
            UUID.fromString("00001911-0000-1000-8000-00805f9b34fb");
    private static final int SPP_RFCOMM_CHANNEL = 10;
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1001;
    private static final int DEFAULT_PAYLOAD_SIZE = 204800;
    private static final int DISCOVERY_TIMEOUT_MS = 15000;
    private static final int BOND_TIMEOUT_MS = 30000;
    private static final int WIFI_ACCEPT_TIMEOUT_MS = 12000;
    private static final int WIFI_IO_TIMEOUT_MS = 15000;
    private static final int LINE_LIMIT = 512;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final AtomicBoolean busy = new AtomicBoolean(false);

    private TextView statusView;
    private TextView resultView;
    private TextView wifiView;
    private TextView logView;
    private ScrollView logScroll;
    private Button connectButton;
    private Button disconnectButton;
    private Button prepareWifiButton;
    private Button runSppButton;
    private Button runWifiButton;

    private BluetoothSocket sppSocket;
    private InputStream sppInput;
    private OutputStream sppOutput;
    private ServerSocket wifiServerSocket;
    private String wifiHost;
    private int wifiPort;
    private int requestSeq;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildContentView());
        setStatus("Idle");
        setResult("等待连接");
        setWifiInfo("Wi-Fi 监听未准备");
        updateButtons();

        if (!hasBluetoothPermissions()) {
            requestBluetoothPermissions();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSIONS) {
            if (hasBluetoothPermissions()) {
                appendLog("蓝牙权限已授予");
            } else {
                appendLog("蓝牙权限未授予");
            }
            updateButtons();
        }
    }

    @Override
    protected void onDestroy() {
        closeWifiServer();
        closeSppSocket();
        executor.shutdownNow();
        super.onDestroy();
    }

    private View buildContentView() {
        int padding = dp(16);
        int buttonGap = dp(8);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, padding, padding, padding);

        statusView = new TextView(this);
        statusView.setTextSize(18);
        root.addView(statusView, matchWrap());

        resultView = new TextView(this);
        resultView.setTextSize(14);
        resultView.setPadding(0, dp(8), 0, 0);
        root.addView(resultView, matchWrap());

        wifiView = new TextView(this);
        wifiView.setTextSize(13);
        wifiView.setPadding(0, dp(6), 0, dp(10));
        root.addView(wifiView, matchWrap());

        LinearLayout row1 = new LinearLayout(this);
        row1.setOrientation(LinearLayout.HORIZONTAL);
        connectButton = makeButton("Connect SPP", v -> connectSpp());
        disconnectButton = makeButton("Disconnect", v -> disconnectAll());
        row1.addView(connectButton, weight(buttonGap));
        row1.addView(disconnectButton, weight(0));
        root.addView(row1, matchWrap());

        LinearLayout row2 = new LinearLayout(this);
        row2.setOrientation(LinearLayout.HORIZONTAL);
        row2.setPadding(0, buttonGap, 0, 0);
        prepareWifiButton = makeButton("Prepare Wi-Fi", v -> prepareWifiListener());
        runSppButton = makeButton("Run SPP Transfer", v -> runSppTransfer());
        row2.addView(prepareWifiButton, weight(buttonGap));
        row2.addView(runSppButton, weight(0));
        root.addView(row2, matchWrap());

        LinearLayout row3 = new LinearLayout(this);
        row3.setOrientation(LinearLayout.HORIZONTAL);
        row3.setPadding(0, buttonGap, 0, 0);
        runWifiButton = makeButton("Run Wi-Fi Transfer", v -> runWifiTransfer());
        Button clearButton = makeButton("Clear Log", v -> logView.setText(""));
        row3.addView(runWifiButton, weight(buttonGap));
        row3.addView(clearButton, weight(0));
        root.addView(row3, matchWrap());

        logScroll = new ScrollView(this);
        logScroll.setFillViewport(true);
        logView = new TextView(this);
        logView.setTextSize(12);
        logView.setTextIsSelectable(true);
        logView.setPadding(0, dp(12), 0, 0);
        logScroll.addView(logView, matchWrap());
        root.addView(logScroll, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        return root;
    }

    private LinearLayout.LayoutParams matchWrap() {
        return new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
    }

    private LinearLayout.LayoutParams weight(int rightMargin) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        params.rightMargin = rightMargin;
        return params;
    }

    private Button makeButton(String text, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setAllCaps(false);
        button.setText(text);
        button.setOnClickListener(listener);
        return button;
    }

    private int dp(int value) {
        return Math.round(getResources().getDisplayMetrics().density * value);
    }

    private boolean hasBluetoothPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            return checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    == PackageManager.PERMISSION_GRANTED
                    && checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)
                    == PackageManager.PERMISSION_GRANTED;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)
                    == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }

    private void requestBluetoothPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            requestPermissions(new String[]{
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Manifest.permission.BLUETOOTH_SCAN
            }, REQUEST_BLUETOOTH_PERMISSIONS);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{
                    Manifest.permission.ACCESS_FINE_LOCATION
            }, REQUEST_BLUETOOTH_PERMISSIONS);
        }
    }

    private void runBusyTask(String name, TaskRunnable runnable) {
        if (!busy.compareAndSet(false, true)) {
            appendLog("已有任务执行中: " + name);
            return;
        }
        updateButtons();
        executor.execute(() -> {
            try {
                runnable.run();
            } catch (Exception e) {
                appendLog(name + " 失败: " + e.getClass().getSimpleName() + ": " + e.getMessage());
                Log.e(TAG, name + " failed", e);
            } finally {
                busy.set(false);
                runOnUiThreadSafe(this::updateButtons);
            }
        });
    }

    private void connectSpp() {
        if (!hasBluetoothPermissions()) {
            appendLog("缺少蓝牙权限，先授权");
            requestBluetoothPermissions();
            return;
        }
        runBusyTask("connect_spp", this::connectSppInternal);
    }

    private void prepareWifiListener() {
        runBusyTask("prepare_wifi", this::prepareWifiListenerInternal);
    }

    private void runSppTransfer() {
        runBusyTask("run_spp_transfer", this::runSppTransferInternal);
    }

    private void runWifiTransfer() {
        runBusyTask("run_wifi_transfer", this::runWifiTransferInternal);
    }

    private void disconnectAll() {
        closeWifiServer();
        closeSppSocket();
        setStatus("Disconnected");
        setWifiInfo("Wi-Fi 监听未准备");
        appendLog("已断开 SPP 和 Wi-Fi 监听");
        updateButtons();
    }

    private void connectSppInternal() {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            appendLog("BluetoothAdapter 不可用");
            return;
        }
        if (!adapter.isEnabled()) {
            appendLog("蓝牙未打开");
            return;
        }
        if (isSppConnected()) {
            appendLog("SPP 已连接");
            setStatus("SPP connected");
            return;
        }

        setStatus("发现眼镜中...");
        BluetoothDevice discovered = discoverOsaigDevice(adapter);
        if (discovered != null) {
            appendLog("发现设备 " + describe(discovered));
            if (connectToDevice(discovered)) {
                return;
            }
            appendLog("直连失败，尝试配对后重连 " + describe(discovered));
            if (ensureBonded(discovered) && connectToDevice(discovered)) {
                return;
            }
        }

        List<BluetoothDevice> bondedDevices = findPairedOsaigDevices(adapter);
        if (bondedDevices.isEmpty()) {
            appendLog("没有找到可用的 OSAIG 设备");
            setStatus("未发现设备");
            return;
        }

        if (bondedDevices.size() > 1) {
            appendLog("发现失败且存在多个已配对 OSAIG 设备，停止自动回退以避免误连");
            setStatus("SPP connect failed");
            return;
        }

        for (BluetoothDevice device : bondedDevices) {
            if (connectToDevice(device)) {
                return;
            }
        }

        setStatus("SPP connect failed");
    }

    private BluetoothDevice discoverOsaigDevice(BluetoothAdapter adapter) {
        AtomicReference<BluetoothDevice> found = new AtomicReference<>();
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
                    appendLog("发现 " + name + " " + device.getAddress());
                    if (isOsaigName(name) && found.compareAndSet(null, device)) {
                        adapter.cancelDiscovery();
                        latch.countDown();
                    }
                } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                    appendLog("设备发现结束");
                    latch.countDown();
                }
            }
        };

        registerReceiverCompat(receiver, filter);
        try {
            if (adapter.isDiscovering()) {
                adapter.cancelDiscovery();
            }
            if (!adapter.startDiscovery()) {
                appendLog("startDiscovery 返回 false");
                return null;
            }
            try {
                latch.await(DISCOVERY_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            return found.get();
        } finally {
            try {
                unregisterReceiver(receiver);
            } catch (IllegalArgumentException ignored) {
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
                    appendLog("配对请求 " + describe(changed) + " variant=" + variant);
                    confirmPairing(changed, variant);
                    if (isOrderedBroadcast()) {
                        abortBroadcast();
                    }
                    return;
                }
                int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.ERROR);
                appendLog("配对状态 " + describe(changed) + " state=" + state);
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
            appendLog("开始配对 " + describe(device));
            if (!device.createBond()) {
                appendLog("createBond 返回 false");
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
            }
        }
    }

    private void confirmPairing(BluetoothDevice device, int variant) {
        if (variant == 0) {
            try {
                Method setPin = device.getClass().getMethod("setPin", byte[].class);
                setPin.invoke(device, "1234".getBytes(StandardCharsets.UTF_8));
                appendLog("已尝试 setPin 1234");
            } catch (Exception e) {
                appendLog("setPin 跳过: " + e.getClass().getSimpleName());
            }
        }
        try {
            Method setPairingConfirmation =
                    device.getClass().getMethod("setPairingConfirmation", boolean.class);
            setPairingConfirmation.invoke(device, true);
            appendLog("已确认配对");
        } catch (Exception e) {
            appendLog("setPairingConfirmation 跳过: " + e.getClass().getSimpleName());
        }
    }

    private List<BluetoothDevice> findPairedOsaigDevices(BluetoothAdapter adapter) {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        List<BluetoothDevice> result = new ArrayList<>();
        appendLog("已配对设备数=" + bondedDevices.size());
        for (BluetoothDevice device : bondedDevices) {
            String name = safeName(device);
            appendLog("已配对 " + name + " " + device.getAddress());
            if (isOsaigName(name)) {
                result.add(device);
            }
        }
        return result;
    }

    private boolean connectToDevice(BluetoothDevice device) {
        appendLog("连接设备 " + describe(device));
        closeSppSocket();

        BluetoothSocket socket = null;
        IOException lastError = null;
        try {
            socket = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
            socket.connect();
        } catch (IOException e) {
            lastError = e;
            safeClose(socket);
            appendLog("UUID SPP 连接失败，尝试 channel 10: " + e.getMessage());
            try {
                socket = createInsecureSocketByChannel(device, SPP_RFCOMM_CHANNEL);
                socket.connect();
                lastError = null;
            } catch (Exception channelError) {
                safeClose(socket);
                appendLog("channel 10 连接失败: " + channelError.getMessage());
                if (channelError instanceof IOException) {
                    lastError = (IOException) channelError;
                } else {
                    lastError = new IOException(channelError);
                }
            }
        }

        if (lastError != null || socket == null) {
            appendLog("SPP 连接失败 " + describe(device));
            return false;
        }

        try {
            sppInput = socket.getInputStream();
            sppOutput = socket.getOutputStream();
            sppSocket = socket;
            setStatus("SPP connected");
            appendLog("SPP 已连接 " + describe(device));
            updateButtons();
            return true;
        } catch (IOException e) {
            appendLog("获取流失败: " + e.getMessage());
            safeClose(socket);
            sppInput = null;
            sppOutput = null;
            sppSocket = null;
            return false;
        }
    }

    private BluetoothSocket createInsecureSocketByChannel(BluetoothDevice device, int channel)
            throws Exception {
        Method method = device.getClass().getMethod("createInsecureRfcommSocket", int.class);
        return (BluetoothSocket) method.invoke(device, channel);
    }

    private void prepareWifiListenerInternal() {
        String host = findBestIpv4Address();
        if (host == null) {
            setWifiInfo("未找到可用 IPv4，请先让手机连入与眼镜同一 Wi-Fi");
            appendLog("未找到可用 IPv4");
            return;
        }

        ServerSocket server = null;
        try {
            server = new ServerSocket();
            server.setReuseAddress(true);
            server.bind(new InetSocketAddress(0));
            server.setSoTimeout(WIFI_ACCEPT_TIMEOUT_MS);
            synchronized (this) {
                closeWifiServerLocked();
                wifiServerSocket = server;
                wifiHost = host;
                wifiPort = server.getLocalPort();
            }
            setWifiInfo("监听 " + host + ":" + wifiPort);
            appendLog("Wi-Fi 监听已准备 " + host + ":" + wifiPort);
        } catch (IOException e) {
            safeClose(server);
            appendLog("准备 Wi-Fi 监听失败: " + e.getMessage());
        }
    }

    private void runSppTransferInternal() throws Exception {
        requireSppConnected();

        String requestId = nextRequestId("spp");
        String command = String.format(Locale.US,
                "SPP_SEND request=%s size=%d\n", requestId, DEFAULT_PAYLOAD_SIZE);

        appendLog("发送 SPP 测试 request=" + requestId + " size=" + DEFAULT_PAYLOAD_SIZE);
        long commandStartMs = SystemClock.elapsedRealtime();
        writeAsciiLine(sppOutput, command);

        ReceiveReport receive = receiveFrame(sppInput, "spp", requestId);
        long ackStartMs = SystemClock.elapsedRealtime();
        sendAck(sppOutput, requestId, receive.payloadReceiveMs,
                ackStartMs - commandStartMs, receive.bytes, receive.crc32);
        long ackSentMs = SystemClock.elapsedRealtime();

        DoneReport done = readDoneReport(sppInput, requestId, "spp");
        long endToEndMs = SystemClock.elapsedRealtime() - commandStartMs;

        String summary = String.format(Locale.US,
                "SPP size=%d crc32=%08x payload=%dms local_total=%dms glasses_total=%dms "
                        + "prep=%dms data=%dms ack=%dms",
                receive.bytes,
                receive.crc32,
                receive.payloadReceiveMs,
                endToEndMs,
                done.totalMs,
                done.prepMs,
                done.dataMs,
                done.ackMs);
        setResult(summary);
        appendLog(summary);
        appendLog(String.format(Locale.US,
                "SPP 细分 header_wait=%dms ack_send=%dms reason=%s",
                receive.headerWaitMs,
                ackSentMs - ackStartMs,
                done.reason));

        if (!"ok".equals(done.status)) {
            appendLog("SPP 眼镜端返回错误 status=" + done.status + " reason=" + done.reason);
        }
        if (done.totalMs > 100) {
            appendLog(String.format(Locale.US,
                    "WARN: SPP 超过 100ms，需要排查。total=%dms prep=%dms data=%dms ack=%dms",
                    done.totalMs, done.prepMs, done.dataMs, done.ackMs));
        }
    }

    private void runWifiTransferInternal() throws Exception {
        requireSppConnected();

        ServerSocket server;
        String host;
        int port;
        synchronized (this) {
            server = wifiServerSocket;
            host = wifiHost;
            port = wifiPort;
        }
        if (server == null || host == null || port <= 0) {
            throw new IOException("Wi-Fi 监听未准备");
        }

        String requestId = nextRequestId("wifi");
        String command = String.format(Locale.US,
                "WIFI_SEND request=%s host=%s port=%d size=%d\n",
                requestId, host, port, DEFAULT_PAYLOAD_SIZE);

        appendLog("发送 Wi-Fi 测试 request=" + requestId + " target=" + host + ":" + port
                + " size=" + DEFAULT_PAYLOAD_SIZE);
        long commandStartMs = SystemClock.elapsedRealtime();
        writeAsciiLine(sppOutput, command);

        Socket dataSocket = null;
        try {
            dataSocket = server.accept();
            dataSocket.setSoTimeout(WIFI_IO_TIMEOUT_MS);
            dataSocket.setTcpNoDelay(true);
            String peer = dataSocket.getInetAddress() == null
                    ? "unknown"
                    : dataSocket.getInetAddress().getHostAddress();
            appendLog("Wi-Fi 已接入 peer=" + peer);

            InputStream tcpIn = dataSocket.getInputStream();
            OutputStream tcpOut = dataSocket.getOutputStream();
            ReceiveReport receive = receiveFrame(tcpIn, "wifi", requestId);
            long ackStartMs = SystemClock.elapsedRealtime();
            sendAck(tcpOut, requestId, receive.payloadReceiveMs,
                    ackStartMs - commandStartMs, receive.bytes, receive.crc32);
            long ackSentMs = SystemClock.elapsedRealtime();
            DoneReport done = readDoneReport(sppInput, requestId, "wifi");
            long endToEndMs = SystemClock.elapsedRealtime() - commandStartMs;

            String summary = String.format(Locale.US,
                    "Wi-Fi size=%d crc32=%08x payload=%dms local_total=%dms glasses_total=%dms "
                            + "connect=%dms prep=%dms data=%dms ack=%dms",
                    receive.bytes,
                    receive.crc32,
                    receive.payloadReceiveMs,
                    endToEndMs,
                    done.totalMs,
                    done.connectMs,
                    done.prepMs,
                    done.dataMs,
                    done.ackMs);
            setResult(summary);
            appendLog(summary);
            appendLog(String.format(Locale.US,
                    "Wi-Fi 细分 header_wait=%dms ack_send=%dms reason=%s",
                    receive.headerWaitMs,
                    ackSentMs - ackStartMs,
                    done.reason));

            if (!"ok".equals(done.status)) {
                appendLog("Wi-Fi 眼镜端返回错误 status=" + done.status + " reason=" + done.reason);
            }
        } catch (IOException acceptError) {
            DoneReport maybeDone = tryReadDoneReportAfterAcceptFailure(requestId, "wifi");
            if (maybeDone != null) {
                appendLog("Wi-Fi accept 失败，但眼镜端已返回 status=" + maybeDone.status
                        + " reason=" + maybeDone.reason);
            }
            throw acceptError;
        } finally {
            safeClose(dataSocket);
        }
    }

    private DoneReport tryReadDoneReportAfterAcceptFailure(String requestId, String channel) {
        long deadline = SystemClock.elapsedRealtime() + 1500;
        while (SystemClock.elapsedRealtime() < deadline) {
            try {
                if (sppInput != null && sppInput.available() > 0) {
                    return readDoneReport(sppInput, requestId, channel);
                }
            } catch (Exception ignored) {
                return null;
            }
            SystemClock.sleep(50);
        }
        return null;
    }

    private void requireSppConnected() throws IOException {
        if (!isSppConnected() || sppInput == null || sppOutput == null) {
            throw new IOException("SPP 未连接");
        }
    }

    private boolean isSppConnected() {
        return sppSocket != null && sppSocket.isConnected();
    }

    private synchronized String nextRequestId(String prefix) {
        requestSeq++;
        return prefix + "_" + requestSeq;
    }

    private ReceiveReport receiveFrame(InputStream in, String expectedChannel, String requestId)
            throws IOException {
        long waitStartMs = SystemClock.elapsedRealtime();
        String headerLine = readAsciiLine(in, LINE_LIMIT);
        long headerReceivedMs = SystemClock.elapsedRealtime();

        Map<String, String> headerMap = parseKeyValueLine(headerLine, "OSAIG_BENCH_V1");
        String channel = requireValue(headerMap, "channel");
        String request = requireValue(headerMap, "request");
        int size = parsePositiveInt(requireValue(headerMap, "size"));
        long expectedCrc = parseHexLong(requireValue(headerMap, "crc32"));

        if (!expectedChannel.equals(channel)) {
            throw new IOException("channel mismatch " + channel);
        }
        if (!requestId.equals(request)) {
            throw new IOException("request mismatch " + request);
        }

        CRC32 crc32 = new CRC32();
        byte[] buffer = new byte[8192];
        int remaining = size;
        while (remaining > 0) {
            int toRead = Math.min(buffer.length, remaining);
            int read;
            try {
                read = in.read(buffer, 0, toRead);
            } catch (IOException e) {
                int received = size - remaining;
                throw new IOException("payload read failed at " + received + "/" + size
                        + ": " + e.getMessage(), e);
            }
            if (read < 0) {
                int received = size - remaining;
                throw new EOFException("payload eof at " + received + "/" + size);
            }
            crc32.update(buffer, 0, read);
            remaining -= read;
        }
        long payloadDoneMs = SystemClock.elapsedRealtime();
        long actualCrc = crc32.getValue() & 0xFFFFFFFFL;
        if (actualCrc != expectedCrc) {
            throw new IOException(String.format(Locale.US,
                    "crc mismatch expected=%08x actual=%08x", expectedCrc, actualCrc));
        }

        ReceiveReport report = new ReceiveReport();
        report.bytes = size;
        report.crc32 = actualCrc;
        report.headerWaitMs = headerReceivedMs - waitStartMs;
        report.payloadReceiveMs = payloadDoneMs - headerReceivedMs;
        report.totalReceiveMs = payloadDoneMs - waitStartMs;
        return report;
    }

    private void sendAck(OutputStream out,
                         String requestId,
                         long receiveMs,
                         long totalMs,
                         long bytes,
                         long crc32) throws IOException {
        String ack = String.format(Locale.US,
                "OSAIG_BENCH_ACK request=%s status=ok recv_ms=%d total_ms=%d bytes=%d crc32=%08x\n",
                requestId, receiveMs, totalMs, bytes, crc32);
        writeAsciiLine(out, ack);
    }

    private DoneReport readDoneReport(InputStream in, String expectedRequest, String expectedChannel)
            throws IOException {
        String line = readAsciiLine(in, LINE_LIMIT);
        Map<String, String> map = parseKeyValueLine(line, "OSAIG_BENCH_DONE");
        DoneReport report = new DoneReport();
        report.requestId = requireValue(map, "request");
        report.channel = requireValue(map, "channel");
        report.status = requireValue(map, "status");
        report.totalMs = parsePositiveInt(requireValue(map, "total_ms"));
        report.prepMs = parsePositiveInt(requireValue(map, "prep_ms"));
        report.connectMs = parsePositiveInt(requireValue(map, "connect_ms"));
        report.dataMs = parsePositiveInt(requireValue(map, "data_ms"));
        report.ackMs = parsePositiveInt(requireValue(map, "ack_ms"));
        report.bytes = parsePositiveInt(requireValue(map, "bytes"));
        report.crc32 = parseHexLong(requireValue(map, "crc32"));
        report.reason = map.containsKey("reason") ? map.get("reason") : "";

        if (!expectedRequest.equals(report.requestId)) {
            throw new IOException("done request mismatch " + report.requestId);
        }
        if (!expectedChannel.equals(report.channel)) {
            throw new IOException("done channel mismatch " + report.channel);
        }
        return report;
    }

    private Map<String, String> parseKeyValueLine(String line, String expectedPrefix)
            throws IOException {
        String[] parts = line.trim().split("\\s+");
        if (parts.length == 0 || !expectedPrefix.equals(parts[0])) {
            throw new IOException("unexpected line: " + line);
        }
        Map<String, String> map = new HashMap<>();
        for (int i = 1; i < parts.length; i++) {
            int equalsIndex = parts[i].indexOf('=');
            if (equalsIndex <= 0 || equalsIndex >= parts[i].length() - 1) {
                continue;
            }
            map.put(parts[i].substring(0, equalsIndex), parts[i].substring(equalsIndex + 1));
        }
        return map;
    }

    private String requireValue(Map<String, String> map, String key) throws IOException {
        String value = map.get(key);
        if (value == null || value.isEmpty()) {
            throw new IOException("missing key " + key);
        }
        return value;
    }

    private int parsePositiveInt(String value) throws IOException {
        try {
            int parsed = Integer.parseInt(value);
            if (parsed < 0) {
                throw new IOException("negative value " + value);
            }
            return parsed;
        } catch (NumberFormatException e) {
            throw new IOException("invalid integer " + value, e);
        }
    }

    private long parseHexLong(String value) throws IOException {
        try {
            return Long.parseLong(value, 16) & 0xFFFFFFFFL;
        } catch (NumberFormatException e) {
            throw new IOException("invalid hex " + value, e);
        }
    }

    private String readAsciiLine(InputStream in, int limit) throws IOException {
        byte[] line = new byte[limit];
        int used = 0;
        while (used + 1 < limit) {
            int value = in.read();
            if (value < 0) {
                throw new EOFException("line eof");
            }
            if (value == '\r') {
                continue;
            }
            line[used++] = (byte) value;
            if (value == '\n') {
                return new String(line, 0, used, StandardCharsets.UTF_8);
            }
        }
        throw new IOException("line too long");
    }

    private void writeAsciiLine(OutputStream out, String text) throws IOException {
        byte[] data = text.getBytes(StandardCharsets.UTF_8);
        out.write(data);
        out.flush();
    }

    private String findBestIpv4Address() {
        List<Inet4Address> preferred = new ArrayList<>();
        List<Inet4Address> fallback = new ArrayList<>();
        try {
            Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
            if (interfaces == null) {
                return null;
            }
            while (interfaces.hasMoreElements()) {
                NetworkInterface network = interfaces.nextElement();
                try {
                    if (!network.isUp() || network.isLoopback() || network.isVirtual()) {
                        continue;
                    }
                } catch (SocketException ignored) {
                    continue;
                }

                String name = network.getName() == null ? "" : network.getName().toLowerCase(Locale.US);
                Enumeration<InetAddress> addresses = network.getInetAddresses();
                while (addresses.hasMoreElements()) {
                    InetAddress address = addresses.nextElement();
                    if (!(address instanceof Inet4Address) || address.isLoopbackAddress()) {
                        continue;
                    }
                    Inet4Address ipv4 = (Inet4Address) address;
                    if (name.startsWith("wlan") || name.startsWith("ap")
                            || name.startsWith("eth") || name.startsWith("en")) {
                        preferred.add(ipv4);
                    } else {
                        fallback.add(ipv4);
                    }
                }
            }
        } catch (SocketException e) {
            appendLog("获取本机 IPv4 失败: " + e.getMessage());
            return null;
        }

        if (!preferred.isEmpty()) {
            return preferred.get(0).getHostAddress();
        }
        if (!fallback.isEmpty()) {
            return fallback.get(0).getHostAddress();
        }
        return null;
    }

    private void registerReceiverCompat(BroadcastReceiver receiver, IntentFilter filter) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Bluetooth discovery broadcasts may come from the platform Bluetooth app
            // instead of the system UID; NOT_EXPORTED can miss them on some Android 13+ builds.
            registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            registerReceiver(receiver, filter);
        }
    }

    private boolean isOsaigName(String name) {
        return name != null && (name.startsWith("OSAIG-") || name.equals("OSAIG"));
    }

    private String safeName(BluetoothDevice device) {
        String name = device.getName();
        return name == null ? "" : name;
    }

    private String describe(BluetoothDevice device) {
        return safeName(device) + " " + device.getAddress();
    }

    private void closeSppSocket() {
        BluetoothSocket socketToClose;
        synchronized (this) {
            socketToClose = sppSocket;
            sppSocket = null;
            sppInput = null;
            sppOutput = null;
        }
        safeClose(socketToClose);
    }

    private void closeWifiServer() {
        synchronized (this) {
            closeWifiServerLocked();
        }
    }

    private void closeWifiServerLocked() {
        ServerSocket socketToClose = wifiServerSocket;
        wifiServerSocket = null;
        wifiHost = null;
        wifiPort = 0;
        safeClose(socketToClose);
    }

    private void safeClose(BluetoothSocket socket) {
        if (socket == null) {
            return;
        }
        try {
            socket.close();
        } catch (IOException ignored) {
        }
    }

    private void safeClose(ServerSocket socket) {
        if (socket == null) {
            return;
        }
        try {
            socket.close();
        } catch (IOException ignored) {
        }
    }

    private void safeClose(Socket socket) {
        if (socket == null) {
            return;
        }
        try {
            socket.close();
        } catch (IOException ignored) {
        }
    }

    private void safeClose(Closeable closeable) {
        if (closeable == null) {
            return;
        }
        try {
            closeable.close();
        } catch (IOException ignored) {
        }
    }

    private void setStatus(String text) {
        runOnUiThreadSafe(() -> statusView.setText(text));
    }

    private void setResult(String text) {
        runOnUiThreadSafe(() -> resultView.setText(text));
    }

    private void setWifiInfo(String text) {
        runOnUiThreadSafe(() -> wifiView.setText(text));
    }

    private void appendLog(String text) {
        Log.i(TAG, text);
        runOnUiThreadSafe(() -> {
            logView.append(text);
            logView.append("\n");
            logScroll.post(() -> logScroll.fullScroll(View.FOCUS_DOWN));
        });
    }

    private void updateButtons() {
        boolean hasPermission = hasBluetoothPermissions();
        boolean connected = isSppConnected();
        boolean running = busy.get();
        boolean wifiReady = wifiServerSocket != null && wifiHost != null && wifiPort > 0;

        connectButton.setEnabled(hasPermission && !running && !connected);
        disconnectButton.setEnabled(!running && (connected || wifiReady));
        prepareWifiButton.setEnabled(!running);
        runSppButton.setEnabled(!running && connected);
        runWifiButton.setEnabled(!running && connected && wifiReady);
    }

    private void runOnUiThreadSafe(Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
        } else {
            mainHandler.post(runnable);
        }
    }

    private interface TaskRunnable {
        void run() throws Exception;
    }

    private static final class ReceiveReport {
        int bytes;
        long crc32;
        long headerWaitMs;
        long payloadReceiveMs;
        long totalReceiveMs;
    }

    private static final class DoneReport {
        String requestId;
        String channel;
        String status;
        int totalMs;
        int prepMs;
        int connectMs;
        int dataMs;
        int ackMs;
        int bytes;
        long crc32;
        String reason;
    }
}
