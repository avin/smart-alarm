# Smart Alarm CLI

Smart Alarm exposes a command-line mode for scripts and LLM agents:

```powershell
SmartAlarm.exe cli <command> [options]
```

The Windows installer adds the Smart Alarm `bin` directory to the current
user's `PATH`. Terminals that were already open during installation might need
to be restarted before `SmartAlarm.exe` is resolved by name.

The CLI is a client for the already running tray application. It never edits
`settings.json` directly and never starts Smart Alarm automatically.

If the tray application is not running, commands return `app_not_running` and
exit with code `2`.

## Output Modes

Human-readable output is the default:

```powershell
SmartAlarm.exe cli status
```

Machine-readable output is available with `--json`:

```powershell
SmartAlarm.exe cli status --json
```

Successful JSON response:

```json
{"ok":true,"data":{}}
```

Error JSON response:

```json
{"ok":false,"error":{"code":"app_not_running","message":"Smart Alarm is not running"}}
```

## Exit Codes

| Code | Meaning |
| ---: | --- |
| `0` | Success |
| `1` | Validation error or bad arguments |
| `2` | Smart Alarm is not running |
| `3` | Operation failed inside the running app |
| `4` | Protocol or internal error |

## UUID Format

The public CLI uses `uuid`, not `id`. UUID values are printed without braces:

```text
8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

Commands may accept UUIDs with braces, but all output is canonical without
braces.

## Persistent Notification Commands

### List Notifications

```powershell
SmartAlarm.exe cli list
SmartAlarm.exe cli list --json
```

`list` returns only persistent notifications stored in the application config.
Runtime-only notifications created by `trigger` are not included.

### Get Notification

```powershell
SmartAlarm.exe cli get --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

### Delete Notification

```powershell
SmartAlarm.exe cli delete --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

`delete` removes immediately. There is no CLI confirmation prompt.

### Add Notification

Common options:

| Option | Values |
| --- | --- |
| `--message` | Required text |
| `--enabled` | `true`, `false`, `yes`, `no`, `1`, `0` |
| `--color` | `#RRGGBB` |
| `--sound` | preset id or `custom` |
| `--pattern` | custom sound pattern, only with `--sound custom` |
| `--volume` | `0..100` |
| `--play-count` | `0..999`, where `0` means until dismissed |

Preset ids:

```text
classic_beep
double_beep
digital_alert
gentle_chime
urgent
soft_pulse
high_low_alert
triple_pulse
```

Once schedule:

```powershell
SmartAlarm.exe cli add --message "Stand up" --schedule-type once --date 2026-05-29 --time 15:30
```

Weekly schedule:

```powershell
SmartAlarm.exe cli add --message "Gym" --schedule-type weekly --days mon,wed,fri --time 18:00
```

Weekly schedule with date range:

```powershell
SmartAlarm.exe cli add --message "Report" --schedule-type weekly --days mon --time 09:00 --start-date 2026-06-01 --end-date 2026-06-30
```

Nth-week schedule:

```powershell
SmartAlarm.exe cli add --message "Review" --schedule-type nth-week --every-weeks 2 --weekday mon --time 10:00 --reference-date 2026-05-25
```

Nth-week schedule with end date:

```powershell
SmartAlarm.exe cli add --message "Review" --schedule-type nth-week --every-weeks 2 --weekday mon --time 10:00 --reference-date 2026-05-25 --end-date 2026-12-31
```

Interval schedule:

```powershell
SmartAlarm.exe cli add --message "Water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from trigger
```

Interval schedule with weekday limit:

```powershell
SmartAlarm.exe cli add --message "Water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from trigger --days mon,tue,wed,thu,fri
```

Interval schedule with date range:

```powershell
SmartAlarm.exe cli add --message "Water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from confirmation --days mon,tue,wed,thu,fri --start-date 2026-06-01 --end-date 2026-06-30
```

Interval-specific option:

```text
--snooze-minutes 0..1440
```

For persistent interval notifications, `0` means use the global default snooze
interval.

### Update Notification

Common fields can be patched without changing the schedule:

```powershell
SmartAlarm.exe cli update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --message "Drink water" --enabled false
```

Schedule updates are strict: pass `--schedule-type` and all required fields for
the new schedule. Deep partial schedule patches are intentionally not supported.

```powershell
SmartAlarm.exe cli update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --schedule-type weekly --days mon,wed --time 10:30
```

Sound updates are also full replacements:

```powershell
SmartAlarm.exe cli update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --sound custom --pattern "880/150, _/50, 660/150"
```

## Runtime Trigger

`trigger` shows a popup immediately and does not save anything to JSON.
It ignores the global runtime toggle.

```powershell
SmartAlarm.exe cli trigger --message "Build finished"
```

With explicit options:

```powershell
SmartAlarm.exe cli trigger --message "Build finished" --color "#2F80ED" --sound gentle_chime --volume 70 --play-count 1
```

With custom sound:

```powershell
SmartAlarm.exe cli trigger --message "Custom alert" --sound custom --pattern "880/150, _/50, 660/150" --volume 80 --play-count 3
```

Runtime trigger defaults:

| Option | Default |
| --- | --- |
| `--color` | `#D94841` |
| `--sound` | `gentle_chime` |
| `--volume` | `70` |
| `--play-count` | `1` |
| `--snooze-minutes` | `0`, use global default |

JSON output includes the runtime-only UUID:

```powershell
SmartAlarm.exe cli trigger --message "Done" --json
```

```json
{"ok":true,"data":{"uuid":"8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c","runtimeOnly":true}}
```

The returned UUID can be used with `dismiss` and `snooze` while the popup is
active.

## Runtime State Commands

Status:

```powershell
SmartAlarm.exe cli status
SmartAlarm.exe cli status --json
```

Enable or disable future scheduled notifications:

```powershell
SmartAlarm.exe cli enable-runtime
SmartAlarm.exe cli disable-runtime
```

This does not close already open popups and does not stop currently playing
sound.

Reset an interval notification timer:

```powershell
SmartAlarm.exe cli reset-interval --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

Dismiss or snooze an active popup:

```powershell
SmartAlarm.exe cli dismiss --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
SmartAlarm.exe cli snooze --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c
```

If the UUID does not currently have an active popup, these commands return
`not_active`.

## Date, Time, and Day Formats

Dates:

```text
yyyy-MM-dd
```

Times:

```text
HH:mm
```

Weekdays:

```text
mon,tue,wed,thu,fri,sat,sun
```

Schedule types:

```text
once
weekly
nth-week
interval
```

`count-from` values:

```text
trigger
confirmation
```
