package one.ayugram.desktop;

import android.Manifest;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;

import java.lang.reflect.Method;

public final class AyuGramNotifier {
    private static final String CHANNEL_ID = "ayugram_messages";
    private static final String CHANNEL_NAME = "AyuGram Messages";

    private AyuGramNotifier() {
    }

    private static NotificationManager manager(Context context) {
        return (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    private static void ensureChannel(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return;
        }
        final NotificationManager manager = manager(context);
        if (manager == null) {
            return;
        }
        if (manager.getNotificationChannel(CHANNEL_ID) != null) {
            return;
        }
        final NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID,
                CHANNEL_NAME,
                NotificationManager.IMPORTANCE_DEFAULT);
        channel.enableVibration(true);
        manager.createNotificationChannel(channel);
    }

    private static PendingIntent contentIntent(Context context, int id) {
        final PackageManager pm = context.getPackageManager();
        final Intent launchIntent = pm.getLaunchIntentForPackage(context.getPackageName());
        if (launchIntent == null) {
            return null;
        }
        launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        return PendingIntent.getActivity(
                context,
                id,
                launchIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
    }

    public static void show(
            Context context,
            int id,
            String title,
            String text,
            String subtitle) {
        if (context == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            final Activity activity = resolveActivity(context);
            if (activity != null) {
                AyuGramBridge.requestPermissionSync(
                        activity,
                        Manifest.permission.POST_NOTIFICATIONS,
                        10000);
            }
            if (context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                return;
            }
        }

        ensureChannel(context);
        final NotificationManager manager = manager(context);
        if (manager == null) {
            return;
        }

        final Notification.Builder builder = new Notification.Builder(context, CHANNEL_ID)
                .setAutoCancel(true)
                .setVisibility(Notification.VISIBILITY_PRIVATE)
                .setContentTitle(title == null ? "" : title)
                .setContentText(text == null ? "" : text)
                .setSmallIcon(context.getApplicationInfo().icon != 0
                        ? context.getApplicationInfo().icon
                        : android.R.drawable.sym_action_chat);

        if (subtitle != null && !subtitle.isEmpty()) {
            builder.setSubText(subtitle);
        }

        final PendingIntent intent = contentIntent(context, id);
        if (intent != null) {
            builder.setContentIntent(intent);
        }

        manager.notify(id, builder.build());
    }

    public static void cancel(Context context, int id) {
        if (context == null) {
            return;
        }
        final NotificationManager manager = manager(context);
        if (manager != null) {
            manager.cancel(id);
        }
    }

    public static void clearAll(Context context) {
        if (context == null) {
            return;
        }
        final NotificationManager manager = manager(context);
        if (manager != null) {
            manager.cancelAll();
        }
    }

    private static Activity resolveActivity(Context context) {
        if (context instanceof Activity) {
            return (Activity) context;
        }
        try {
            final Class<?> qtNativeClass = Class.forName("org.qtproject.qt.android.QtNative");
            final Method activityMethod = qtNativeClass.getMethod("activity");
            final Object value = activityMethod.invoke(null);
            if (value instanceof Activity) {
                return (Activity) value;
            }
        } catch (ReflectiveOperationException ignored) {
        }
        return null;
    }
}
