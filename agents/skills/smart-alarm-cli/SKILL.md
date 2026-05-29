---
name: smart-alarm-cli
description: Creates and manages reminders, alarms, and notifications in the installed Smart Alarm app through its CLI. Use when the user asks to remind them later, set an alarm, schedule a notification, show a popup now, snooze/dismiss a reminder, or manage Smart Alarm entries.
---

# Smart Alarm CLI

## Rules

- Use the CLI only; never edit `settings.json` directly.
- The tray app must already be running. If CLI returns `app_not_running`, report that Smart Alarm is not running and stop.
- Use `--json` when automation needs stable parsing.
- Public identifiers are `uuid` values without braces. Do not use short ids.
- `trigger` is runtime-only: it shows a popup now, does not save JSON, and ignores the global runtime toggle.

## Executable

Prefer `SmartAlarm.exe` from `PATH`. The Windows installer adds Smart Alarm to the current user's `PATH`; already-open shells may need restart after installation.

If `PATH` lookup fails on Windows, also try:

```powershell
$env:LOCALAPPDATA\Programs\SmartAlarm\bin\SmartAlarm.exe
```

## Health Check

```powershell
SmartAlarm.exe cli status --json
```

If `ok=false` and `error.code=app_not_running`, do not start the app unless the user explicitly asks.

## Common Commands

List saved notifications:

```powershell
SmartAlarm.exe cli list --json
```

Create a one-time notification:

```powershell
SmartAlarm.exe cli add --message "Stand up" --schedule-type once --date 2026-05-29 --time 15:30 --json
```

Create a weekly notification:

```powershell
SmartAlarm.exe cli add --message "Gym" --schedule-type weekly --days mon,wed,fri --time 18:00 --json
```

Create an interval notification:

```powershell
SmartAlarm.exe cli add --message "Water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from trigger --days mon,tue,wed,thu,fri --json
```

Update common fields:

```powershell
SmartAlarm.exe cli update --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --message "Drink water" --enabled true --json
```

Delete immediately:

```powershell
SmartAlarm.exe cli delete --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --json
```

Trigger a runtime-only popup:

```powershell
SmartAlarm.exe cli trigger --message "Build finished" --color "#2F80ED" --sound gentle_chime --volume 70 --play-count 1 --json
```

Dismiss or snooze an active popup:

```powershell
SmartAlarm.exe cli dismiss --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --json
SmartAlarm.exe cli snooze --uuid 8b2b4a5e-8f52-4b27-9b27-efb17f9f4b4c --json
```

Enable or disable future scheduled notifications:

```powershell
SmartAlarm.exe cli enable-runtime --json
SmartAlarm.exe cli disable-runtime --json
```

## Scheduling

Use these schedule types:

- `once`: requires `--date yyyy-MM-dd --time HH:mm`
- `weekly`: requires `--days mon,wed --time HH:mm`; optional `--start-date`, `--end-date`
- `nth-week`: requires `--every-weeks N --weekday mon --time HH:mm --reference-date yyyy-MM-dd`; optional `--end-date`
- `interval`: requires `--every-minutes N --from HH:mm --to HH:mm --count-from trigger|confirmation`; optional `--days`, `--start-date`, `--end-date`, `--snooze-minutes`

For interval date limits, pass `--days`; date-only limits are rejected.

## Sound

Preset ids:

```text
classic_beep, double_beep, digital_alert, gentle_chime, urgent, soft_pulse, high_low_alert, triple_pulse
```

Custom sound:

```powershell
SmartAlarm.exe cli trigger --message "Custom alert" --sound custom --pattern "880/150, _/50, 660/150" --json
```

## Exit Codes

- `0`: success
- `1`: validation error
- `2`: app is not running
- `3`: operation failed, not found, or popup not active
- `4`: protocol/internal error

Full CLI documentation is installed with Smart Alarm as `CLI.md`.
