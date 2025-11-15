package org.verya.QWalkieTalkie

import android.util.Log
import android.content.Context

object TestBridge {
    @JvmStatic
    external fun onMessageFromKotlin(msg: String)

    @JvmStatic
    external fun nativeOnNotificationAction()

    @JvmStatic
    fun notifyCPlusPlus(msg: String) {
        onMessageFromKotlin(msg)
    }

    @JvmStatic
    fun postNotification(ctx: Context, title: String, message: String) {
        val helper = NotificationHelper(ctx)
        helper.show(title, message)
    }
}