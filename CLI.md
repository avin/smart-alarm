# Smart Alarm CLI

Smart Alarm installs two executables:

- `SmartAlarm.exe`: tray application, no console window.
- `SmartAlarmCli.exe`: console CLI for scripts and agents.

The CLI talks to the already running tray application. It never edits
`settings.json` directly and never starts Smart Alarm automatically. If the tray
application is not running, commands return `app_not_running` and exit with code
`2`.

The Windows installer can add the Smart Alarm `bin` directory to the current
user's `PATH`. This option is enabled by default. Already-open terminals may
need to be restarted before `SmartAlarmCli.exe` is resolved by name.

## Help

Help does not require the tray application to be running:

```powershell
SmartAlarmCli.exe
SmartAlarmCli.exe help
SmartAlarmCli.exe --help
SmartAlarmCli.exe help add
SmartAlarmCli.exe add --help
SmartAlarmCli.exe help --json
```

## Output And Exit Codes

Use `--json` for machine-readable output:

```powershell
SmartAlarmCli.exe status --json
```

Exit codes:

| Code | Meaning |
| ---: | --- |
| `0` | Success |
| `1` | Validation error or bad arguments |
| `2` | Smart Alarm is not running |
| `3` | Operation failed inside the running app |
| `4` | Protocol or internal error |

JSON success:

```json
{"ok":true,"data":{}}
```

JSON error:

```json
{"ok":false,"error":{"code":"app_not_running","message":"Smart Alarm is not running"}}
```

## UUID

The CLI uses `uuid`, not `id`. Output uses UUIDs without braces:

```text
8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

## Saved Notifications

List and inspect:

```powershell
SmartAlarmCli.exe list
SmartAlarmCli.exe list --json
SmartAlarmCli.exe get --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

Delete immediately:

```powershell
SmartAlarmCli.exe delete --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

Create once:

```powershell
SmartAlarmCli.exe add --message "Stand up" --schedule-type once --date 2026-05-29 --time 15:30
```

Create weekly:

```powershell
SmartAlarmCli.exe add --message "Gym" --schedule-type weekly --days mon,wed,fri --time 18:00
```

Create nth-week:

```powershell
SmartAlarmCli.exe add --message "Review" --schedule-type nth-week --every-weeks 2 --weekday mon --time 10:00 --reference-date 2026-05-25
```

Create interval:

```powershell
SmartAlarmCli.exe add --message "Water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from trigger --days mon,tue,wed,thu,fri
```

Update:

```powershell
SmartAlarmCli.exe update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --message "Drink water" --enabled false
SmartAlarmCli.exe update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --schedule-type weekly --days mon,wed --time 10:30
```

Common options:

```text
--enabled true|false
--color #RRGGBB
--sound preset-id|custom
--pattern custom-pattern
--volume 0..100
--play-count 0..999
```

## Runtime Popup

`trigger` shows a popup immediately and does not save anything to JSON. It
ignores the global runtime toggle.

```powershell
SmartAlarmCli.exe trigger --message "Build finished"
SmartAlarmCli.exe trigger --message "Build finished" --color "#2F80ED" --sound gentle_chime --volume 70 --play-count 1
SmartAlarmCli.exe trigger --message "Custom alert" --sound custom --pattern "880/150, _/50, 660/150"
```

JSON output returns a runtime-only UUID:

```powershell
SmartAlarmCli.exe trigger --message "Done" --json
```

```json
{"ok":true,"data":{"uuid":"8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c","runtimeOnly":true}}
```

Dismiss or snooze an active popup:

```powershell
SmartAlarmCli.exe dismiss --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
SmartAlarmCli.exe snooze --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

## Runtime State

```powershell
SmartAlarmCli.exe status
SmartAlarmCli.exe enable-runtime
SmartAlarmCli.exe disable-runtime
SmartAlarmCli.exe reset-interval --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

`disable-runtime` does not close already open popups and does not stop currently
playing sound.

## Formats

```text
date: yyyy-MM-dd
time: HH:mm
days: mon,tue,wed,thu,fri,sat,sun
schedule-type: once, weekly, nth-week, interval
count-from: trigger, confirmation
sound presets: classic_beep, double_beep, digital_alert, gentle_chime, urgent, soft_pulse, high_low_alert, triple_pulse
```
