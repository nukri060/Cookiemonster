param(
    [Parameter(Mandatory=$true)]
    [int]$DelayMinutes,
    
    [Parameter(Mandatory=$true)]
    [string]$CommitMessage,
    
    [Parameter(Mandatory=$true)]
    [string]$FilesToAdd
)

# Convert delay to milliseconds
$delayMs = $DelayMinutes * 60 * 1000

# Start a background job to commit after delay
Start-Job -ScriptBlock {
    param($delay, $message, $files)
    Start-Sleep -Milliseconds $delay
    git add $files
    git commit -m $message
} -ArgumentList $delayMs, $CommitMessage, $FilesToAdd

Write-Host "Commit scheduled for $DelayMinutes minutes from now with message: $CommitMessage" 