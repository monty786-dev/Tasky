package com.monty.wlc.taskbar

data class WinInfo(
    val hwnd: Long,
    val flags: Int,
    val title: String,
    val cls: String,
    val proc: String
) {
    val isActive: Boolean   get() = flags and 1 != 0
    val isMin: Boolean      get() = flags and 2 != 0
    val isMax: Boolean      get() = flags and 4 != 0
}
