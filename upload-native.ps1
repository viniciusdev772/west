param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [string]$BuildType = "Debug"
)

$uploadApiUrl = "https://modmanager-chi.vercel.app/api/v11/upload"

if (-not (Test-Path $FilePath)) {
    Write-Host "ERRO: Arquivo para upload nao encontrado: $FilePath" -ForegroundColor Red
    exit 1
}

Add-Type -AssemblyName System.Net.Http

$fileName = [System.IO.Path]::GetFileName($FilePath)
$description = "West Gunfighter Hooks - $BuildType - upload automatico"
$tags = "west,native,lib,so,$($BuildType.ToLower())"

$httpClient = [System.Net.Http.HttpClient]::new()
$multipart = [System.Net.Http.MultipartFormDataContent]::new()

try {
    $fileBytes = [System.IO.File]::ReadAllBytes($FilePath)
    $fileContent = [System.Net.Http.ByteArrayContent]::new($fileBytes)
    $fileContent.Headers.ContentType = [System.Net.Http.Headers.MediaTypeHeaderValue]::Parse("application/octet-stream")

    $multipart.Add($fileContent, "file", $fileName)
    $multipart.Add([System.Net.Http.StringContent]::new($description), "description")
    $multipart.Add([System.Net.Http.StringContent]::new($tags), "tags")

    Write-Host "Enviando biblioteca para API v11..." -ForegroundColor Cyan
    Write-Host "- Endpoint: $uploadApiUrl" -ForegroundColor White
    Write-Host "- Arquivo: $fileName" -ForegroundColor White

    $response = $httpClient.PostAsync($uploadApiUrl, $multipart).GetAwaiter().GetResult()
    $responseBody = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()

    if (-not $response.IsSuccessStatusCode) {
        throw "Upload falhou com status $([int]$response.StatusCode): $responseBody"
    }

    $json = $responseBody | ConvertFrom-Json
    if (-not $json.success) {
        throw "API retornou success=false: $responseBody"
    }

    Write-Host "Upload concluido com sucesso!" -ForegroundColor Green
    if ($json.file.url) { Write-Host "- URL: $($json.file.url)" -ForegroundColor White }
    if ($json.file.directUrl) { Write-Host "- Direct URL: $($json.file.directUrl)" -ForegroundColor White }
    exit 0
} catch {
    Write-Host "ERRO: Falha no upload da biblioteca" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    if ($multipart) { $multipart.Dispose() }
    if ($httpClient) { $httpClient.Dispose() }
}
