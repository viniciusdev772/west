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

# Verificar NDK
$ndkPath = "$androidHome\ndk\25.2.9519653"
if (-not (Test-Path $ndkPath)) {
    Write-Host "ERRO: Android NDK não encontrado em $ndkPath" -ForegroundColor Red
    Write-Host "Por favor, instale o Android NDK versão 25.2.9519653 através do SDK Manager" -ForegroundColor Red
    Read-Host "Pressione Enter para sair"
    exit 1
}

# Definir variáveis de ambiente
$env:ANDROID_NDK_HOME = $ndkPath

Write-Host "Configurações:" -ForegroundColor Green
Write-Host "- ANDROID_HOME: $androidHome" -ForegroundColor White
Write-Host "- ANDROID_NDK_HOME: $ndkPath" -ForegroundColor White
Write-Host ""

# Criar local.properties se não existir
if (-not (Test-Path "local.properties")) {
    Write-Host "Criando local.properties..." -ForegroundColor Yellow
    $sdkPath = $androidHome -replace '\\', '/'
    $ndkPathFormatted = $ndkPath -replace '\\', '/'
    
    @"
sdk.dir=$sdkPath
ndk.dir=$ndkPathFormatted
"@ | Out-File -FilePath "local.properties" -Encoding UTF8
}

# Determinar o tipo de build
$buildType = if ($Release) { "Release" } else { "Debug" }
$gradleTask = if ($Release) { ":app:assembleRelease" } else { ":app:assembleDebug" }

Write-Host "Iniciando build da biblioteca nativa ($buildType)..." -ForegroundColor Cyan
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
    Write-Host "1. Verifique se o Android SDK e NDK estão instalados corretamente" -ForegroundColor White
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

# Procurar pelos arquivos .so gerados
Write-Host "Procurando bibliotecas compiladas..." -ForegroundColor Cyan
$soFiles = Get-ChildItem -Path "app\build" -Filter "*.so" -Recurse -ErrorAction SilentlyContinue

if ($soFiles) {
    Write-Host "Bibliotecas encontradas:" -ForegroundColor Green
    foreach ($file in $soFiles) {
        Write-Host "  - $($file.FullName)" -ForegroundColor White
        
        # Mostrar informações do arquivo
        $fileInfo = Get-Item $file.FullName
        Write-Host "    Tamanho: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Gray
        Write-Host "    Modificado: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    }
} else {
    Write-Host "Nenhuma biblioteca .so encontrada no diretório de build" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "A biblioteca nativa foi compilada com sucesso!" -ForegroundColor Green
Write-Host "Arquivos .so estão localizados em: app\build\intermediates\ndkBuild\$($buildType.ToLower())\obj\local\armeabi-v7a\" -ForegroundColor White
Write-Host ""

# Mostrar opções de uso
Write-Host "Opções de uso do script:" -ForegroundColor Cyan
Write-Host "  .\build-native.ps1           - Build debug (padrão)" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Release  - Build release" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Clean    - Limpar antes do build" -ForegroundColor White
Write-Host "  .\build-native.ps1 -Verbose  - Modo verboso" -ForegroundColor White
Write-Host ""

Read-Host "Pressione Enter para sair"