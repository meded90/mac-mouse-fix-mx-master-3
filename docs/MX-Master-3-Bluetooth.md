# MX Master 3 Bluetooth special buttons — experimental fork

This is a derivative of [Mac Mouse Fix](https://github.com/noah-nuebling/mac-mouse-fix), not an official release. Source and original monetization/licensing remain under the unchanged [MMF License](../License).

## Scope and evidence

Read-only IORegistry inspection on the development Mac, 2026-10-12, found **MX Master 3**, vendor `0x046D`, product `0xB023`, transport **Bluetooth Low Energy**, vendor collection usage page `0xFF43`, usage `0x0202`, and a 20-byte report `0x11`. No serial number, Bluetooth address, screenshots, credentials or personal config are included here.

This confirms model, connection and available transport, **not** the physical CID assignments or successful firmware diversion. The helper has not been launched, the device has not been reconfigured, Mouser has not been stopped, and macOS permissions have not been changed. Physical validation is still required.

Only that VID/PID and Bluetooth transport are matched. No support claim for MX Master 3S, USB/Unifying/Bolt, wheel tilt, DPI, Back/Forward or other models is made. Back/Forward remain on their existing MMF path.

## Why this implementation

| Reference | Useful part | Why not use the whole implementation |
| --- | --- | --- |
| [#1710](https://github.com/noah-nuebling/mac-mouse-fix/pull/1710), PradyumnaKrishna, `23f3838b6818297be89566ec23d4bdc784c23f3d` | HID++ feature discovery and temporary diversion | Adds hidapi and a second drag-event path; focuses on Back/Forward; more transport/state complexity than needed |
| [#1856](https://github.com/noah-nuebling/mac-mouse-fix/pull/1856), Miguel Angelo, `b430bd0cb66423de1f74863cea71cbe1df1149c2` | Thumb CID groundwork | Global response matching; only first CID decoded; releases all only on zero CID; direct Buttons calls bypass the current event-tap thread contract; reported test was 3S/Bolt |
| [#1904](https://github.com/noah-nuebling/mac-mouse-fix/pull/1904), Miguel Angelo, `56c951bacba7260ccc94ecb1c68d4b40fe69de56` | Bluetooth HID++ groundwork | Over 5,000 inserted lines, unrelated UI, scrolling, drag actions and brightness changes |
| [#1995](https://github.com/noah-nuebling/mac-mouse-fix/pull/1995), lcandy2, `a874b40130ba0ab9bbea9e3eab57b5fa5de12f34` | IOHID transport, serialized background request queue, sender-tagged CGEvents | Broad model/receiver/control policy; unchecked partial frames; first-CID-only verification; cross-thread restoration; current master has no SIGTERM cleanup |

`LogitechButtonDiverter.h/.m` and six Xcode project entries are **adapted from lcandy2's #1995** under the MMF License. The ancestor implementation credits PradyumnaKrishna, Miguel Angelo and nikuscs for earlier groundwork. Their complete PRs and hidapi have not been imported. The pure protocol/state helpers and tests were written for this fork. [Solaar's `hidpp20.py`](https://github.com/pwr-Solaar/Solaar/blob/master/lib/logitech_receiver/hidpp20.py) was consulted for protocol semantics (valid bit, CID reporting and unchanged remap), not copied. No Solaar GPL implementation is linked or vendored.

Base upstream SHA: `14ecba1fbf7c6e8c87385e999ae8cc93eceae872`.

## Button policy and normal MMF bindings

Enable only in a separately agreed physical test session, with MMF disabled first and a backup of its config:

```xml
<!-- Add these BOOLEAN entries inside the existing General dictionary. -->
<key>logitechMXMaster3SpecialButtons</key><true/>
<key>logitechMXMaster3DivertModeShift</key><true/>
```

The keys are startup policy. Restart the helper after changing them. Missing/false `logitechMXMaster3SpecialButtons` means **no HID++ manager, probing or writes**. Missing/false `logitechMXMaster3DivertModeShift` leaves mode shift native and enables only thumb diversion. There is deliberately no automatic change to the user's live config.

- Thumb gesture button below Back/Forward: advertised divertable CID `0x00D0`, or legacy candidate `0x00C3`, becomes MMF **Button 6**. Only one alias is selected; unknown CIDs are ignored.
- Mode shift button above the main wheel: advertised divertable CID `0x00C4` becomes MMF **Button 7** when separately opted in.

The helper discovers `ReprogControlsV4` (`0x1B04`), enumerates advertised controls and checks divertability before writing. It skips controls already diverted by another program. It only changes temporary diversion (flags `0x03` to enable, `0x02` to disable), with remap zero meaning unchanged. Permanent diversion, raw XY diversion and firmware wheel configuration are not enabled.

Down/up events carry the real IORegistry sender ID (MMF field 87) and the integer pressure representation used by `ButtonInputReceiver`. Virtual button count is exposed to SwitchMaster and the normal Add/binding UI. Click, double click, hold, click-and-drag gestures and click-and-scroll use the existing MMF button/modifier/drag pipeline. Mouse movement continues normally: no second synthetic drag stream or raw XY suppression is introduced. This describes the integration contract; actual hold/drag smoothness must still be tested.

### Wheel-mode compromise

Diverting mode shift disables its firmware ratchet/free-spin toggle. There is no claim that the same click can both run an arbitrary MMF action and retain that firmware toggle.

To **preserve physical mode switching**, set `logitechMXMaster3DivertModeShift` false (or remove it) and restart MMF; thumb remains assignable. To assign **both** buttons, opt in to mode shift diversion and accept losing its native toggle while MMF runs. Disabling this fork restores native mode shift; power-cycle the mouse if restoration cannot be confirmed. Existing firmware SmartShift is not changed, but its behavior is not validated by this work. No untested replacement wheel-mode command/action is shipped.

## Recovery and limitations

A complete notification is a four-CID held snapshot. Releases are emitted before new presses; aliases and duplicates cannot create double downs. Malformed/foreign reports do not mutate state. Requests match device, feature, function and rotating software ID; CID replies are checked against the requested CID. A lost set-report ACK retains cleanup ownership and triggers rollback rather than orphaning the write.

On interface removal, sleep, reactivation or unreachable-device verification, held buttons are released. Wake reprobes after three seconds; wireless-status notifications reprobe after 0.3 seconds. Every 30 seconds the helper verifies **every** owned CID and retries an initially sleeping device. Traffic stays on a dedicated run-loop worker.

Normal app termination, SIGTERM/SIGINT (including launchd disable) serialize restoration and close the manager/callbacks. Shutdown cancels ordinary probing and waits at most four seconds for cleanup. SIGKILL, power loss, a disconnected/sleeping mouse or a failed transport cannot guarantee a confirmed restore; mouse power cycling is the recovery path. There is no hardware compare-and-swap ownership primitive: do not run competing remappers during the physical test. A dropped release notification with an otherwise reachable device cannot be distinguished from an intentional indefinite hold; subsequent snapshots/reconnect resolve it. Recovery releases may complete a pending MMF click/hold action: use harmless bindings in validation.

The test bundle retains upstream bundle/launchd identifiers and uses the existing MMF config. It is built in a separate directory, **not installed**, and is not a safe concurrent installation. Do not launch it beside an installed MMF instance. No notarization or distribution signing is claimed.

## Reproducible checks

```sh
Tests/Logitech/run.sh
plutil -lint 'Mouse Fix.xcodeproj/project.pbxproj'
git diff --check
xcodebuild -project 'Mouse Fix.xcodeproj' -scheme App -configuration Debug \
  -derivedDataPath ../build-mx-master-3 CODE_SIGNING_ALLOWED=NO build
```

Tests compile production pure framing/response/state code and production activation/verification/restoration methods against a fake in-memory transport. CGEvent posting is intercepted in the tests; no event is posted and no HID device or manager is opened. AddressSanitizer/UBSan cover framing, request correlation, high-byte CID filtering, aliases, malformed input, duplicate/indefinite hold, simultaneous buttons, 64 exhaustive transitions, native policy, foreign ownership, lost-ACK rollback, recovery of the second CID, initial sleep retry, shutdown idempotence, and the sender/button/pressure event contract. These are not physical integration tests.

Xcode 27.0 (`27A266a`) Debug App+Helper build passed on this Mac after allowing Xcode's own Swift macro plugin outside the execution sandbox. Existing deployment-target/deprecation warnings remain. The app/helper were not run.

No AGENTS.md or .agents/skills instructions were present in this checkout or its checked workspace ancestors. Local memory was not modified.

## Minimal safe physical test — requires a separate agreed session

1. Keep the built archive unlaunched until the session is agreed. Have the normal installed MMF/Mouser available, and back up the MMF config. Use trackpad/keyboard for recovery. Record normal wheel-mode behavior first.
2. In that session, disable existing MMF and pause Mouser by explicit consent. Do not auto-grant Accessibility/Input Monitoring. Inspect/agree any permission request before proceeding. Extract the test bundle to a temporary directory, not `/Applications`.
3. Start with thumb only. Confirm the logged advertised CID and sender device, then use normal MMF Add mode to bind Button 6 to a harmless action. Check one down/up, click, double click, a long hold, click-and-drag and release during movement; check Back/Forward and scrolling for regressions.
4. Enable mode shift separately after accepting the wheel-mode compromise. Repeat for Button 7, including both buttons held and released in both orders. Verify no duplicate or stuck modifiers and no cursor jump.
5. With harmless bindings, test reconnect, mouse power cycle and system sleep/wake, including while holding. Allow the 30-second verification interval. Disable MMF via its normal toggle; confirm native thumb/mode shift behavior returns. Do not mark physical support verified until these checks pass.
6. Roll back: disable the experimental helper normally, restore the backed-up config, power-cycle the mouse if restoration is uncertain, return to the original MMF/Mouser arrangement. Do not use force-kill as the normal cleanup path. No rollback script is run automatically.
