@echo off
setlocal enabledelayedexpansion
set "STARTED_FROM_CMD=%COMSPEC%"
set "KEEP_OPEN=1"

echo ============================================
echo  Build Script para Biblioteca Nativa
echo  West Gunfighter Hooks - vdev
echo ============================================
echo.

:: Verificar e corrigir o Gradle wrapper
if not exist "gradlew.bat" (
    echo ERRO: gradlew.bat nao encontrado no diretorio atual
    echo Certifique-se de estar executando este script na raiz do projeto
    goto :end
)

:: Verificar se o gradle wrapper esta funcionando corretamente
if not exist "gradle\wrapper\gradle-wrapper.jar" (
    echo AVISO: Gradle wrapper corrompido ou faltando
    echo Tentando corrigir o wrapper...
    
    if exist "fix-gradle-wrapper.cmd" (
        echo Executando corretor do wrapper...
        call fix-gradle-wrapper.cmd
    ) else (
        :: Usar gradle global se disponivel
        where gradle >nul 2>&1
        if !ERRORLEVEL! equ 0 (
            echo Usando Gradle global para recriar o wrapper...
            gradle wrapper --gradle-version=8.13
        ) else (
            echo ERRO: Gradle nao encontrado globalmente
            echo Crie o arquivo fix-gradle-wrapper.cmd ou instale o Gradle
            goto :end
        )
    )
)

:: Validar se gradle-wrapper.jar contem GradleWrapperMain
set "WRAPPER_OK=0"
if exist "gradle\wrapper\gradle-wrapper.jar" (
    powershell -Command "& { Add-Type -AssemblyName System.IO.Compression.FileSystem; try { $zip = [System.IO.Compression.ZipFile]::OpenRead('gradle\wrapper\gradle-wrapper.jar'); $ok = $zip.Entries.FullName -contains 'org/gradle/wrapper/GradleWrapperMain.class'; $zip.Dispose(); if ($ok) { exit 0 } else { exit 1 } } catch { exit 1 } }"
    if !ERRORLEVEL! equ 0 set "WRAPPER_OK=1"
)

if "!WRAPPER_OK!"=="0" (
    echo AVISO: Gradle wrapper invalido ou corrompido
    if exist "fix-gradle-wrapper.cmd" (
        echo Executando corretor do wrapper...
        call fix-gradle-wrapper.cmd
    ) else (
        echo ERRO: fix-gradle-wrapper.cmd nao encontrado para reparo automatico
        goto :end
    )
)

:: Verificar Java/JDK
if "%JAVA_HOME%"=="" (
    echo AVISO: JAVA_HOME nao esta definido
    echo Tentando localizar Java automaticamente...

    set "JAVA_CANDIDATE=%ProgramFiles%\Android\Android Studio\jbr"
    if exist "!JAVA_CANDIDATE!\bin\java.exe" set "JAVA_HOME=!JAVA_CANDIDATE!"

    if "%JAVA_HOME%"=="" (
        set "JAVA_CANDIDATE=%ProgramFiles%\Android\Android Studio\jre"
        if exist "!JAVA_CANDIDATE!\bin\java.exe" set "JAVA_HOME=!JAVA_CANDIDATE!"
    )

    if "%JAVA_HOME%"=="" (
        set "JAVA_CANDIDATE=%LOCALAPPDATA%\Programs\Android Studio\jbr"
        if exist "!JAVA_CANDIDATE!\bin\java.exe" set "JAVA_HOME=!JAVA_CANDIDATE!"
    )

    if not "%JAVA_HOME%"=="" (
        echo JAVA_HOME encontrado automaticamente: !JAVA_HOME!
    )
)

if "%JAVA_HOME%"=="" (
    where java >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo ERRO: Java nao encontrado
        echo Instale o JDK 17+ ou configure JAVA_HOME corretamente
        goto :end
    )
) else (
    set "PATH=%JAVA_HOME%\bin;%PATH%"
)

:: Verificar se o Android SDK esta configurado
if "%ANDROID_HOME%"=="" (
    echo AVISO: ANDROID_HOME nao esta definido
    echo Tentando usar o SDK padrao do Android Studio...
    
    :: Tentar encontrar o Android SDK em locais comuns
    set "POTENTIAL_SDK=%USERPROFILE%\AppData\Local\Android\Sdk"
    if exist "!POTENTIAL_SDK!" (
        set "ANDROID_HOME=!POTENTIAL_SDK!"
        echo SDK encontrado em: !ANDROID_HOME!
    ) else (
        echo ERRO: Android SDK nao encontrado
        echo Por favor, defina a variavel ANDROID_HOME ou instale o Android Studio
        goto :end
    )
)

echo Configuracoes:
echo - ANDROID_HOME: %ANDROID_HOME%
if not "%JAVA_HOME%"=="" echo - JAVA_HOME: %JAVA_HOME%
echo - NDK: controlado por ndkVersion no app/build.gradle
echo.

:: Criar arquivo local.properties se nao existir
if not exist "local.properties" (
    echo Criando local.properties...
    echo sdk.dir=%ANDROID_HOME:\=/% > local.properties
)

echo Iniciando build da biblioteca nativa...
echo.

:: Executar build nativo direto (.so)
echo Executando: gradlew.bat :app:externalNativeBuildDebug
call gradlew.bat :app:externalNativeBuildDebug

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERRO: Build falhou com codigo %ERRORLEVEL%
    echo.
    echo Tentativas de solucao:
    echo 1. Verifique se Java JDK 17+ esta instalado e JAVA_HOME configurado
    echo 2. Verifique se todas as dependencias estao satisfeitas
    echo 3. Execute: gradlew.bat clean :app:assembleDebug
    goto :end
)

echo.
echo ============================================
echo  Build concluido com sucesso!
echo ============================================
echo.

:: Mostrar apenas o .so mais recente
echo Procurando biblioteca compilada mais recente...
set "LATEST_SO="
for /f "delims=" %%f in ('dir /s /b /a-d /o-d "app\build\*.so" 2^>nul') do (
    if not defined LATEST_SO set "LATEST_SO=%%f"
)

if defined LATEST_SO (
    echo Mais recente: %LATEST_SO%
) else (
    echo Nenhuma biblioteca .so encontrada no diretorio app\build
    goto :end
)

echo.
echo Enviando biblioteca para API v11...
powershell -NoProfile -ExecutionPolicy Bypass -File ".\upload-native.ps1" -FilePath "%LATEST_SO%" -BuildType "Debug"

if %ERRORLEVEL% neq 0 (
    echo ERRO: Falha no upload da biblioteca para a API v11
    goto :end
)

echo.
echo A biblioteca nativa foi compilada com sucesso!
if defined LATEST_SO echo Biblioteca .so mais recente: %LATEST_SO%
echo Upload para API v11: concluido
echo.

:end
echo.
echo O script terminou. Esta janela permanecera aberta.
cmd /k
