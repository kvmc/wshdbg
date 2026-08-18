param(
    [string]$BuildDir = "build-local",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root $BuildDir
$logDir = Join-Path $build "test-logs"
New-Item -ItemType Directory -Force $logDir | Out-Null

Write-Host "Configuring wshdbg with Windows integration tests..."
cmake -S $root -B $build -A x64 -DWSHDBG_BUILD_INTEGRATION_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "Building $Configuration..."
cmake --build $build --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "Running unit and integration tests..."
ctest --test-dir $build -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "CTest failed" }

$exe = Join-Path $build "$Configuration/wshdbg.exe"
$script = Join-Path $root "tests/data/breakpoints/simple.vbs"
$trace = Join-Path $logDir "breakpoint-trace.log"

Write-Host "Running CLI breakpoint smoke test at simple.vbs:4..."
"continue" | & $exe debug --break 4 --log-level trace --log-file $trace $script
if ($LASTEXITCODE -ne 0) { throw "CLI breakpoint smoke test failed" }

Write-Host "Smoke test passed. Trace: $trace"
