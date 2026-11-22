# Prismatik Audit: Unrecoverable Grabbing & Device Halt Edge Cases

**Scope:** Screen grabbing (`Software/grab/*`, `GrabManager`) and LED device management (`LedDeviceManager`, `AbstractLedDevice*`, `LedDevice*`, `LightpackApplication`, `SystemSession`).  
**Goal:** Identify edge cases that can permanently halt grabbing or LED output without automatic recovery, even when hardware is healthy or only failed transiently — and evaluate reachability, masking factors, and appropriate fixes for each.

---

## Executive Summary

| # | Area | Issue | Permanent Halt? | Real-World Likelihood |
|---|---|---|---|---|
| 1.1 | `LedDeviceManager` / `GrabManager` | `commandCompleted(false)` suspends grabbing (and darkens moodlamp/sound output) without arming recovery; device↔manager signal contract is not 1-ack-per-command | Yes (closed-handle paths) | Medium (sleep/wake with missed resume, close races; LED-count mismatch is latent) |
| 1.2a | `LedDeviceLightpack` | `closeDevices()` permanently blocks `m_timerPingDevice` signals | Indirect (kills ping watchdog) | **Very High** (every disconnect/reopen/sleep) |
| 1.2b | `LedDeviceLightpack` | `timerPingDeviceTimeout()` closes device right after opening it | Yes (if reached) | Low today (masked by 1.2a), **High once 1.2a is fixed** |
| 1.2c | `LedDeviceLightpack` | Use-after-free on `phid_device` after `tryToReopenDevice()` | Crash / recovery loop | **High** (any transient USB write error) |
| 1.3 | `LedDeviceManager` | `switchOffLeds()` / `switchOnLeds()` queue race latches `StatusOff` | Yes (LED output) | Medium–High (rapid toggles, lock/display events) |
| 1.4 | `LedDeviceManager` / Session | Cross-thread `close()`, missed `PBT_APMRESUMEAUTOMATIC`, sleep/wake ordering, `--nogui` null deref | Yes (halt or crash) | **High** (sleep/wake, Modern Standby, timed-out I/O) |
| 1.5 | `Adalight` / `Ardulight` | Stalled serial TX buffer (`bytesToWrite() > 0`) silently drops all frames forever | Yes (LED output) | **High** (USB-serial disconnect/glitch mid-stream) |
| 1.6 | `AbstractLedDevice` | Settings changes while backlight is off always time out (500ms) | No (spurious reopen) | **High** (deterministic when changing settings while off) |
| 1.7 | `LedDeviceAlienFx` | Transient init failure in constructor is never retried | Yes (LED output) | Medium (AlienFX service slow at login) |
| 1.8 | `GrabManager` / `LedDeviceManager` | Uninitialized member variables (`m_isGrabbingSuspendedDueToDeviceError`, `m_ledDevice`) | UB (mostly masked at init) | Low |
| 2.1a | `DDuplGrabber` | Transient `DXGI_ERROR_UNSUPPORTED` or `init()` failure latches `Unavailable` forever | **Yes** | **Very High** (RDP, FSE games, DWM/GPU reset, sleep) |
| 2.1b | `DDuplGrabber` | `DXGI_ERROR_DEVICE_REMOVED` / `DEVICE_RESET` / `SESSION_DISCONNECTED` / D3D11 errors never trigger realloc | **Yes** | **High** (GPU TDR, driver update, RDP/FUS) |
| 2.1c | `DDuplGrabber` | Stale `IDXGIFactory1` / `m_adapters` never refreshed on hotplug/wake/TDR | **Yes** | **Very High** (DisplayPort/USB-C wake, docks, KVMs) |
| 2.1d | `DDuplGrabber` | Multi-monitor starvation & NULL-deref crash when Monitor 0 is static | Yes / Crash | Medium (multi-monitor setups) |
| 2.1e | `DDuplGrabber` | `runThreadCommand(INFINITE)` hang & 4× under-allocation in `returnBlackBuffer()` | Hang / Crash | Low–Medium (multi-monitor `E_ACCESSDENIED`) |
| 2.2 | `WinAPIGrabber` | Unchecked GDI calls silently freeze forever + `DeleteObject(HDC)` handle leak | **Yes** | High (display sleep/lock/mode switch) |
| 2.3 | `MacOSAVGrabber` | Runtime error, interruption, or display ID change permanently halts capture | **Yes** | **Very High** (external displays, macOS sleep/wake) |
| 2.4 | `X11Grabber` | Unchecked display/XShm errors & unhandled `BadMatch` abort | Yes / Crash | Medium (XRandR changes, SHM limits) |
| 2.5 | `GrabManager` / `GrabWidget` / `D3D10` | Stuck drag-pause flag; `D3D10Grabber` `WAIT_ABANDONED`/handoff desync | **Yes** | Low–Medium (mid-drag Alt+Tab/UAC; DX hook users) |

---

## Part 1: Device Recovery & `GrabManager` ↔ `LedDeviceManager` Logic

### 1.1 `commandCompleted(false)` suspends grabbing without scheduling recovery

**Locations:**
- `Software/src/LedDeviceManager.cpp:107-126, 377-462, 596-714`
- `Software/src/GrabManager.cpp:179-201`
- `Software/src/AbstractLedDevice.cpp:32-108`
- `Software/src/LedDeviceLightpack.cpp:54-55, 77-86, 147-168, 242-290, 375-384, 471-532`
- `Software/src/LedDeviceAdalight.cpp:191-197, 258-299` (and the matching `LedDeviceArdulight.cpp:196-205, 270-306`)
- `Software/src/AbstractLedDeviceUdp.cpp:65-80, 139-156`

**What the code does:**
`LedDeviceManager::ledDeviceCommandCompleted(false)` emits `ioDeviceSuccess(false)`. That makes `GrabManager::ledDeviceCallSuccess(false)` call `start(false)` and set `m_isGrabbingSuspendedDueToDeviceError = true`. But it **never calls `triggerRecreateLedDevice()`**. Only `ledDeviceIoDeviceSuccess(false)`, `ledDeviceOpenDeviceSuccess(false)` and `ledDeviceCommandTimedOut()` start `m_recreateTimer`.

When suspended, `GrabManager` stops sending frames. It resumes only on `openDeviceSuccess(true)` or `ioDeviceSuccess(true)`. If nothing has armed a recovery, nothing will ever send one of those signals, so the halt is permanent.

**This isn't limited to grabbing.** `MoodLampManager`, `SoundManagerBase` and API `setcolor` keep sending frames to `LedDeviceManager::setColors()`. Each frame fails with `commandCompleted(false)` and never arms recovery. Those modes don't stop, but the **LEDs stay dark forever**.

**No hidden recovery path exists in practice.** The Lightpack ping timer could in theory rescue it: a successful NOP ping emits `ioDeviceSuccess(true)` every second, and when `m_devices` is empty, `timerPingDeviceTimeout()` reopens the device and emits `openDeviceSuccess(true)`. But every path that empties `m_devices` goes through `closeDevices()`, which permanently disables the ping (§1.2a). Non-Lightpack devices have no equivalent.

**Can this actually happen without `m_recreateTimer` already running?**
Here is every path that emits `commandCompleted(false)` without also emitting `ioDeviceSuccess(false)` or `openDeviceSuccess(false)`:

1. **Startup before `open()` / after a failed `open()`: MASKED.**  
   `LightpackApplication::startLedDeviceManager()` queues `init()` before `GrabManager` is connected, so `ledDeviceOpen()` runs on `m_ledDeviceThread` before any `setColors()`. If `open()` fails, `openDeviceSuccess(false)` already arms `m_recreateTimer`, and each failed retry re-arms it.
2. **Write error on an open handle (`hid_write`, short `QSerialPort::write`, `QUdpSocket::write`): MASKED.**  
   All of these emit `ioDeviceSuccess(false)` before returning `false`, so recovery is armed. *(Exception: a stalled serial TX buffer returns `true` and never reports an error at all. See §1.5.)*
3. **Command reaches a closed or null handle: REACHABLE.** This is the only failure class that is not masked today. All of these return `false` with **no** `ioDeviceSuccess(false)`:
   - Lightpack: `m_devices.size() == 0` in `setColors()` (`LedDeviceLightpack.cpp:77-80`) and `writeBufferToAllDevicesWithCheck()` (`:376`), which covers `switchOffLeds()` and every settings command.
   - Adalight / Ardulight: `m_AdalightDevice == NULL || !isOpen()` (`LedDeviceAdalight.cpp:275-276`, `LedDeviceArdulight.cpp:282-283`).
   - UDP (`Drgb`/`Dnrgb`/`Warls`): `m_Socket == NULL` (`AbstractLedDeviceUdp.cpp:143-144`).

   The device can end up closed while `m_recreateTimer` is idle in two ways:
   - **3a. Sleep: halts permanently only if the resume event is missed.** `GrabManager` does **not** stop on `Sleeping`: `GrabManager::onSessionChange` is only a signal forwarded to `DDuplGrabber` (`GrabManager.hpp:53`). So frames keep arriving after `LedDeviceManager::onSessionChange(Sleeping)` closes the device. They reach the closed handle in two cases:
     - `isKeepLightsOnAfterSuspend` is `true`;
     - a command was in flight when `switchOffLeds()` arrived. `OffLeds` is then only queued and `m_backlightStatus` stays `StatusOn` (§1.3).

     Either way `GrabManager` suspends. A normal resume recovers: `Resuming` → `recreateLedDevice()` → `openDeviceSuccess(true)`. A missed `PBT_APMRESUMEAUTOMATIC` (§1.4) halts it permanently.
   - **3b. Races: REACHABLE.** Examples: the cross-thread `close()` in `recreateLedDevice()` / `onSessionChange` (§1.4), or the Lightpack ping self-close (§1.2b, mostly masked today). Any race that leaves the device closed while `m_recreateTimer` is idle turns the next frame into a permanent halt.
4. **`LedDeviceLightpack::setColors()` with `colors.count() > maxLedsCount()`: LATENT (unreachable in the current build).**  
   An earlier version of this audit called this reachable via a profile switch. That was wrong:
   - LED count is a **global** setting (`Main::Key::Lightpack::NumberOfLeds`), not a per-profile one, so switching profiles never changes it.
   - The only runtime writer is the setup wizard (`wizard/GlobalColorCoefPage.cpp:158`). The wizard runs as a **separate process** after the main instance quits (`SettingsWindow.cpp:2310-2321` launches `--wizard` and calls `quit()`; `LightpackApplication.cpp:517-526` sends `quitForWizard` and waits).
   - The plugin/API `SetCountLeds` is disabled: it returns `false` before doing anything (`LightpackPluginInterface.cpp:523-526`).
   - The runtime `connectedDeviceChanged` handler is commented out (`LightpackApplication.cpp:670`).
   - `LedDeviceLightpack::open()` refuses to open fewer devices than configured (`:278-283`), so `m_devices` is either empty or large enough.
   - The plugin interface always sends 10 colours (`LightpackPluginInterface.cpp:23`). That is ≤ the minimum `maxLedsCount()` of an open Lightpack.

   This becomes reachable if runtime LED-count or device-type changes are ever re-enabled. Even then, it halts permanently only if the ping timer is already dead (§1.2a). Otherwise the result is a 1 Hz suspend/resume flap driven by ping `ioDeviceSuccess(true)`.

**Today's signal contract (what actually emits what)**

The recovery fix depends on the device ↔ manager signals meaning what their names say. Today they don't:

| Area | Actual behaviour | Consequence |
|---|---|---|
| `commandCompleted` count | **Not 1 per command.** `LedDeviceLightpack::updateDeviceSettings()` emits **5** (one `setColors()` via `AbstractLedDevice::updateDeviceSettings()` plus 4 setters). Adalight/Ardulight emit **2** (`setColors()` plus `setColorSequence()` → `setColors()`). | The first ack completes the command. The rest each stop `m_cmdTimeoutTimer` and pop the next queued command early, or set `m_isLastCommandCompleted = true` while the device thread is still busy. Several commands end up in flight, and timeout protection is lost. |
| Unsolicited acks | `LedDeviceLightpack::open()` calls `updateDeviceSettings()` (`:287`): **5 acks per open**, including reopens via `tryToReopenDevice()` mid-command. `ledDeviceOpenDeviceSuccess(true)` sends a raw `ledDeviceUpdateDeviceSettings()` (`LedDeviceManager.cpp:403`) around the queue and timer: another 5 / 2 / 1 acks (Lightpack / serial / UDP). The serial last-will timer calls `setColors()` (`LedDeviceAdalight.cpp:263-269`). `close()` calls `writeLastWill(true)`, so when `close()` is called cross-thread (§1.4), that ack is emitted **from the manager thread**. `switchOnLeds()` also bypasses the queue (§1.3). | Same as above. Under a merged error handler, a stray `false` ack would also start a reopen. |
| Missing acks | `AbstractLedDevice` setters with `updateColors == false` never ack (§1.6). | Spurious timeout → reopen. |
| `ioDeviceSuccess` | Emitted per HID write / serial write / UDP datagram: N+1 times per frame. `ledDeviceCommandTimedOut()` emits `ioDeviceSuccess(false)` twice (`:701-702`). | Noisy, but harmless today. |
| Ping coupling | `restartPingDevice` is connected to the device's **own** `ioDeviceSuccess` (`LedDeviceLightpack.cpp:54-55`). That's how "ping 1 s after the last transfer" works. | Removing per-write `ioDeviceSuccess` without rewiring changes the ping behaviour. |
| Failure clears queue | `ledDeviceCommandCompleted(false)` clears all of `m_cmdQueue` (`:392`). | Settings are reapplied on reopen, but a queued `OffLeds` is **silently lost** (worse together with §1.3). |
| Backoff reset | `m_failedCreationAttempts` resets on **any** `openDeviceSuccess(true)` (`:402`). UDP `open()` always reports success (`AbstractLedDeviceUdp.cpp:77`), and so does AlienFx (§1.7). | Persistent write failure on a device whose open "succeeds" gives a 200 ms reopen loop forever (DTR-resetting Arduinos on serial) and `GrabManager` flapping on and off. |
| Stale results | The device object is reused across recreations (`m_ledDevices` cache, `:446-460`). Qt still delivers queued events posted before a disconnect/reconnect. `recreateLedDevice()` doesn't stop `m_cmdTimeoutTimer`. | Results of commands issued before a recreate arrive after the new `open()` and look identical to fresh results. Example: the §1.4 resume ordering, where `switchOnLeds()` → `setColors()` hits the still-closed device. |
| Sleep vs. armed recovery | `onSessionChange(Sleeping)` doesn't stop `m_recreateTimer`. | A recovery armed just before suspend fires anyway and reopens the device during or after the suspend transition. |

**Should `commandCompleted(false)` and `ioDeviceSuccess(false)` be merged?**

*Original proposal:* merge the error handling inside `LedDeviceManager`. Route both through one `handleDeviceIoResult(bool ok)` that arms `triggerRecreateLedDevice()` on `!ok` unless `m_isSleeping`. Keep `commandCompleted(bool)` as the one-ack-per-command signal on `AbstractLedDevice`, and reserve `ioDeviceSuccess(false)` for out-of-band health events.

That direction is still right: `LedDeviceManager` already folds both into one outbound `ioDeviceSuccess(bool)`. But on its own it would:
- trigger reopens from stray and stale acks;
- assume a one-ack-per-command rule that doesn't hold today;
- spread the 200 ms reopen loop to more paths;
- break ping scheduling;
- leave an armed recovery able to fire into suspend.

The revised fix below covers those gaps.

#### Revised fix

**Design principle:** no per-command IDs and no generation number stamped by the device. No consumer cares *which* command an ack belongs to. The manager needs only two things from an ack: whether the in-flight slot is free, and whether the command worked. Matching each ack to the right command comes from three properties:
1. one ack per command;
2. first-in-first-out (FIFO) order on both threads;
3. a link state in the manager that sends nothing while a reopen is pending.

**Prerequisites:**
- §1.4 queued `close()` on `m_ledDeviceThread`. R2 **depends on this**: its ordering argument requires close and open to run behind any command already queued on the device thread.
- §1.2a–c Lightpack cleanups: remove `tryToReopenDevice()`, `blockSignals(true)`, the reopen branch in `timerPingDeviceTimeout()`, and `updateDeviceSettings()` inside `open()`.

**R1. Exactly one ack per command, by construction.**
- Every public command slot on a device acks exactly once. Paths that aren't manager commands never ack: `open()`, `close()` and the serial last-will timer use helpers that return a `bool` instead of calling the public slots.
- Composite commands (`updateDeviceSettings()`, `setColorSequence()` → `setColors()`) wrap their internal calls in an `AckBatch` guard, which ANDs the results into one ack.
- On the manager side, every command goes through one `sendCommand(cmd)` helper: public setters, `cmdQueueProcessNext()`, `switchOnLeds()` and the post-open replay. Remove the raw emits that bypass the queue (`ledDeviceOpenDeviceSuccess()` at `:403`, and `switchOnLeds()`).
- Add the missing `ack(true)` for `updateColors == false` (fixes §1.6).

```cpp
// AbstractLedDevice
void AbstractLedDevice::ack(bool ok) {
    if (m_batchDepth > 0) { m_batchOk = m_batchOk && ok; return; }   // inside a composite command
    emit commandCompleted(ok);
}

struct AckBatch {   // RAII: one ack for the whole composite command
    AbstractLedDevice *d;
    explicit AckBatch(AbstractLedDevice *d) : d(d) { if (d->m_batchDepth++ == 0) d->m_batchOk = true; }
    ~AckBatch() { if (--d->m_batchDepth == 0) d->ack(d->m_batchOk); }
};

void LedDeviceLightpack::updateDeviceSettings() {
    AckBatch batch(this);                          // 5 internal results -> 1 ack
    AbstractLedDevice::updateDeviceSettings();
    setUsbPowerLedDisabled(Settings::isDeviceUsbPowerLedDisabled());
    setRefreshDelay(Settings::getDeviceRefreshDelay());
    setColorDepth(Settings::getDeviceColorDepth());
    setSmoothSlowdown(Settings::getDeviceSmooth());
}

void LedDeviceAdalight::writeLastWill(bool force) {
    if (force || m_AdalightDevice->bytesToWrite() == 0)
        writeColors(m_colorsSaved);                // bool helper, no ack: not a manager command
}
```

**R2. Discard late results with a link state in the manager (no generations).**

`m_cmdQueue` holds only commands not yet sent, so clearing it doesn't remove an ack from a command already sent. Without anything else, that late ack would complete the *next* command, and a late `false` would start a needless reopen of the fresh device. These rules make late results harmless instead:
1. `recreateLedDevice()` resets the command state: clear the queue, stop the timeout, set `m_isLastCommandCompleted = true`. It then sets `m_linkState = Opening`, increments `m_pendingOpens`, and emits a queued close followed by a queued open.
2. The device thread is FIFO. Any command still in progress finishes, and acks, before close and open run. Queued signals from the device thread to the manager arrive in posting order, so the late ack reaches the manager **before** `openDeviceSuccess`.
3. While the link isn't `Ready`, the manager sends nothing:
   - Frames are dropped, but still saved to `m_savedColors`.
   - Settings changes are dropped, because the post-open replay (R5) reads current values from `Settings`. Values set through the plugin interface and not saved to `Settings` are lost, which already happens on every recreate today.

   So when the late ack arrives, nothing is in flight, and it is ignored.
4. Out-of-band `ioDeviceSuccess(false)` is ignored unless the link is `Ready`; the open result decides instead. This covers a ping failure posted just before the close.
5. Overlapping reopens (for example, the recovery timer fires, then `Resuming` arrives before that open finishes) are handled by `m_pendingOpens`. Only the result of the most recent open is acted on. Open results that arrive while sleeping are ignored; the queued close that follows leaves the device closed.

Link states: `Closed` (sleeping, or failed and waiting for the recovery timer), `Opening`, `Ready`.

```cpp
void LedDeviceManager::recreateLedDevice() {
    resetCommandState();                           // clear queue, stop cmd timeout, m_isLastCommandCompleted = true
    m_linkState = LinkState::Opening;
    ++m_pendingOpens;
    m_openTimeoutTimer->start();                   // e.g. 3 s
    // ... device selection as in initLedDevice() ...
    emit ledDeviceClose();                         // queued (§1.4)
    emit ledDeviceOpen();                          // queued: runs after close and after any in-flight command
}

void LedDeviceManager::ledDeviceOpenDeviceSuccess(bool ok) {
    if (m_pendingOpens == 0 || --m_pendingOpens > 0) return;   // superseded by a later open
    if (m_isSleeping) return;                                  // queued close follows
    m_openTimeoutTimer->stop();
    if (!ok) { handleDeviceFailure(); return; }
    m_linkState = LinkState::Ready;
    emit openDeviceSuccess(true);
    replayDeviceState();                                       // R5
}

void LedDeviceManager::ledDeviceCommandCompleted(bool ok) {
    if (m_isLastCommandCompleted) return;          // nothing in flight: late ack from before a reset
    m_cmdTimeoutTimer->stop();
    m_isLastCommandCompleted = true;
    if (!ok) { handleDeviceFailure(); return; }
    onCommandSucceeded();                          // R4 reset, emit ioDeviceSuccess(true), send next queued command
}

void LedDeviceManager::ledDeviceIoDeviceSuccess(bool ok) {
    if (m_linkState != LinkState::Ready) return;   // late out-of-band result from a closed link
    if (!ok) handleDeviceFailure();
}

void LedDeviceManager::setColors(const QList<QRgb> &colors) {
    if (m_backlightStatus != Backlight::StatusOn) return;
    m_savedColors = colors;
    m_isColorsSaved = true;
    if (m_linkState != LinkState::Ready) return;   // replayed after open (R5)
    if (m_isLastCommandCompleted) sendCommand(LedDeviceCommands::SetColors);
    else cmdQueueAppend(LedDeviceCommands::SetColors);
}
```

*Limitation (same with or without generations):* a command that never returns blocks the device thread, so the queued close and open never run. The main example is Windows `hid_write()` waiting forever (§1.4). The open timeout stops the manager from waiting silently: it keeps reporting failure and retrying with backoff. `m_pendingOpens` makes sure only the newest result counts if the thread ever unblocks. The real fix is finite I/O timeouts (§1.4).

**R3. One failure handler, gated by an explicit sleep state.**

```cpp
void LedDeviceManager::handleDeviceFailure() {  // command ack false, cmd/open timeout, open false, out-of-band io false
    resetCommandState();
    m_linkState = LinkState::Closed;               // stop sending until the next reopen
    emit ioDeviceSuccess(false);                   // exactly once
    if (m_isSleeping) return;                      // never reopen during suspend
    triggerRecreateLedDevice();
}

void LedDeviceManager::onSessionChange(SystemSession::Status s) {
    if (s == SystemSession::Status::Sleeping) {
        if (m_isSleeping) return;
        m_isSleeping = true;
        m_recreateTimer->stop();                   // an armed recovery must not fire into suspend
        m_openTimeoutTimer->stop();
        resetCommandState();
        m_linkState = LinkState::Closed;
        if (!Settings::isKeepLightsOnAfterSuspend())
            emit ledDeviceOffLeds();               // queued behind the in-flight command; its ack arrives with nothing in flight and is ignored
        emit ledDeviceClose();                     // queued: runs after the off frame
    } else if (m_isSleeping && (s == SystemSession::Status::Resuming
                             || s == SystemSession::Status::Unlocking
                             || s == SystemSession::Status::DisplayOn)) {
        m_isSleeping = false;                      // idempotent; also covers a missed resume event (§1.4)
        m_failedAttempts = 0;
        recreateLedDevice();
    }
}
```

- `ledDeviceCommandTimedOut()` and the open timeout both call `handleDeviceFailure()`, which fixes today's double emit.
- The manager sends the sleep-time off frame itself, ahead of the close. This removes the §1.3 / §1.4 race where `OffLeds` is queued, then dropped by a queue clear or overtaken by the cross-thread close.

**R4. Reset the backoff on a successful command, not on a successful open.**
- `triggerRecreateLedDevice()` picks the interval from `m_failedAttempts` (200 ms / 500 ms / 1 s / 5 s as today), then increments it.
- `m_failedAttempts` is reset in `onCommandSucceeded()` and on resume, **not** in `ledDeviceOpenDeviceSuccess(true)`.
- Any successful ack is a good enough signal. After every open, the first command is the R5 replay's `UpdateDeviceSettings`, which writes to the device on every device type. So a device whose writes keep failing never produces a success before failing again, and backs off to 5 s instead of looping at 200 ms. If a stricter rule is ever needed, the manager already knows which command it sent, with no extra device plumbing.
- Optional: stop `GrabManager` resuming on `openDeviceSuccess(true)`, and resume it on the first successful command after reopen instead. The R5 replay produces that command even while grabbing is suspended. This removes the remaining on/off flapping.

**R5. Keep the backlight state across failures and recovery.**
- With the §1.3 fix, `m_backlightStatus` is set when `switchOffLeds()` / `switchOnLeds()` is *requested*, so clearing the queue can no longer lose the user's choice.
- On entering `Ready`, the manager queues a fixed replay through `sendCommand()`: `UpdateDeviceSettings`, then `OffLeds` if `m_backlightStatus == StatusOff`, else `SetColors(m_savedColors)` if colours are saved. This also re-sends any frames dropped while the link wasn't `Ready`.
- This replaces the raw `emit ledDeviceUpdateDeviceSettings()` in `ledDeviceOpenDeviceSuccess()`. It also replaces the separate `updateDeviceSettings` call on resume in `LightpackApplication::onSessionChange` (`:451`), which also fixes the §1.4 ordering problem.

**R6. Re-wire the Lightpack ping.**
- Connect `restartPingDevice` to the device's own `commandCompleted`, restarting only after successful `SetColors` / settings commands. Don't restart after `OffLeds`; the explicit `stop()` in `switchOffLeds()` should win. Today `stop()` is overridden by the per-write `ioDeviceSuccess(true)`.
- Ping failure: `closeDevices()`, then `emit ioDeviceSuccess(false)`. This is one of the few remaining out-of-band error sources, along with `QSerialPort::errorOccurred` and the §1.5 TX-stall watchdog. The manager acts on it only while `Ready` (R2).
- Ping with `m_devices` empty: stop the timer. Reopening is the manager's job (§1.2b).

**R7. Narrow `ioDeviceSuccess` to out-of-band events.**
- Remove the per-write `ioDeviceSuccess(true/false)` from `writeBufferToDevice()` / `writeBuffer()`. Failures inside a command are reported through `ack(false)` alone.
- `LedDeviceManager` keeps emitting its outbound `ioDeviceSuccess(true)` on successful acks, so `GrabManager` and `SettingsWindow` see no change.

**Recommended implementation order:**
1. Prerequisites (§1.4 queued close, §1.2a–c).
2. R1 together with §1.6.
3. R2 and R3 together: the link state and the failure handler are one change.
4. R4.
5. R5 together with §1.3.
6. R6 and R7.

Each step can ship on its own, with two exceptions:
- R2/R3 need the queued close and R1 in place first. Otherwise extra or late `false` acks would start reopens.
- R2's "drop while not `Ready`" needs R5's replay, so frames and settings dropped during a reopen get re-sent.

---

### 1.2 `LedDeviceLightpack`: Permanently blocked ping timer, self-closing reopen, and dangling `hid_device*`

**Locations:**
- `Software/src/LedDeviceLightpack.cpp:285-290`
- `Software/src/LedDeviceLightpack.cpp:428-451`
- `Software/src/LedDeviceLightpack.cpp:471-532`

#### 1.2a `closeDevices()` permanently blocks `m_timerPingDevice` signals
- **Can it happen?** **Yes — 100% deterministic.** `closeDevices()` calls `m_timerPingDevice->blockSignals(true);` (line 476), and `blockSignals(false)` is **never called anywhere in the codebase**. After the very first disconnect, reopen, or sleep/wake, the 1-second Lightpack ping (`IsPingDeviceEverySecond`, enabled by default) never fires again for the lifetime of the process.
- **Impact:** Does not directly halt grabbing on its own, but permanently disables ping-based disconnect detection (relevant when `SendDataOnlyIfColorsChanges` is enabled on a static screen) and masks §1.2b.

#### 1.2b `timerPingDeviceTimeout()` closes the device right after opening it
- **Can it happen?** **Mostly masked today, but a ticking time bomb once §1.2a is fixed.**  
  In `timerPingDeviceTimeout()` (lines 498–514), if `m_devices.size() == 0`, it calls `open()` (which succeeds and emits `openDeviceSuccess(true)`), emits `openDeviceSuccess(true)` a second time, and then immediately executes `closeDevices(); // device should be opened by open() function`!  
  - *Why it rarely triggers today:* When `open()` fails at startup, `m_recreateTimer` fires its first retry at **500ms**, calling `close()` → `closeDevices()` → `blockSignals(true)` *before* `m_timerPingDevice`'s **1000ms** interval fires. It can only trigger today if `m_ledDeviceManagerThread` or `open()` stalls >500ms on the first attempt.
  - *Why it's critical:* As soon as §1.2a is fixed, any Lightpack plugged in while `m_recreateTimer` is in its 5-second backoff will be caught by the 1-second ping timer, opened, announced as `openDeviceSuccess(true)` (resuming `GrabManager`), and **immediately closed** — hitting §1.1 on the next frame!

#### 1.2c Use-after-free on `phid_device` after `tryToReopenDevice()` (plus false `openDeviceSuccess(true)`)
- **Can it happen?** **Yes — 100% deterministic on any transient USB write error while a Lightpack is connected.**  
  In `writeBufferToDeviceWithCheck(int command, hid_device *phid_device)` (lines 428–451):
  1. `writeBufferToDevice` fails twice (lines 434, 436).
  2. `tryToReopenDevice()` calls `closeDevices()` → **`hid_close(phid_device)`** (closing the OS handle and `free()`ing the `hid_device` struct) → calls `open()` (allocating new `hid_device*` handles in `m_devices`).
  3. Line 439 then calls `writeBufferToDevice(command, phid_device)` using the **freed, dangling `phid_device` pointer** (and line 447 passes `NULL`). This either crashes the application (permanent halt) or fails the write immediately after a successful reopen.
  4. In addition, `open()` calls `updateDeviceSettings()` (line 287) before emitting `openDeviceSuccess(true)` (line 289). If a transient write error inside `updateDeviceSettings()` triggers `tryToReopenDevice()` and clears `m_devices`, the outer `open()` still unconditionally emits `openDeviceSuccess(true)` with `m_devices` empty (though the earlier `ioDeviceSuccess(false)` did arm `m_recreateTimer`, so this causes flapping rather than a permanent halt).

**Recommended Fix:**
- Remove `tryToReopenDevice()` and reopen-from-`timerPingDeviceTimeout()` from `LedDeviceLightpack`; let `LedDeviceManager` be the single owner of device reopen/retry.
- Remove `m_timerPingDevice->blockSignals(true)` in `closeDevices()` (just `stop()` it on `m_ledDeviceThread`).
- Don't call `updateDeviceSettings()` inside `open()` (since `LedDeviceManager::ledDeviceOpenDeviceSuccess(true)` already triggers it), or check `!m_devices.isEmpty()` before emitting `openDeviceSuccess(true)`.

---

### 1.3 `LedDeviceManager::switchOffLeds()` / `switchOnLeds()` queue race latches `StatusOff`

**Location:** `Software/src/LedDeviceManager.cpp:119-167`

**Can it happen?** **Yes — realistic in normal use.**  
During active grabbing (30–60 FPS), `m_isLastCommandCompleted` is frequently `false` while `setColors()` executes on `m_ledDeviceThread`.
1. `switchOffLeds()` arrives while `m_isLastCommandCompleted == false` → appends `LedDeviceCommands::OffLeds` to `m_cmdQueue`; `m_backlightStatus` remains `StatusOn`.
2. Before the in-flight command finishes (e.g., user nudges mouse right as display turns off, rapid lock/unlock, tray double-click, or API `off` → `on`), `switchOnLeds()` arrives.
3. `switchOnLeds()` sets `m_backlightStatus = StatusOn` and emits `ledDeviceSetColors(m_savedColors)` directly — **without removing `OffLeds` from `m_cmdQueue`**.
4. Milliseconds later, the in-flight command completes → `ledDeviceCommandCompleted(true)` pops `OffLeds` → `processOffLeds()` sets **`m_backlightStatus = Backlight::StatusOff`** and turns off LEDs.
5. `LedDeviceManager::setColors()` now drops every frame forever (`if (m_backlightStatus == Backlight::StatusOn)` is false), even though `GrabManager` is running and the UI shows Backlight ON.
- *Secondary issue:* `switchOnLeds()` bypasses `m_isLastCommandCompleted` and `m_cmdTimeoutTimer`, so its extra `commandCompleted` reply can prematurely advance `m_cmdQueue` or stop the timeout timer for another in-flight command.

**Recommended Fix:**
- In `switchOnLeds()`: call `m_cmdQueue.removeAll(LedDeviceCommands::OffLeds);` and send `m_savedColors` via `setColors(m_savedColors)` (going through the command queue).
- In `switchOffLeds()`: set `m_backlightStatus = Backlight::StatusOff` immediately and remove any queued `LedDeviceCommands::SetColors`.

---

### 1.4 Cross-thread `m_ledDevice->close()`, missed Windows `PBT_APMRESUMEAUTOMATIC`, and sleep/wake races

**Locations:**
- `Software/src/LedDeviceManager.cpp:107-117, 438-462, 706-714`
- `Software/src/LightpackApplication.cpp:233-235, 394-475`
- `Software/src/WindowsSession.cpp:70-80`
- `Software/src/hidapi/windows/hid.c:593`

**Can it happen?**
1. **Cross-thread `m_ledDevice->close()` — YES, happens on EVERY `recreateLedDevice()` and EVERY `Sleeping` event.**  
   `LedDeviceManager` lives on `m_ledDeviceManagerThread`, while `m_ledDevice` (and its `QSerialPort`, `QUdpSocket`, `QTimer` members) lives on `m_ledDeviceThread`. Calling `m_ledDevice->close()` directly from `m_ledDeviceManagerThread`:
   - Can `delete m_AdalightDevice` / `delete m_Socket` / `hid_close()` **concurrently** while `m_ledDeviceThread` is executing `setColors()` or blocked in Windows `hid_write()` (`GetOverlappedResult(..., TRUE)` with no timeout!) — causing heap UAF / crash.
   - Fails Qt's thread-affinity check on `m_lastWillTimer->stop()` / `m_timerPingDevice->stop()` ("Timers cannot be stopped from another thread").
2. **Missed Windows resume (`PBT_APMRESUMEAUTOMATIC`) — YES, common on Modern Standby (S0ix) and wake-to-lock-screen.**  
   `WindowsEventFilter` handles `PBT_APMSUSPEND` (closing the device and turning off LEDs) and *only* `PBT_APMRESUMESUSPEND`. Windows always delivers `PBT_APMRESUMEAUTOMATIC` on wake, but may omit `PBT_APMRESUMESUSPEND` when the wake wasn't classified as user-initiated. If `PBT_APMRESUMESUSPEND` isn't sent, `recreateLedDevice()` and `switchOnLeds()` never run, leaving the device closed and (via §1.1) grabbing permanently halted.
3. **Sleep/Resume signal ordering & unstopped `m_cmdTimeoutTimer` — YES, deterministic on every sleep/wake.**  
   Because `LightpackApplication::onSessionChange` is connected before `LedDeviceManager::onSessionChange`:
   - On `Resuming`, `switchOnLeds` and `updateDeviceSettings` are queued onto `m_ledDeviceManagerThread` *before* `recreateLedDevice()`, executing against the still-closed device.
   - `recreateLedDevice()` never stops `m_cmdTimeoutTimer` or clears `m_cmdQueue`, so a pending 500ms timeout can fire *after* `open()` succeeds.
4. **`--nogui` crash on session change — YES, deterministic in `--nogui` mode.**  
   `LightpackApplication::onSessionChange` calls `getLightpackApp()->settingsWnd()->switchOffLeds()` / `switchOnLeds()` without checking `settingsWnd() != nullptr`.

**Recommended Fix:**
- Always invoke `m_ledDevice->close()` via `Qt::QueuedConnection` on `m_ledDeviceThread` (ordered ahead of queued `open()`).
- Stop `m_cmdTimeoutTimer` and clear `m_cmdQueue` inside `recreateLedDevice()`.
- Handle `PBT_APMRESUMEAUTOMATIC` in `WindowsSession.cpp` (deduplicating with `PBT_APMRESUMESUSPEND` via `m_isSuspending` / idempotent resume).
- Invoke `LedDeviceManager` slots directly in `LightpackApplication::onSessionChange` instead of going through `settingsWnd()`, and sequence resume so `recreateLedDevice()` runs before `switchOnLeds()` / `updateDeviceSettings()`.

---

### 1.5 Serial devices (`Adalight` / `Ardulight`) silently halt forever when TX buffer stalls

**Locations:**
- `Software/src/LedDeviceAdalight.cpp:225-299`
- `Software/src/LedDeviceArdulight.cpp:231-306`

**Can it happen?** **Yes — very common with USB-serial adapters (Arduino, CH340, CP2102, FTDI).**
1. `QSerialPort::write()` is asynchronous: it copies data into Qt's write buffer and immediately returns `buff.count()`.
2. Neither `LedDeviceAdalight` nor `LedDeviceArdulight` connects `QSerialPort::errorOccurred`.
3. If the USB-serial device disconnects, resets, or stalls while bytes remain unsent in Qt's TX buffer, `isOpen()` often remains `true` while `bytesToWrite() > 0` persists indefinitely.
4. Every subsequent `writeBuffer()` hits `if (bytesToWrite() > 0)`, skips writing, restarts `m_lastWillTimer` (100ms), and **returns `true`** → emits `commandCompleted(true)`.
5. `writeLastWill(false)` checks `if (force || bytesToWrite() == 0)` and does nothing.
- **Result:** No error is ever reported, `recreateLedDevice()` is never called, and LED output halts permanently even after the adapter is reconnected.
- *Secondary issue:* In `open()`, if `QSerialPort::open()` succeeds but `setBaudRate()` or `setDataBits()` fails, `openDeviceSuccess(false)` is emitted while leaving the serial port open.

**Recommended Fix:**
- Connect `QSerialPort::errorOccurred`: on fatal errors (`ResourceError`, `WriteError`, `DeviceNotFoundError`, `PermissionError`), close the port and emit `ioDeviceSuccess(false)`.
- Track how long `bytesToWrite() > 0` has remained non-zero (e.g. >1–2 seconds without `bytesWritten` progress); if stalled, close the port and emit `ioDeviceSuccess(false)`.
- Call `close()` inside `open()` if `setBaudRate()` or `setDataBits()` fails.

---

### 1.6 Settings changes while backlight is off always time out (500ms)

**Locations:**
- `Software/src/AbstractLedDevice.cpp:37-94`
- `Software/src/LedDeviceManager.cpp:229-318`

**Can it happen?** **Yes — 100% deterministic.**  
When `m_backlightStatus == StatusOff`, `LedDeviceManager::setGamma` / `setBrightness` / `setBrightnessCap` / `setLuminosityThreshold` / `setMinimumLuminosityEnabled` / `setDitheringEnabled` start `m_cmdTimeoutTimer` (500ms) and pass `updateColors = false`. `AbstractLedDevice` updates the member variable, skips `setColors()`, and **never emits `commandCompleted(true)`**.
- **Does it permanently halt?** **No** — `ledDeviceCommandTimedOut()` triggers `recreateLedDevice()`, which recovers after 200ms. However, it causes a spurious `StatusDeviceError` and an unnecessary device close/reopen (which can reset Arduino boards via DTR).

**Recommended Fix:** Add `else emit commandCompleted(true);` (or `ack(true)` under §1.1 R1) in those `AbstractLedDevice` setters when `!updateColors`.

---

### 1.7 `LedDeviceAlienFx` cannot recover from transient startup failure

**Locations:**
- `Software/src/LedDeviceAlienFx.cpp:60-96, 176-180`
- `Software/src/LedDeviceManager.cpp:446-461`

**Can it happen?** **Yes (on AlienFX systems if the AlienFX service isn't ready yet at login).**  
`LoadLibrary` and `lfxInitFunction()` execute only in the constructor (and `emit ioDeviceSuccess(false)` in the constructor fires before signals are connected). `close()` is a no-op, `open()` unconditionally emits `openDeviceSuccess(true)`, `setColors()` emits `commandCompleted(true)` even when `!m_isInitialized`, and `LedDeviceManager` reuses the cached object in `m_ledDevices[DeviceTypeAlienFx]`. If `lfxInitFunction()` fails once at startup, `m_isInitialized` stays `false` forever.

**Recommended Fix:** Move `LoadLibrary`/`lfxInitFunction()` into `open()`, `lfxReleaseFunction()` into `close()`, emit `openDeviceSuccess(m_isInitialized)`, and check `LFX_RESULT` in `setColors()`.

---

### 1.8 Uninitialized member variables

**Locations:**
- `Software/src/GrabManager.hpp:142` (`m_isGrabbingSuspendedDueToDeviceError`)
- `Software/src/LedDeviceManager.hpp:136` (`m_ledDevice`)

**Can it cause a permanent halt?** **Mostly masked at startup, but still C++ UB.**  
`LightpackApplication::initializeAll()` calls `settingsChanged()` → `m_grabManager->start(isCanStart)` synchronously before the event loop runs, which overwrites `m_isGrabbingSuspendedDueToDeviceError` (though `start(false)` reads the uninitialized bool at line 160 first). `m_ledDevice` is initialized when queued `init()` runs first on `m_ledDeviceManagerThread`, but would be dangling if `onSessionChange` or destruction occurred first.

**Recommended Fix:** Default-initialize both in their headers (`= false;`, `= nullptr;`).

---

## Part 2: Screen Grabbers & `GrabManager` Logic

### 2.1 `DDuplGrabber` (Windows Desktop Duplication) — Multiple permanent-halt traps

**Location:** `Software/grab/DDuplGrabber.cpp`

#### 2.1a Transient `DXGI_ERROR_UNSUPPORTED` or `init()` failure permanently latches `m_state = Unavailable`
- **Lines:** `147-182`, `310-313`, `436-441`, `562-589`
- **Can it happen?** **Yes — very common on Windows.**  
  Microsoft's DXGI documentation notes that `IDXGIOutput1::DuplicateOutput` can return `DXGI_ERROR_UNSUPPORTED` **transiently** during Remote Desktop (RDP) sessions, fullscreen exclusive / HDR mode transitions, DWM restarts, display power-off, and hybrid GPU (Optimus) switches. Setting `m_state = Unavailable` makes `isReallocationNeeded()` return `false` forever and `grabScreens()` return `GrabResultFrameNotReady` forever. Stopping and restarting grabbing (`startGrabbing()`) does not reset `m_state`.  
  Likewise, `init()` sets `m_state = Unavailable` on line 149 before calling `recreateAdapters()`; a transient `CreateDXGIFactory1` failure during early boot or GPU reset locks `m_state = Unavailable` permanently.
- **Recommended Fix:** Only treat missing `dxgi.dll`/`d3d11.dll` entry points as permanently `Unavailable`. Treat `DXGI_ERROR_UNSUPPORTED` and `recreateAdapters()` failures as retryable with backoff (e.g., every 5s, reset on `DisplayOn`/`Resuming`/`Unlocking`/`changeScreen`/`startGrabbing`).

#### 2.1b `AcquireNextFrame` / D3D11 errors (`DEVICE_REMOVED`, `DEVICE_RESET`, `SESSION_DISCONNECTED`, etc.) never trigger reallocation
- **Lines:** `613-625`, `631-761`
- **Can it happen?** **Yes — deterministic on GPU TDR (driver crash/reset), GPU driver updates, eGPU reconnect, or RDP/Fast User Switch.**  
  Only `DXGI_ERROR_ACCESS_LOST` and `DXGI_ERROR_INVALID_CALL` transition `m_state = LostAccess`. Any other `FAILED(hr)` from `AcquireNextFrame` (notably `DXGI_ERROR_DEVICE_REMOVED`, `DXGI_ERROR_DEVICE_RESET`, `DXGI_ERROR_SESSION_DISCONNECTED`) or from `CreateTexture2D`, `CreateShaderResourceView`, `QueryInterface`, `surface->Map`, or `catch (_com_error)` leaves `m_state == Allocated` (and leaks `ReleaseFrame()`). `isReallocationNeeded()` sees unchanged monitor rects and returns `false` forever.
- **Recommended Fix:** On any unexpected `FAILED(hr)` or `_com_error`, release the frame if acquired, set `m_state = LostAccess`, and mark adapters dirty so the next reallocation recreates the DXGI factory/adapters and D3D11 device.

#### 2.1c Stale `IDXGIFactory1` / `m_adapters` never refreshed on hotplug, monitor wake, or GPU reset
- **Lines:** `184-282`, `397-488`
- **Can it happen?** **Yes — very common with DisplayPort/USB-C monitors waking from sleep, docks, KVM switches, and GPU TDR.**  
  `recreateAdapters()` is only called when `outputDesc.Monitor == NULL` (line 256) or `hr == DXGI_ERROR_MODE_CHANGE_IN_PROGRESS` (line 442), and `GrabManager::changeScreen()` is never connected to `DDuplGrabber`. When DXGI topology changes, the cached `IDXGIFactory1` becomes stale (`IsCurrent() == FALSE`): `EnumOutputs` returns 0 outputs or old `HMONITOR` handles that no longer match `MonitorFromWindow()`, causing `screensWithWidgets()` to return empty (`"No screens with widgets found"`) forever — or `_reallocate()` to repeatedly fail `D3D11CreateDevice`/`DuplicateOutput` on stale adapters.
- **Recommended Fix:** Retain `IDXGIFactory1Ptr m_factory` and call `recreateAdapters()` whenever `!m_factory->IsCurrent()`, whenever `screensWithWidgets()` returns empty, whenever `_reallocate()` fails, and on `GrabManager::changeScreen()` / `DisplayOn` / `Resuming`.

#### 2.1d Multi-monitor starvation & NULL-deref crash when Monitor 0 is static
- **Lines:** `604-630` (and `Software/grab/GrabberBase.cpp:233`)
- **Can it happen?** **Yes — on multi-monitor setups with widgets on 2+ screens.**
  - If Monitor 0 has never presented a frame since reallocation (`imgData == NULL`), `DXGI_ERROR_WAIT_TIMEOUT` returns `GrabResultFrameNotReady` immediately, starving Monitor 1 indefinitely until Monitor 0 repaints.
  - Conversely, if Monitor 0 receives a cursor-only update (`LastPresentTime == 0`), it hits `continue` with `imgData == NULL`; if Monitor 1 updates, `grabScreens()` returns `GrabResultOk`, and `GrabberBase::grab()` crashes on `Q_ASSERT(grabbedScreen->imgData)` / NULL dereference in `calculateAvgColor()`.
- **Recommended Fix:** In `DDuplGrabber::grabScreens()`, `continue` instead of returning early on `DXGI_ERROR_WAIT_TIMEOUT`, and in `GrabberBase::grab()` safely skip widgets whose `grabbedScreen->imgData == nullptr`.

#### 2.1e `runThreadCommand(INFINITE)` hang & `returnBlackBuffer()` 4× buffer under-allocation
- **Lines:** `350-361`, `525-558`
- **Can it happen?**
  - `runThreadCommand(INFINITE)` blocks the GUI thread indefinitely if D3D11/DXGI hangs in the worker thread.
  - `returnBlackBuffer()` computes `screen.bytesPerRow = width >> DownscaleMipLevel` (missing `* 4` for 4 bytes/pixel). On single-monitor setups this is masked because `freeScreens()` cleared `_screensWithWidgets` beforehand, so the loop runs 0 times — **but on multi-monitor setups where Monitor 0 succeeded `DuplicateOutput` and Monitor 1 returned `E_ACCESSDENIED`**, `_screensWithWidgets` contains Monitor 0 and `calculateAvgColor()` reads 4× past the allocated heap buffer (risking a crash).
- **Recommended Fix:** Fix `bytesPerRow = (width >> DownscaleMipLevel) * 4`, call `freeScreens()` before entering `AccessDeniedDuplication`, and use a finite timeout in `runThreadCommand()`.

---

### 2.2 `WinAPIGrabber` (Windows GDI) — Unchecked GDI failures & handle leak

**Location:** `Software/grab/WinAPIGrabber.cpp:58-184`

**Can it happen?** **Yes (for users using WinAPI grabber).**
1. **Silent permanent halt:** `reallocate()` never checks `CreateDC`, `CreateCompatibleBitmap`, `CreateCompatibleDC`, `GetObject`, or `malloc` for failure and unconditionally returns `true`. If called during a locked desktop, UAC prompt, display sleep, or mode transition, it caches `NULL` handles and a 0-byte buffer. `isReallocationNeeded()` then returns `false` forever because rects match, and `grabScreens()` ignores `BitBlt()`/`GetBitmapBits()` failures — returning `GrabResultOk` with black/stale frames forever. Even if `reallocate()` succeeded initially, an invalidated `hScreenDC` after display sleep/reset fails `BitBlt()` silently forever.
2. **GDI handle leak:** `freeScreens()` calls `DeleteObject()` on `HDC`s (`hScreenDC`, `hMemDC`) instead of `DeleteDC()`, and deletes `hBitmap` while still selected into `hMemDC`. Every reallocation leaks 3 GDI handles; hitting Windows' per-process 10,000 GDI object quota causes all future allocations to fail permanently.

**Recommended Fix:** Restore the original bitmap before `DeleteObject(hBitmap)`, use `DeleteDC()` for both `HDC`s, validate all GDI calls in `reallocate()` (freeing and returning `false` on error), and check `BitBlt()`/`GetBitmapBits()` in `grabScreens()` (clearing `_screensWithWidgets` and returning `GrabResultError` on failure so the next tick reallocates).

---

### 2.3 `MacOSAVGrabber` (macOS AVFoundation) — Permanent halt on runtime error, interruption, or display change

**Location:** `Software/grab/MacOSAVGrabber.mm:160-231, 384-453`

**Can it happen?** **Yes — very common on macOS (especially laptops with external monitors and sleep/wake).**
1. **`reallocate()` does not update `_captures`:** `_captures` (`QMap<CGDirectDisplayID, MacOSNativeAVCapture*>`) is only built in `startGrabbing()`. `MacOSAVGrabber` inherits `MacOSGrabberBase::reallocate()` without overriding it. If a display wakes/reconnects with a new `CGDirectDisplayID`, or if `startGrabbing()` ran while a display was asleep or `createCaptureSession` transiently failed (also leaking `capture` on failure at line 406), `grabDisplay()` hits `_captures.value(display, nullptr) == nullptr` and returns `GrabResultError` forever.
2. **Non-recoverable runtime errors destroy the session permanently:** Any `AVCaptureSessionRuntimeErrorNotification` other than `AVErrorDisplayWasDisabled` or `AVErrorSessionNotRunning` (e.g., `AVErrorScreenCaptureFailed`, `AVErrorSessionConfigurationChanged`, `AVErrorUnknown`) destroys `_captureSession` and latches `self.captureError` forever — every future `grabDisplay()` returns `GrabResultError` with no retry.
3. **Interruptions and recoverable errors aren't retried:** `AVCaptureSessionInterruptionEndedNotification` only logs and returns without ensuring the session restarts; `handleRecoverableCaptureSessionRuntimeError:` attempts `startRunning` once immediately without retrying if the display isn't ready yet.

**Recommended Fix:** Override `reallocate()` in `MacOSAVGrabber` to recreate `_captures` for the active displays; on runtime errors, interruption end, or missing capture, trigger a backed-off recreation of the affected `MacOSNativeAVCapture` rather than latching `captureError` permanently.

---

### 2.4 `X11Grabber` (Linux X11) — No display reopen or XShm/BadMatch handling

**Location:** `Software/grab/X11Grabber.cpp:51-163`

**Can it happen?** **Medium likelihood on Linux X11.**
- `_display = XOpenDisplay(NULL)` runs only in the constructor with no null check or reopen (crashes if X isn't ready at launch).
- `reallocate()` logs `shmget` failure (e.g., if SysV SHM limits are transiently exceeded on multi-monitor/4K setups) and continues anyway (`shmat(-1)` returns `(void*)-1`, leading to a segfault).
- `XShmGetImage()` return value is unchecked, and there is no X error handler — an XRandR resolution shrink or monitor unplug between `screensWithWidgets()` and `XShmGetImage()` raises `BadMatch`, causing Xlib's default error handler to terminate Prismatik.

**Recommended Fix:** Lazily open/reopen `_display`; validate `XShmCreateImage`/`shmget`/`shmat`/`XShmAttach` and return `false` on failure; install a scoped X error handler around `XShmGetImage` and force reallocation on `BadMatch` / `False`.

---

### 2.5 `GrabManager` drag-pause latch & `D3D10Grabber` handoff/mutex traps

**Locations:**
- `Software/src/GrabManager.cpp:153-177, 233-253, 419-422, 697-712`
- `Software/src/GrabWidget.cpp:249-301, 575-587`
- `Software/grab/D3D10Grabber.cpp:397-437`

**Can they happen?**
1. **Stuck `m_isPauseGrabWhileResizeOrMoving` — YES.** Set on `GrabWidget::mousePressEvent`, cleared only on `mouseReleaseEvent` (or `settingsProfileChanged`). If the user Alt+Tabs, locks the screen, gets a UAC prompt, or hides/removes widgets mid-drag, `mouseReleaseEvent` is never delivered; `handleGrabbedColors()` drops every frame forever (even `GrabManager::start(false)`/`start(true)` does not clear it).
2. **`D3D10Grabber` `WAIT_ABANDONED` / `INFINITE` mutex wait — YES (if DX hooking is enabled).** `start()`, `stop()`, and `setGrabInterval()` call `WaitForSingleObject(m_mutex, INFINITE)` on the GUI thread. A hung hooked game freezes the GUI thread permanently; a crashed hooked game causes `WAIT_ABANDONED`, which grants mutex ownership to Prismatik's GUI thread, but the `if (WAIT_OBJECT_0 == ...)` check skips `ReleaseMutex(m_mutex)`, permanently deadlocking future hooked processes.
3. **`D3D10Grabber` handoff desync — YES (if DX hooking is enabled).** `reinitDx1011Grabber()` deletes an active `m_d3d10Grabber` while `m_grabber` is stopped, leaving **both** stopped permanently. `GrabManager::start(false)` fails to stop `m_d3d10Grabber`, and `onGrabberStateChangeRequested(false)` restarts `m_grabber` even when `m_isGrabbingStarted == false`.

**Recommended Fix:**
- Reset `m_isPauseGrabWhileResizeOrMoving = false` in `GrabManager::start()` and `setVisibleLedWidgets(false)`, and emit `resizeOrMoveCompleted` in `GrabWidget::hideEvent()` / destructor when `cmd != NOP`.
- Use a finite timeout on `WaitForSingleObject(m_mutex, ...)` and handle `WAIT_ABANDONED` by releasing `m_mutex`.
- Synchronize `m_d3d10Grabber` with `m_grabber` in `start(false)`, `reinitDx1011Grabber()`, and `onGrabberStateChangeRequested(false)`.

---

## Recommended Remediation Priority

1. **P0 (Highest real-world impact):**
   - `DDuplGrabber`: make `DXGI_ERROR_UNSUPPORTED`/init retryable, trigger `LostAccess` + adapter refresh on all DXGI/D3D11 errors, check `IDXGIFactory1::IsCurrent()` / empty screens, fix multi-monitor static-screen starvation & `returnBlackBuffer()` size.
   - `LedDeviceManager` + `WindowsSession` + `LightpackApplication`: fix cross-thread `close()`, handle `PBT_APMRESUMEAUTOMATIC`, fix `switchOffLeds()`/`switchOnLeds()` queue race, fix `--nogui` null deref.
   - `LedDeviceManager` / `AbstractLedDevice` signal contract (§1.1 revised fix R1–R7): structural 1-ack-per-command, manager-side link state (`Closed`/`Opening`/`Ready`) to discard late results, single sleep-gated failure handler, backoff reset on successful command, post-open replay preserving backlight intent, ping rewiring.
   - `LedDeviceLightpack`: remove `blockSignals(true)`, remove `tryToReopenDevice()` UAF and `timerPingDeviceTimeout()` self-close.
2. **P1 (High impact on specific hardware/platforms):**
   - `LedDeviceAdalight` / `LedDeviceArdulight`: connect `QSerialPort::errorOccurred` and add TX-stall watchdog.
   - `MacOSAVGrabber`: override `reallocate()` to manage `_captures` and recover from runtime errors/interruptions.
   - `WinAPIGrabber`: fix `DeleteDC()` leak and validate all GDI calls in `reallocate()`/`grabScreens()`.
3. **P2 (Defense-in-depth / legacy paths):**
   - `GrabManager` / `GrabWidget`: clear drag-pause latch safely; fix `D3D10Grabber` `WAIT_ABANDONED` and handoff desync.
   - `AbstractLedDevice`: emit `commandCompleted(true)` when `!updateColors`.
   - `X11Grabber`, `LedDeviceAlienFx`, and uninitialized member variables.
