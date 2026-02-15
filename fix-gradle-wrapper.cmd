@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Corrigindo Gradle Wrapper
echo ============================================
echo.

:: Verificar Java/JDK para executar gradle
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
)

if not "%JAVA_HOME%"=="" (
    set "PATH=%JAVA_HOME%\bin;%PATH%"
    echo JAVA_HOME: %JAVA_HOME%
) else (
    where java >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo ERRO: Java nao encontrado
        echo Instale o JDK 17+ ou configure JAVA_HOME corretamente
        pause
        exit /b 1
    )
)

:: Gradle recomenda gerar wrapper via task "wrapper" (sem baixar JAR manualmente)
set "GRADLE_VERSION=8.13"
set "GRADLE_DIST_URL=https://services.gradle.org/distributions/gradle-%GRADLE_VERSION%-bin.zip"
set "TMP_GRADLE_DIR=%TEMP%\west-gradle-%GRADLE_VERSION%"
set "TMP_GRADLE_ZIP=%TEMP%\west-gradle-%GRADLE_VERSION%.zip"
set "GRADLE_CMD=gradle"

where gradle >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo AVISO: Gradle global nao encontrado, usando distribuicao temporaria...
    if not exist "%TMP_GRADLE_DIR%\gradle-%GRADLE_VERSION%\bin\gradle.bat" (
        echo Baixando Gradle %GRADLE_VERSION%...
        powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%GRADLE_DIST_URL%' -OutFile '%TMP_GRADLE_ZIP%'}"
        if !ERRORLEVEL! neq 0 (
            echo ERRO: Falha ao baixar Gradle de %GRADLE_DIST_URL%
            pause
            exit /b 1
        )

        if exist "%TMP_GRADLE_DIR%" rmdir /s /q "%TMP_GRADLE_DIR%"
        mkdir "%TMP_GRADLE_DIR%"
        echo Extraindo Gradle...
        powershell -Command "Expand-Archive -Path '%TMP_GRADLE_ZIP%' -DestinationPath '%TMP_GRADLE_DIR%' -Force"
        if !ERRORLEVEL! neq 0 (
            echo ERRO: Falha ao extrair distribuicao do Gradle
            pause
            exit /b 1
        )
    )
    set "GRADLE_CMD=%TMP_GRADLE_DIR%\gradle-%GRADLE_VERSION%\bin\gradle.bat"
)

echo Executando: %GRADLE_CMD% wrapper --gradle-version=%GRADLE_VERSION% --distribution-type=bin
call "%GRADLE_CMD%" wrapper --gradle-version=%GRADLE_VERSION% --distribution-type=bin
if !ERRORLEVEL! neq 0 (
    echo ERRO: Falha ao gerar arquivos do Gradle Wrapper
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Gradle Wrapper corrigido com sucesso!
echo ============================================
echo.

echo Testando wrapper...
call gradlew.bat --version

if %ERRORLEVEL% equ 0 (
    echo.
    echo Wrapper funcionando corretamente!
) else (
    echo.
    echo ERRO: Wrapper ainda nao esta funcionando
    echo Verifique se o Java esta instalado e configurado
)

echo.
