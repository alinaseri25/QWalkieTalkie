package org.verya.QWalkieTalkie

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log

class NotificationReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        Log.d("NOTIFY_RECEIVER", "Action clicked!")
        try {
            // Instead of System.loadLibrary(...)  ✅
            TestBridge.nativeOnNotificationAction()
        } catch (e: UnsatisfiedLinkError) {
            TestBridge.notifyCPlusPlus("JNI call failed: ${e.message}")
            Log.e("NOTIFY_RECEIVER", "JNI call failed: ${e.message}")
        } catch (e: Throwable) {
            TestBridge.notifyCPlusPlus("Unexpected: ${e.message}")
            Log.e("NOTIFY_RECEIVER", "Unexpected: ${e.message}")
        }
    }
}
