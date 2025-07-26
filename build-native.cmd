@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Build Script para Biblioteca Nativa
echo  West Gunfighter Hooks - vdev
echo ============================================
echo.

:: Verificar e corrigir o Gradle wrapper
if not exist "gradlew.bat" (
    echo ERRO: gradlew.bat nao encontrado no diretorio atual
    echo Certifique-se de estar executando este script na raiz do projeto
    pause
    exit /b 1
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
            gradle wrapper --gradle-version=8.6
        ) else (
            echo ERRO: Gradle nao encontrado globalmente
            echo Crie o arquivo fix-gradle-wrapper.cmd ou instale o Gradle
            pause
            exit /b 1
        )
    )
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
        pause
        exit /b 1
    )
)

:: Encontrar versao do NDK instalada
set "NDK_PATH="
for /d %%i in ("%ANDROID_HOME%\ndk\*") do (
    set "NDK_PATH=%%i"
    goto :found_ndk
)
:found_ndk

if "%NDK_PATH%"=="" (
    echo ERRO: Nenhuma versao do Android NDK encontrada em %ANDROID_HOME%\ndk\
    echo Por favor, instale o Android NDK atraves do SDK Manager
    pause
    exit /b 1
)

echo NDK encontrado: %NDK_PATH%

:: Definir variaveis de ambiente necessarias
set "ANDROID_NDK_HOME=%NDK_PATH%"

echo Configuracoes:
echo - ANDROID_HOME: %ANDROID_HOME%
echo - ANDROID_NDK_HOME: %ANDROID_NDK_HOME%
echo.

:: Criar arquivo local.properties se nao existir
if not exist "local.properties" (
    echo Criando local.properties...
    echo sdk.dir=%ANDROID_HOME:\=/% > local.properties
    echo ndk.dir=%ANDROID_NDK_HOME:\=/% >> local.properties
)

echo Iniciando build da biblioteca nativa...
echo.

:: Executar o build
echo Executando: gradlew.bat :app:assembleDebug
call gradlew.bat :app:assembleDebug

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERRO: Build falhou com codigo %ERRORLEVEL%
    echo.
    echo Tentativas de solucao:
    echo 1. Verifique se o Android SDK e NDK estao instalados corretamente
    echo 2. Verifique se todas as dependencias estao satisfeitas
    echo 3. Execute: gradlew.bat clean :app:assembleDebug
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  Build concluido com sucesso!
echo ============================================
echo.

:: Procurar pelos arquivos .so gerados
echo Procurando bibliotecas compiladas...
for /r "app\build" %%f in (*.so) do (
    echo Encontrado: %%f
)

echo.
echo A biblioteca nativa foi compilada com sucesso!
echo Arquivos .so estao localizados em: app\build\intermediates\ndkBuild\debug\obj\local\armeabi-v7a\
echo.

pause