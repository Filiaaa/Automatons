$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$build = Join-Path $root "build-cov"
cmake -B $build -G "MinGW Makefiles" -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++
cmake --build $build

Get-ChildItem $build -Recurse -Filter "*.gcda" -ErrorAction SilentlyContinue |
  Remove-Item -Force

$testExe = Join-Path $build "automata_tests.exe"
& $testExe
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }

$objDir = Join-Path $build "CMakeFiles\automata_tests.dir\tests"
Push-Location $objDir
Remove-Item *.gcov -ErrorAction SilentlyContinue
& gcov "all_tests.cpp.gcda" | Out-Null

function Get-Coverage([string]$gcovFile) {
  $exec = 0
  $cov = 0
  foreach ($line in Get-Content $gcovFile) {
    if ($line -match "^[ ]*(-|[0-9]+|#####):") {
      $prefix = ($line -split ":", 2)[0].Trim()
      if ($prefix -eq "-") { continue }
      $exec++
      if ($prefix -ne "#####") { $cov++ }
    }
  }
  $pct = if ($exec -eq 0) { 100.0 } else { [math]::Round(100.0 * $cov / $exec, 2) }
  return [pscustomobject]@{ Covered = $cov; Executable = $exec; Percent = $pct }
}

$headers = @("NFA.hpp.gcov", "Regex.hpp.gcov", "DotExport.hpp.gcov")
$totalCov = 0
$totalExec = 0
foreach ($header in $headers) {
  if (-not (Test-Path $header)) { throw "Missing $header" }
  $stats = Get-Coverage $header
  $name = $header -replace "\.gcov$", ""
  Write-Output ("{0}: {1}/{2} ({3}%)" -f $name, $stats.Covered, $stats.Executable, $stats.Percent)
  $totalCov += $stats.Covered
  $totalExec += $stats.Executable
}
Pop-Location

$totalPct = [math]::Round(100.0 * $totalCov / $totalExec, 2)
Write-Output ("TOTAL: {0}/{1} ({2}%)" -f $totalCov, $totalExec, $totalPct)
if ($totalPct -lt 95) { throw "Coverage $totalPct% is below 95%" }
Write-Output "Coverage OK (>= 95%)"
