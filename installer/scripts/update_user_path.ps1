param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("add", "remove")]
    [string]$Action,

    [Parameter(Mandatory = $true)]
    [string]$Path
)

$ErrorActionPreference = "Stop"

function Normalize-PathEntry {
    param([string]$Value)
    return $Value.Trim().TrimEnd('\', '/')
}

$scope = [System.EnvironmentVariableTarget]::User
$current = [System.Environment]::GetEnvironmentVariable("Path", $scope)
$entries = @()

if (-not [string]::IsNullOrWhiteSpace($current)) {
    $entries = $current -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}

$normalizedTarget = Normalize-PathEntry -Value $Path

if ($Action -eq "add") {
    $exists = $false
    foreach ($entry in $entries) {
        if ((Normalize-PathEntry -Value $entry) -ieq $normalizedTarget) {
            $exists = $true
            break
        }
    }

    if (-not $exists) {
        $entries = @($entries) + $Path
    }
} else {
    $entries = @($entries) | Where-Object {
        (Normalize-PathEntry -Value $_) -ine $normalizedTarget
    }
}

$next = ($entries | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }) -join ';'
[System.Environment]::SetEnvironmentVariable("Path", $next, $scope)
