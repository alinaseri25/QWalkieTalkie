package org.verya.QWalkieTalkie

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.graphics.BitmapFactory
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat

class NotificationHelper(private val context: Context) {

    private val channelId = "qt_channel_primary"

    init {
        createNotificationChannel()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                channelId,
                "Qt JNI Notifications",
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "Notifications generated from the C++ (Qt) layer via JNI"
            }

            val manager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            manager.createNotificationChannel(channel)
        }
    }

    /**
     * نمایش اعلان با دکمه‌ی اکشن (کلیک آن به JNI → C++ Callback می‌رود)
     */
    fun show(title: String, message: String) {
        // Intent و PendingIntent برای action (BroadcastReceiver)
        val actionIntent = Intent(context, NotificationReceiver::class.java).apply {
            action = "org.verya.QWalkieTalkie.ACTION_NOTIFY_CLICK"
        }

        val actionPendingIntent = PendingIntent.getBroadcast(
            context,
            0,
            actionIntent,
            PendingIntent.FLAG_IMMUTABLE
        )

        // Intent برای باز کردن Activity اصلی Qt (روی نوتیف کلیک معمولی)
        val openIntent = Intent(context, org.qtproject.qt.android.bindings.QtActivity::class.java)
        val openPendingIntent = PendingIntent.getActivity(
            context,
            1,
            openIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val builder = NotificationCompat.Builder(context, channelId)
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .setLargeIcon(BitmapFactory.decodeResource(context.resources, android.R.drawable.ic_dialog_info))
            .setContentTitle(title)
            .setContentText(message)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setAutoCancel(true)
            .setContentIntent(openPendingIntent)
            // دکمه اکشن که BroadcastReceiver را فعال می‌کند
            .addAction(android.R.drawable.ic_input_add, "Action", actionPendingIntent)

        // ارسال نوتیفیکیشن
        NotificationManagerCompat.from(context).notify(
            System.currentTimeMillis().toInt(),
            builder.build()
        )
    }
}
