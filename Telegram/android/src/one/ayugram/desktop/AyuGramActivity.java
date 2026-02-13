package one.ayugram.desktop;

import org.qtproject.qt.android.bindings.QtActivity;

public class AyuGramActivity extends QtActivity {
    @Override
    public void onRequestPermissionsResult(
            int requestCode,
            String[] permissions,
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        AyuGramBridge.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
}
