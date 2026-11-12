package com.monty.wlc.taskbar

import android.content.Intent
import android.os.Bundle
import android.view.Gravity
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var bar: TaskbarView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        bar = TaskbarView(this)
        /* floating taskbar overlay via WindowManager, but for MVP just show it as content */
        setContentView(bar)

        startService(Intent(this, BridgeService::class.java))
    }

    override fun onResume() {
        super.onResume()
        val svc = BridgeService.instance ?: return
        svc.onWindows = { list -> runOnUiThread { bar.setWindows(list) } }
        bar.onActivate = { h -> svc.activate(h) }
        bar.onClose = { h -> svc.close(h) }
        svc.requestList()
    }
}
