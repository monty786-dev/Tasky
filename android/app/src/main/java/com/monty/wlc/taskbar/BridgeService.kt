package com.monty.wlc.taskbar

import android.app.*
import android.content.Intent
import android.os.Build
import android.os.IBinder
import kotlinx.coroutines.*
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder

class BridgeService : Service() {

    companion object {
        const val PORT = 47921
        const val MAGIC = 0x574C4342u
        const val VER = 1
        const val MSG_WIN_LIST_REQ = 10
        const val MSG_WIN_LIST     = 11
        const val MSG_WIN_ACTIVATE = 20
        const val MSG_WIN_MINIMIZE = 21
        const val MSG_WIN_CLOSE    = 22

        var instance: BridgeService? = null
            private set
    }

    private var sock: Socket? = null
    private var out: DataOutputStream? = null
    private var job: Job? = null
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    var onWindows: ((List<WinInfo>) -> Unit)? = null
    var connected: Boolean = false
        private set

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        instance = this
        startForeground(1, buildNotification())
        connectLoop()
    }

    override fun onDestroy() {
        instance = null
        job?.cancel()
        scope.cancel()
        runCatching { sock?.close() }
        super.onDestroy()
    }

    private fun buildNotification(): Notification {
        val ch = "wlc_bridge"
        if (Build.VERSION.SDK_INT >= 26) {
            val mgr = getSystemService(NotificationManager::class.java)
            if (mgr.getNotificationChannel(ch) == null)
                mgr.createNotificationChannel(NotificationChannel(ch, "Bridge", NotificationManager.IMPORTANCE_LOW))
        }
        return Notification.Builder(this, ch)
            .setContentTitle("WLC Taskbar")
            .setContentText("Connected to Wine bridge")
            .setSmallIcon(android.R.drawable.ic_menu_view)
            .build()
    }

    private fun connectLoop() {
        job = scope.launch {
            while (isActive) {
                try {
                    val s = Socket()
                    s.connect(InetSocketAddress("127.0.0.1", PORT), 2000)
                    sock = s
                    out = DataOutputStream(s.getOutputStream())
                    connected = true
                    requestList()
                    readLoop(s)
                } catch (e: Exception) {
                    connected = false
                }
                delay(1000)
            }
        }
    }

    private fun readLoop(s: Socket) {
        val inp = DataInputStream(s.getInputStream())
        val hdr = ByteArray(16)
        while (!s.isClosed) {
            inp.readFully(hdr)
            val bb = ByteBuffer.wrap(hdr).order(ByteOrder.LITTLE_ENDIAN)
            if (bb.int != MAGIC.toInt()) break
            bb.int  /* version */
            val type = bb.int
            val len = bb.int
            val payload = ByteArray(len)
            if (len > 0) inp.readFully(payload)
            handle(type, payload)
        }
    }

    private fun handle(type: Int, payload: ByteArray) {
        if (type == MSG_WIN_LIST) {
            val list = parseWinList(payload)
            onWindows?.invoke(list)
        }
    }

    private fun parseWinList(p: ByteArray): List<WinInfo> {
        val bb = ByteBuffer.wrap(p).order(ByteOrder.LITTLE_ENDIAN)
        val n = bb.int
        val out = ArrayList<WinInfo>(n)
        repeat(n) {
            val hwnd = bb.long
            val flags = bb.int
            val tl = bb.int
            val cl = bb.int
            val pl = bb.int
            bb.int  /* iconRGBA */
            val title = readUtf16(bb, tl)
            val cls   = readUtf16(bb, cl)
            val proc  = readUtf16(bb, pl)
            out += WinInfo(hwnd, flags, title, cls, proc)
        }
        return out
    }

    private fun readUtf16(bb: ByteBuffer, chars: Int): String {
        val bytes = ByteArray(chars * 2)
        bb.get(bytes)
        return String(bytes, Charsets.UTF_16LE)
    }

    fun requestList() {
        send(MSG_WIN_LIST_REQ, ByteArray(0))
    }
    fun activate(hwnd: Long) = send(MSG_WIN_ACTIVATE, longLe(hwnd))
    fun minimize(hwnd: Long) = send(MSG_WIN_MINIMIZE, longLe(hwnd))
    fun close(hwnd: Long)    = send(MSG_WIN_CLOSE,    longLe(hwnd))

    private fun longLe(v: Long) =
        ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(v).array()

    @Synchronized
    private fun send(type: Int, payload: ByteArray) {
        val o = out ?: return
        try {
            val hdr = ByteBuffer.allocate(16).order(ByteOrder.LITTLE_ENDIAN)
                .putInt(MAGIC.toInt()).putInt(VER).putInt(type).putInt(payload.size).array()
            o.write(hdr); o.write(payload); o.flush()
        } catch (_: Exception) { connected = false }
    }
}
