---
name: smart-alarm-cli
description: Creates and manages reminders, alarms, and notifications in Smart Alarm. Use when the user asks to be reminded later, set an alarm, schedule or edit notifications, show a popup now, snooze or dismiss a reminder, or inspect Smart Alarm state.
---

# Smart Alarm CLI

## Use This For

- "Remind me at 14:00 to put the kettle on" -> create a saved `once` notification.
- "Remind me every weekday at 10:00" -> create a saved `weekly` notification.
- "Remind me every 40 minutes during work hours" -> create a saved `interval` notification.
- "Show this alert now" -> use `trigger`.
- "Delete/update/snooze/dismiss/list my reminders" -> use the matching CLI command.

## Command Basics

Use JSON output for automation:

```powershell
SmartAlarmCli.exe <command> [options] --json
```

If `SmartAlarmCli.exe` is not on `PATH`, try:

```powershell
%LOCALAPPDATA%\Programs\SmartAlarm\bin\SmartAlarmCli.exe
```

Do not edit `settings.json` directly.

If a command returns:

```json
{"ok":false,"error":{"code":"app_not_running"}}
```

tell the user Smart Alarm is not running. Do not start the app unless the user explicitly asks.

Use full `uuid` values from CLI output. Do not invent or shorten UUIDs.

## Choose The Right Command

- Saved future reminder: `add`
- Immediate unsaved popup: `trigger`
- Change saved reminder: `update --uuid UUID`
- Remove saved reminder: `delete --uuid UUID`
- See saved reminders: `list`
- See live runtime state: `status`
- Dismiss/snooze open popup: `dismiss` / `snooze`
- Pause/resume future scheduled notifications: `disable-runtime` / `enable-runtime`

## Create Saved Reminders

Once:

```powershell
SmartAlarmCli.exe add --message "Put the kettle on" --schedule-type once --date 2026-05-30 --time 14:00 --json
```

Weekly:

```powershell
SmartAlarmCli.exe add --message "Gym" --schedule-type weekly --days mon,wed,fri --time 18:00 --json
```

Nth week:

```powershell
SmartAlarmCli.exe add --message "Review" --schedule-type nth-week --every-weeks 2 --weekday mon --time 10:00 --reference-date 2026-05-25 --json
```

Interval:

```powershell
SmartAlarmCli.exe add --message "Drink water" --schedule-type interval --every-minutes 40 --from 10:00 --to 18:00 --count-from trigger --days mon,tue,wed,thu,fri --json
```

Useful shared options:

```text
--enabled true|false
--color #RRGGBB
--sound gentle_chime
--volume 0..100
--play-count 0..999
```

## Immediate Popup

Use `trigger` when the user wants an alert now. It is runtime-only and is not saved.

```powershell
SmartAlarmCli.exe trigger --message "Build finished" --json
SmartAlarmCli.exe trigger --message "Custom alert" --color "#2F80ED" --sound gentle_chime --volume 70 --play-count 1 --json
```

The JSON response includes a `uuid` that can be used with `dismiss` or `snooze` while the popup is active.

## Manage Existing Reminders

```powershell
SmartAlarmCli.exe list --json
SmartAlarmCli.exe get --uuid UUID --json
SmartAlarmCli.exe update --uuid UUID --message "Drink water" --enabled true --json
SmartAlarmCli.exe update --uuid UUID --schedule-type weekly --days mon,wed --time 10:30 --json
SmartAlarmCli.exe delete --uuid UUID --json
```

Schedule updates require `--schedule-type` and all required fields for the new schedule.

## Runtime Controls

```powershell
SmartAlarmCli.exe status --json
SmartAlarmCli.exe dismiss --uuid UUID --json
SmartAlarmCli.exe snooze --uuid UUID --json
SmartAlarmCli.exe enable-runtime --json
SmartAlarmCli.exe disable-runtime --json
SmartAlarmCli.exe reset-interval --uuid UUID --json
```

`disable-runtime` affects only future scheduled notifications. It does not close open popups.

## Formats

- date: `yyyy-MM-dd`
- time: `HH:mm`
- days: `mon,tue,wed,thu,fri,sat,sun`
- schedule types: `once`, `weekly`, `nth-week`, `interval`
- count-from: `trigger`, `confirmation`
- presets: `classic_beep`, `double_beep`, `digital_alert`, `gentle_chime`, `urgent`, `soft_pulse`, `high_low_alert`, `triple_pulse`

For custom sound:

```powershell
SmartAlarmCli.exe trigger --message "Custom alert" --sound custom --pattern "880/150, _/50, 660/150" --json
```
