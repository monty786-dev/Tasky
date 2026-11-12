package com.monty.wlc.taskbar

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View

class TaskbarView(ctx: Context) : View(ctx) {

    private var wins: List<WinInfo> = emptyList()
    private val barPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xF0202020.toInt() }
    private val btnPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0x00000000 }
    private val activePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0x40FFFFFF }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE; textSize = 28f; isFakeBoldText = false
    }
    private val startPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFF0078D7.toInt() }
    private val rects = ArrayList<RectF>()

    var onActivate: ((Long) -> Unit)? = null
    var onClose:    ((Long) -> Unit)? = null

    fun setWindows(list: List<WinInfo>) {
        wins = list
        requestLayout(); invalidate()
    }

    override fun onMeasure(w: Int, h: Int) {
        val want = (resources.displayMetrics.density * 56).toInt()
        setMeasuredDimension(w, want)
    }

    override fun onDraw(canvas: Canvas) {
        canvas.drawRect(0f, 0f, width.toFloat(), height.toFloat(), barPaint)
        rects.clear()

        /* start button */
        val pad = resources.displayMetrics.density * 8
        val s = height - pad * 2
        canvas.drawRoundRect(pad, pad, pad + s, pad + s, 8f, 8f, startPaint)
        rects += RectF(pad, pad, pad + s, pad + s)   /* index 0 = start */

        var x = pad + s + pad
        for (w in wins) {
            val tw = (textPaint.measureText(w.title) + s + pad * 2).coerceAtMost(width * 0.35f)
            if (x + tw > width - pad) break
            val r = RectF(x, pad, x + tw, height - pad)
            btnPaint.color = if (w.isActive) 0x30FFFFFF else 0x00000000
            canvas.drawRoundRect(r, 6f, 6f, btnPaint)
            if (w.isActive) {
                activePaint.color = 0xFF4CC2FF.toInt()
                canvas.drawRect(x + 8, height - 6f, x + tw - 8, height - 2f, activePaint)
            }
            canvas.drawText(w.title, x + s * 0.4f, height * 0.62f, textPaint)
            rects += r
            x += tw + pad * 0.5f
        }
    }

    override fun onTouchEvent(ev: MotionEvent): Boolean {
        if (ev.action != MotionEvent.ACTION_UP) return true
        val px = ev.x; val py = ev.y
        for (i in rects.indices) {
            if (!rects[i].contains(px, py)) continue
            if (i == 0) return true     /* start menu: TODO */
            val w = wins.getOrNull(i - 1) ?: return true
            if (ev.buttonState and MotionEvent.BUTTON_SECONDARY != 0) onClose?.invoke(w.hwnd)
            else onActivate?.invoke(w.hwnd)
            return true
        }
        return true
    }
}
