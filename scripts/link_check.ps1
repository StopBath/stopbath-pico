# A first look at the USB link from a Windows machine, before the development
# peer is run on Linux (Pico spec KE4, hardware gate step one).
#
# Opens the remote's COM port with DTR asserted (the "host opened the port"
# fact), prints what the remote sends, answers its HELLO with one DISPLAY
# record so the panel draws the Wi-Fi page, then prints every BUTTON line
# the keys produce for the next twenty seconds, and closes the port, which
# should return the panel to NOT CONNECTED.
#
# Line ending: a bare line feed, as the protocol requires; a terminal that
# sends a carriage return on Enter is refused as malformed, which is why
# this exists instead of a terminal.
#
# Usage, from PowerShell:
#   .\scripts\link_check.ps1 -Port COM8
param(
    [Parameter(Mandatory = $true)] [string] $Port,
    [int] $ListenSeconds = 20
)

$displayRecord = "DISPLAY status=PRESENTING page=WIFI payload=WIFI%3AT%3AWPA%3BS%3Atest%3BP%3Apass%3B%3B delivered=0 error=NONE"

$serial = New-Object System.IO.Ports.SerialPort $Port, 115200, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.NewLine = "`n"
$serial.Encoding = [System.Text.Encoding]::ASCII
$serial.ReadTimeout = 500
$serial.DtrEnable = $true

function Read-Lines([int] $seconds, [string] $label) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $line = $serial.ReadLine()
            Write-Host ("{0}  <- {1}" -f $label, $line)
        } catch [System.TimeoutException] {
        }
    }
}

try {
    $serial.Open()
    Write-Host ("Opened {0} with DTR asserted; the panel should show CONNECTING." -f $Port)
    Read-Lines 3 "handshake"
    Write-Host ("  -> {0}" -f $displayRecord)
    $serial.Write($displayRecord + "`n")
    Write-Host "Sent the acceptance; the panel should draw the Wi-Fi page and the HELLOs should stop."
    Write-Host ("Press the keys now: tap KEY0, hold KEY0, tap KEY1. Listening for {0} seconds." -f $ListenSeconds)
    Read-Lines $ListenSeconds "keys"
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
        Write-Host "Closed the port; the panel should show NOT CONNECTED."
    }
}
