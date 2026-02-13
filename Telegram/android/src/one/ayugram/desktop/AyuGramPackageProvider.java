package one.ayugram.desktop;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Locale;

public final class AyuGramPackageProvider extends ContentProvider {
    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(
            Uri uri,
            String[] projection,
            String selection,
            String[] selectionArgs,
            String sortOrder) {
        return null;
    }

    @Override
    public String getType(Uri uri) {
        return "application/vnd.android.package-archive";
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        return null;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode) && !"rt".equals(mode)) {
            throw new FileNotFoundException("Read-only provider");
        }
        final String path = uri.getQueryParameter("path");
        if (path == null || path.isEmpty()) {
            throw new FileNotFoundException("Path is missing");
        }
        final File file = new File(path);
        if (!file.exists()) {
            throw new FileNotFoundException("File does not exist");
        }
        if (!file.getName().toLowerCase(Locale.US).endsWith(".apk")) {
            throw new FileNotFoundException("Only APK files are supported");
        }
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
    }
}
