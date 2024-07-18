package vdev.com.android.support;

import static java.lang.Thread.sleep;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class LibraryLoader {

    private static final String TAG = "LibraryLoader";
    public String GameActivity = "com.cg.cowboy.MainActivity";

    // Baixa a biblioteca e a salva no armazenamento interno
    @SuppressLint("UnsafeDynamicallyLoadedCode")
    public static void downloadAndLoadLibrary(Context context, String url, String libraryName) {
        Executor executor = Executors.newSingleThreadExecutor(); // Criar um executor de thread única

        executor.execute(() -> { // Executar a operação de download em uma thread separada
            File file = new File(context.getFilesDir(), "lib.so");
            if (downloadFile(url, file)) {
                try {
                    System.load(file.getAbsolutePath());
                    sleep(100);
                    Main.Start(context);
                    context.startActivity(new Intent(context, Class.forName("com.cg.cowboy.MainActivity")));
                    Log.i(TAG, "Biblioteca carregada com sucesso: " + libraryName);
                } catch (UnsatisfiedLinkError e) {
                    Log.e(TAG, "Erro ao carregar a biblioteca: " + e.getMessage());
                } catch (InterruptedException | ClassNotFoundException e) {
                    throw new RuntimeException(e);
                }
            } else {
                Log.e(TAG, "Download da biblioteca falhou.");
            }
        });
    }

    // Função auxiliar para baixar um arquivo
    private static boolean downloadFile(String urlString, File destination) {
        try (BufferedInputStream in = new BufferedInputStream(new URL(urlString).openStream());
             FileOutputStream fileOutputStream = new FileOutputStream(destination)) {
            byte[] dataBuffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = in.read(dataBuffer, 0, 1024)) != -1) {
                fileOutputStream.write(dataBuffer, 0, bytesRead);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Erro ao baixar o arquivo: " + e.getMessage());
            return false;
        }
    }
}
