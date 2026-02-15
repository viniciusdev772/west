# Build Script para Biblioteca Nativa - PowerShell
# West Gunfighter Hooks - vdev

param(
    [switch]$Clean,
    [switch]$Release,
    [switch]$Verbose
)

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Build Script para Biblioteca Nativa" -ForegroundColor Cyan
Write-Host " West Gunfighter Hooks - vdev" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Verificar se está no diretório correto
if (-not (Test-Path "gradlew.bat")) {
    Write-Host "ERRO: gradlew.bat não encontrado no diretório atual" -ForegroundColor Red
    Write-Host "Certifique-se de estar executando este script na raiz do projeto" -ForegroundColor Red
    Read-Host "Pressione Enter para sair"
    exit 1
}

# Validar gradle-wrapper.jar
$wrapperJar = "gradle\wrapper\gradle-wrapper.jar"
$wrapperOk = $false
if (Test-Path $wrapperJar) {
    try {
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [System.IO.Compression.ZipFile]::OpenRead($wrapperJar)
        $wrapperOk = $zip.Entries.FullName -contains "org/gradle/wrapper/GradleWrapperMain.class"
        $zip.Dispose()
    } catch {
        $wrapperOk = $false
    }
}

if (-not $wrapperOk) {
    Write-Host "AVISO: Gradle wrapper inválido ou corrompido" -ForegroundColor Yellow
    if (Test-Path ".\fix-gradle-wrapper.cmd") {
        Write-Host "Executando corretor do wrapper..." -ForegroundColor Yellow
        & .\fix-gradle-wrapper.cmd
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERRO: Falha ao reparar o Gradle wrapper" -ForegroundColor Red
            Read-Host "Pressione Enter para sair"
            exit $LASTEXITCODE
        }
    } else {
        Write-Host "ERRO: fix-gradle-wrapper.cmd não encontrado para reparo automático" -ForegroundColor Red
        Read-Host "Pressione Enter para sair"
        exit 1
    }
}

# Verificar Java/JDK
if (-not $env:JAVA_HOME) {
    Write-Host "AVISO: JAVA_HOME não está definido" -ForegroundColor Yellow
    Write-Host "Tentando localizar Java automaticamente..." -ForegroundColor Yellow

    $javaCandidates = @(
        "$env:ProgramFiles\Android\Android Studio\jbr",
        "$env:ProgramFiles\Android\Android Studio\jre",
        "$env:LOCALAPPDATA\Programs\Android Studio\jbr"
    )

    foreach ($candidate in $javaCandidates) {
        if (Test-Path (Join-Path $candidate "bin\java.exe")) {
            $env:JAVA_HOME = $candidate
            Write-Host "JAVA_HOME encontrado automaticamente: $candidate" -ForegroundColor Green
            break
        }
    }
}

if ($env:JAVA_HOME) {
    $env:PATH = "$($env:JAVA_HOME)\bin;$($env:PATH)"
} else {
    $javaCmd = Get-Command java -ErrorAction SilentlyContinue
    if (-not $javaCmd) {
        Write-Host "ERRO: Java não encontrado" -ForegroundColor Red
        Write-Host "Instale o JDK 17+ ou configure JAVA_HOME corretamente" -ForegroundColor Red
        Read-Host "Pressione Enter para sair"
        exit 1
    }
}

# Verificar Android SDK
$androidHome = $env:ANDROID_HOME
if (-not $androidHome) {
    Write-Host "AVISO: ANDROID_HOME não está definido" -ForegroundColor Yellow
    Write-Host "Tentando usar o SDK padrão do Android Studio..." -ForegroundColor Yellow
    
    $potentialSdk = "$env:USERPROFILE\AppData\Local\Android\Sdk"
    if (Test-Path $potentialSdk) {
        $androidHome = $potentialSdk
        $env:ANDROID_HOME = $androidHome
        Write-Host "SDK encontrado em: $androidHome" -ForegroundColor Green
    } else {
        Write-Host "ERRO: Android SDK não encontrado" -ForegroundColor Red
        Write-Host "Por favor, defina a variável ANDROID_HOME ou instale o Android Studio" -ForegroundColor Red
        Read-Host "Pressione Enter para sair"
        exit 1
    }
}

Write-Host "Configurações:" -ForegroundColor Green
Write-Host "- ANDROID_HOME: $androidHome" -ForegroundColor White
if ($env:JAVA_HOME) { Write-Host "- JAVA_HOME: $($env:JAVA_HOME)" -ForegroundColor White }
Write-Host "- NDK: controlado por ndkVersion no app/build.gradle" -ForegroundColor White
Write-Host ""

# Criar local.properties se não existir
if (-not (Test-Path "local.properties")) {
    Write-Host "Criando local.properties..." -ForegroundColor Yellow
    $sdkPath = $androidHome -replace '\\', '/'
    
@"
sdk.dir=$sdkPath
"@ | Out-File -FilePath "local.properties" -Encoding UTF8
}

# Determinar o tipo de build
$buildType = if ($Release) { "Release" } else { "Debug" }
$gradleTask = if ($Release) { ":app:externalNativeBuildRelease" } else { ":app:externalNativeBuildDebug" }

Write-Host "Iniciando build nativo (.so) ($buildType)..." -ForegroundColor Cyan
Write-Host ""

# Executar clean se solicitado
if ($Clean) {
    Write-Host "Limpando projeto..." -ForegroundColor Yellow
    & .\gradlew.bat clean
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERRO: Falha na limpeza do projeto" -ForegroundColor Red
        Read-Host "Pressione Enter para sair"
        exit $LASTEXITCODE
    }
}

# Executar build principal
$gradleArgs = @($gradleTask)
if ($Verbose) {
    $gradleArgs += "--info"
}

Write-Host "Executando: gradlew.bat $($gradleArgs -join ' ')" -ForegroundColor White
& .\gradlew.bat @gradleArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERRO: Build falhou com código $LASTEXITCODE" -ForegroundColor Red
    Write-Host ""
    Write-Host "Tentativas de solução:" -ForegroundColor Yellow
    Write-Host "1. Verifique se Java JDK 17+ está instalado e JAVA_HOME configurado" -ForegroundColor White
    Write-Host "2. Verifique se todas as dependências estão satisfeitas" -ForegroundColor White
    Write-Host "3. Execute: .\build-native.ps1 -Clean" -ForegroundColor White
    Write-Host "4. Execute: .\build-native.ps1 -Verbose para mais detalhes" -ForegroundColor White
    Read-Host "Pressione Enter para sair"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host " Build concluído com sucesso!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""

# Mostrar apenas o .so mais recente
Write-Host "Procurando biblioteca compilada mais recente..." -ForegroundColor Cyan
$latestSo = Get-ChildItem -Path "app\build" -Filter "*.so" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($latestSo) {
    Write-Host "Mais recente: $($latestSo.FullName)" -ForegroundColor Green
    Write-Host "Modificado: $($latestSo.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Host "Nenhuma biblioteca .so encontrada no diretório de build" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "A biblioteca nativa foi compilada com sucesso!" -ForegroundColor Green
if ($latestSo) { Write-Host "Biblioteca .so mais recente: $($latestSo.FullName)" -ForegroundColor White }
Write-Host ""

# Mostrar opções de uso
Write-Host "Opções de uso do script:" -ForegroundColor Cyan
Write-Host "  .\build-native.ps1           - Build debug (padrão)" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Release  - Build release" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Clean    - Limpar antes do build" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Verbose  - Modo verboso" -ForegroundColor White
Write-Host ""

Read-Host "Pressione Enter para sair"
