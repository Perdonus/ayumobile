package one.ayugram.desktop;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.location.Address;
import android.location.Geocoder;
import android.location.Location;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

public final class AyuGramBridge {
    public static final int PERMISSION_GRANTED_STATE = 0;
    public static final int PERMISSION_CAN_REQUEST_STATE = 1;
    public static final int PERMISSION_DENIED_STATE = 2;

    private static final String PREFS_NAME = "ayugram_permission_state";
    private static final String REQUESTED_PREFIX = "requested::";

    private static final AtomicInteger NEXT_REQUEST_CODE = new AtomicInteger(6000);
    private static final ConcurrentHashMap<Integer, PermissionAwaiter> REQUESTS =
            new ConcurrentHashMap<>();

    private AyuGramBridge() {
    }

    private static String requestedKey(String permission) {
        return REQUESTED_PREFIX + permission;
    }

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }

    private static boolean wasRequestedBefore(Context context, String permission) {
        return prefs(context).getBoolean(requestedKey(permission), false);
    }

    private static void markRequested(Context context, String permission) {
        prefs(context)
                .edit()
                .putBoolean(requestedKey(permission), true)
                .apply();
    }

    public static int checkPermission(Context context, String permission) {
        if (context == null || permission == null || permission.isEmpty()) {
            return PackageManager.PERMISSION_DENIED;
        }
        return context.checkSelfPermission(permission);
    }

    public static int permissionState(Context context, String permission) {
        if (context == null || permission == null || permission.isEmpty()) {
            return PERMISSION_DENIED_STATE;
        }
        if (checkPermission(context, permission) == PackageManager.PERMISSION_GRANTED) {
            return PERMISSION_GRANTED_STATE;
        }
        final Activity activity = resolveActivity(context);
        if (activity == null) {
            return PERMISSION_DENIED_STATE;
        }
        final boolean requestedBefore = wasRequestedBefore(context, permission);
        if (!requestedBefore || activity.shouldShowRequestPermissionRationale(permission)) {
            return PERMISSION_CAN_REQUEST_STATE;
        }
        return PERMISSION_DENIED_STATE;
    }

    public static int requestPermissionSync(Context context, String permission, int timeoutMs) {
        if (context == null || permission == null || permission.isEmpty()) {
            return PERMISSION_DENIED_STATE;
        }
        final Activity activity = resolveActivity(context);
        if (activity == null) {
            return permissionState(context, permission);
        }
        final int initial = permissionState(context, permission);
        if (initial != PERMISSION_CAN_REQUEST_STATE) {
            return initial;
        }

        markRequested(context, permission);

        final CountDownLatch latch = new CountDownLatch(1);
        final AtomicInteger result = new AtomicInteger(PackageManager.PERMISSION_DENIED);
        final int requestCode = NEXT_REQUEST_CODE.getAndIncrement();
        REQUESTS.put(requestCode, grantResult -> {
            result.set(grantResult);
            latch.countDown();
        });

        activity.runOnUiThread(() -> activity.requestPermissions(
                new String[]{permission},
                requestCode));

        try {
            latch.await(Math.max(timeoutMs, 1500), TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            REQUESTS.remove(requestCode);
        }

        if (result.get() == PackageManager.PERMISSION_GRANTED) {
            return PERMISSION_GRANTED_STATE;
        }
        return permissionState(context, permission);
    }

    public static double[] resolveLastKnownLocation(Context context) {
        if (context == null) {
            return new double[0];
        }

        final int fine = checkPermission(context, "android.permission.ACCESS_FINE_LOCATION");
        final int coarse = checkPermission(context, "android.permission.ACCESS_COARSE_LOCATION");
        if (fine != PackageManager.PERMISSION_GRANTED
                && coarse != PackageManager.PERMISSION_GRANTED) {
            return new double[0];
        }

        final LocationManager manager =
                (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        if (manager == null) {
            return new double[0];
        }

        Location best = null;
        try {
            final List<String> providers = manager.getProviders(true);
            for (String provider : providers) {
                final Location candidate = manager.getLastKnownLocation(provider);
                if (candidate == null) {
                    continue;
                }
                if (best == null || candidate.getTime() > best.getTime()) {
                    best = candidate;
                }
            }
        } catch (SecurityException ignored) {
            return new double[0];
        }

        if (best == null) {
            return new double[0];
        }
        return new double[]{best.getLatitude(), best.getLongitude()};
    }

    static void onRequestPermissionsResult(
            int requestCode,
            String[] permissions,
            int[] grantResults) {
        final PermissionAwaiter awaiter = REQUESTS.remove(requestCode);
        if (awaiter == null) {
            return;
        }
        int result = PackageManager.PERMISSION_DENIED;
        if (grantResults != null && grantResults.length > 0) {
            result = grantResults[0];
        }
        awaiter.complete(result);
    }

    public static void openAppNotificationSettings(Context context) {
        if (context == null) {
            return;
        }
        final Activity activity = resolveActivity(context);
        final Intent intent;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS)
                    .putExtra(
                            Settings.EXTRA_APP_PACKAGE,
                            context.getPackageName());
        } else {
            intent = new Intent(
                    Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                    Uri.parse("package:" + context.getPackageName()));
        }
        if (activity != null) {
            activity.startActivity(intent);
        } else {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        }
    }

    public static void openAppSettings(Context context) {
        if (context == null) {
            return;
        }
        final Intent intent = new Intent(
                Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                Uri.parse("package:" + context.getPackageName()));
        final Activity activity = resolveActivity(context);
        if (activity != null) {
            activity.startActivity(intent);
        } else {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        }
    }

    public static void installPackage(Context context, String uriString) {
        if (context == null || uriString == null || uriString.isEmpty()) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                && !context.getPackageManager().canRequestPackageInstalls()) {
            final Intent settingsIntent = new Intent(
                    Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                    Uri.parse("package:" + context.getPackageName()))
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(settingsIntent);
            } catch (RuntimeException ignored) {
            }
            return;
        }

        final Uri installUri = resolveInstallUri(context, uriString);
        if (installUri == null) {
            return;
        }

        final Intent intent = new Intent(Intent.ACTION_VIEW)
                .setDataAndType(installUri, "application/vnd.android.package-archive")
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try {
            context.startActivity(intent);
        } catch (RuntimeException ignored) {
        }
    }

    public static String resolveLocationAddress(
            Context context,
            double latitude,
            double longitude,
            String languageTag) {
        if (context == null || !Geocoder.isPresent()) {
            return "";
        }

        final Locale locale = (languageTag == null || languageTag.isEmpty())
                ? Locale.getDefault()
                : Locale.forLanguageTag(languageTag.replace('_', '-'));
        final Geocoder geocoder = new Geocoder(context, locale);
        final List<Address> addresses;
        try {
            addresses = geocoder.getFromLocation(latitude, longitude, 1);
        } catch (IOException | IllegalArgumentException ignored) {
            return "";
        }
        if (addresses == null || addresses.isEmpty()) {
            return "";
        }

        final Address address = addresses.get(0);
        final LinkedHashSet<String> uniqueParts = new LinkedHashSet<>();
        appendAddressPart(uniqueParts, address.getThoroughfare());
        appendAddressPart(uniqueParts, address.getSubLocality());
        appendAddressPart(uniqueParts, address.getLocality());
        appendAddressPart(uniqueParts, address.getSubAdminArea());
        appendAddressPart(uniqueParts, address.getAdminArea());
        appendAddressPart(uniqueParts, address.getCountryName());
        if (uniqueParts.isEmpty()) {
            return "";
        }
        return String.join(", ", new ArrayList<>(uniqueParts));
    }

    public static boolean restartApp(Context context) {
        if (context == null) {
            return false;
        }
        final PackageManager packageManager = context.getPackageManager();
        final Intent launchIntent = packageManager
                .getLaunchIntentForPackage(context.getPackageName());
        if (launchIntent == null) {
            return false;
        }
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(launchIntent);
        final Activity activity = resolveActivity(context);
        if (activity != null) {
            activity.finishAffinity();
        }
        return true;
    }

    private interface PermissionAwaiter {
        void complete(int grantResult);
    }

    private static void appendAddressPart(LinkedHashSet<String> target, String value) {
        if (value == null) {
            return;
        }
        final String trimmed = value.trim();
        if (!trimmed.isEmpty()) {
            target.add(trimmed);
        }
    }

    private static Uri resolveInstallUri(Context context, String pathOrUri) {
        final Uri parsed = Uri.parse(pathOrUri);
        final String scheme = parsed.getScheme();
        if ("content".equalsIgnoreCase(scheme)) {
            return parsed;
        }

        final String localPath;
        if ("file".equalsIgnoreCase(scheme)) {
            localPath = parsed.getPath();
        } else {
            localPath = pathOrUri;
        }

        if (localPath == null || localPath.isEmpty()) {
            return null;
        }
        final File file = new File(localPath);
        if (!file.exists()) {
            return null;
        }
        return new Uri.Builder()
                .scheme("content")
                .authority(context.getPackageName() + ".packageprovider")
                .appendPath("apk")
                .appendQueryParameter("path", file.getAbsolutePath())
                .build();
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
