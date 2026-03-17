package vdev.com.android.support;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class LibraryLoader {

    public interface Listener {
        void onLibraryReady(File file);
        void onLibraryError(String message);
    }

    private static final String TAG = "LibraryLoader";

    @SuppressLint("UnsafeDynamicallyLoadedCode")
    public static void downloadAndLoadLibrary(Context context, String url, String libraryName, Listener listener) {
        Executor executor = Executors.newSingleThreadExecutor();

        executor.execute(() -> {
            File file = new File(context.getFilesDir(), libraryName);
            if (!downloadFile(url, file)) {
                if (listener != null) {
                    listener.onLibraryError("Download da biblioteca falhou.");
                }
                return;
            }

            try {
                System.load(file.getAbsolutePath());
                Log.i(TAG, "Biblioteca carregada com sucesso: " + libraryName);
                if (listener != null) {
                    listener.onLibraryReady(file);
                }
            } catch (UnsatisfiedLinkError error) {
                Log.e(TAG, "Erro ao carregar a biblioteca: " + error.getMessage());
                if (listener != null) {
                    listener.onLibraryError("Falha ao carregar a biblioteca nativa.");
                }
            }
        });
    }

    private static boolean downloadFile(String urlString, File destination) {
        try (BufferedInputStream in = new BufferedInputStream(new URL(urlString).openStream());
             FileOutputStream fileOutputStream = new FileOutputStream(destination)) {
            byte[] dataBuffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = in.read(dataBuffer, 0, 1024)) != -1) {
                fileOutputStream.write(dataBuffer, 0, bytesRead);
            }
            return true;
        } catch (IOException error) {
            Log.e(TAG, "Erro ao baixar o arquivo: " + error.getMessage());
            return false;
        }
    }
}
