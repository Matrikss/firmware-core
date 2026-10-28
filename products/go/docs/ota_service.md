# OTA Service

`OtaService` is the product-side glue around the `airgradient-ota` component.
It owns the per-mode OTA wiring — **BLE push** while Portable, **WiFi pull**
while Stationary, nothing while Offline — and the borrowed-server BLE
registration. It never spawns a task or reboots. Cloud calls the Wi-Fi
availability probe; the orchestrator runs transfers and decides reboot or resume
from `OtaStatus`. Both transfers use the same owned `EspOtaImageWriter`.

## Files

| File | Purpose |
|---|---|
| `products/go/main/go_ota.h` | `OtaService` declaration |
| `products/go/main/go_ota.cpp` | Implementation |

## Dependencies

| Dependency | Source | Usage |
|---|---|---|
| `OtaBleService` | `airgradient-ota` (`services/ota_ble_service.h`) | BLE push transport over the borrowed server (owned by value) |
| `OtaUpdater` | `airgradient-ota` (`services/ota_updater.h`) | WiFi pull orchestrator (constructed per check) |
| `WifiHttpOtaSource` | `airgradient-ota` (`backends/wifi/wifi_http_ota_source.h`) | HTTP GET image source (constructed per check) |
| `EspOtaImageWriter` | `airgradient-ota` (`backends/esp/esp_ota_image_writer.h`) | Universal flash-write core (owned by value) |
| `OtaStatus`, `OtaProgress`, `OtaRequest` | `airgradient-ota` (`types/ota_types.h`) | Result, progress, and request types |
| `AgBleServer` | `airgradient-ble` (`hal/ble_server.h`) | Borrowed GATT server, shared with `BleService` |
| `PowerService` | product (`go_power.h`) | External GPIO2 watchdog feeder |

## Public API

| Method | Returns | Purpose |
|---|---|---|
| `setup_ble()` | `bool` | Register the OTA GATT service on the borrowed server. Must run between the server's register phase and `start_advertising()` (Portable only). |
| `teardown_ble()` | `void` | Abort any in-flight transfer and clear the OTA registration. Idempotent; never `deinit()`s the server. |
| `handle_disconnect()` | `void` | Forward a central disconnect so an in-flight `run_ble()` aborts. Runs synchronously in the NimBLE host-task callback. |
| `is_ble_active()` | `bool` | Start-edge probe: `true` from the moment a valid `START` latches (before `begin()`) until the terminal. |
| `run_ble()` | `OtaStatus` | Drive a phone-initiated BLE transfer to its terminal (wraps `run(0)`). Blocks on the caller (orchestrator) task. |
| `check_wifi_update() const` | `OtaStatus` | Zero-length Wi-Fi probe on the cloud task; `Ok` means an image is available. Does not touch flash, display, or watchdog. |
| `run_wifi_check(on_download_started)` | `OtaStatus` | Run one device-initiated WiFi availability check + download. Blocks; returns `UpToDate` when no update. `on_download_started` fires once, on the first `Downloading` tick. |

See [`go_ota.h`](../main/go_ota.h) for full signatures.

## Behavior

### Mode-to-Transport Mapping

| Operating Mode | OTA Path | Drive Model | Component Pieces |
|---|---|---|---|
| Portable | BLE push | Phone-initiated | `OtaBleService` on the borrowed `AgBleServer` + `EspOtaImageWriter` |
| Stationary | WiFi pull | Device-initiated | `WifiHttpOtaSource` + `OtaUpdater` + `EspOtaImageWriter` |
| Offline | None | — | — |

### Foreground, Exclusive Execution

There is **no dedicated OTA task**. `OtaBleService::run()` and `OtaUpdater::run()`
are single blocking calls that the orchestrator runs on its own task. BLE OTA
pauses sensitive services before the call. In Stationary, cloud runs the
availability probe sequentially with POST/FETCH, then disarms itself and sends
`WifiOtaAvailable` if an image is offered. The orchestrator rechecks eligibility
before calling `run_wifi_check()`; sensing, GPS, and PM pause only on its first
`Downloading` tick.
Between `enter_ota()` and the transfer's terminal the orchestrator main loop
never iterates, so mode switches and shutdown are implicitly deferred. The
orchestrator owns the trigger, quiesce, paint, and reboot decision — see
[`orchestrator.md` → OTA](orchestrator.md#firmware-update-ota).

```mermaid
sequenceDiagram
    autonumber
    participant Orch as Orchestrator task
    participant Ota as OtaService
    participant W as EspOtaImageWriter
    Orch->>Ota: run_ble / run_wifi_check (blocks)
    Ota->>W: begin -> write... -> finish
    Note over Ota,Orch: every non-terminal progress tick pets the ext watchdog
    Ota-->>Orch: OtaStatus (Ok / UpToDate / error)
    Orch->>Orch: finish_ota -- reboot on Ok, else resume
```

### Watchdog Feeding

The external GPIO2 watchdog is the only one to manage during a transfer (the BMS
watchdog is disabled at BMS init). `OtaService` feeds it from the component's
`set_on_progress()` callback on every **non-terminal** tick (`Starting` /
`Checking` / `Downloading` / `Applying`) via the borrowed `PowerService`. Logic
lives in the named handlers `_on_ble_progress()` / `_on_wifi_progress()`, wired
through thin forwarders. The pre-transfer start-gap feed is the orchestrator's
concern (`enter_ota()` for BLE; `on_wifi_ota_available()` for Wi-Fi).

### Lazy WiFi Commit

The cloud probe uses `&offset=0&length=0` and closes before the separate image
GET. The server can decline that later GET, so `run_wifi_check()` still defers
quiescing until download starts. `_on_wifi_progress()` invokes the stored `on_download_started`
exactly once, on the first `OtaState::Downloading` tick (one-shot guarded by
`_wifi_download_painted`) — i.e. only when an image is really being pulled. The
orchestrator uses that callback to commit the full quiesce + "Updating firmware…"
paint. `UpToDate` / `Declined` never reach `Downloading`, so a no-op check never
stops sensing or touches the e-paper.

### Local HTTP During Stationary OTA

The availability probe and subsequent streaming GET use separate outgoing
`esp_http_client` requests, not the inbound `HttpServer`. Before download
commits, local API access and queued requests are unchanged.

On the first `Downloading` tick, the orchestrator stores the current local API
access, changes `ReadWrite` to `ReadOnly`, and clears queued local mutations. An
already-active listener, local routes, and mDNS stay active while STA remains
connected; a disabled endpoint stays disabled. Measures and config GETs remain
safe because `GoLocalApiService` serves mutex-protected cached snapshots; config
PUTs and actions reach existing routes but are rejected by the runtime access
gate. This read-only carve-out preserves local observability while the
orchestrator is blocked and sensors are quiesced.

A non-rebooting committed terminal restores the exact pre-OTA access. A
speculative `UpToDate`, `Declined`, or failure never changes local access or
clears its FIFO. Successful OTA reboots instead of restoring services.

### Reboot and Image Validity

`OtaService` never reboots. On `OtaStatus::Ok` the orchestrator reboots via the
shared `reboot()` free function. `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` is off,
so a flashed image boots directly with no mark-valid step or automatic revert.

## Edge Cases / Errors

| Case | Behavior |
|---|---|
| Central disconnect mid-BLE-transfer | `handle_disconnect()` latches `TransportError` synchronously on the host task; `run_ble()` unblocks and returns it → "Update failed". Not `Aborted`. |
| Explicit phone `ABORT` | `run_ble()` returns `Aborted` → "Update cancelled". |
| Disconnect within the `START → poll` window (before `begin()`) | Accepted stillborn-abort race: one wasted quiesce + partition erase, then a clean `TransportError`. |
| Stranded-active latch (auth cleared but `START` still latched) | The orchestrator gate keeps polling on `is_ble_active()`; the next `run_ble()` services the pending abort and clears the latch. |
| OTA `START` during an active Go BLE history export | Serialized, not concurrent — both run on the single orchestrator task; no image bytes flash until the export finishes. |
| `setup_ble()` failure | Non-fatal: the orchestrator logs, calls `teardown_ble()`, and keeps advertising Go data + provisioning without OTA. |
| WiFi drop mid-download | The source read fails and `run()` returns `TransportError`; the device stays Stationary-but-offline. The committed-exit queue drain can discard the queued disconnect event, so automatic reconnect is not guaranteed. |
| Local config/action during committed WiFi OTA | If the endpoint was active and remains reachable, existing routes return forbidden while cached GETs remain available; queued pre-commit mutations are discarded. |

## Configuration

The product selects `CONFIG_AG_DEVICE_MODEL_GO`, so WiFi pull uses the Go
firmware endpoint. The component's `CONFIG_AG_OTA_*` Kconfig knobs are reused
as-is (timeouts, buffer sizes, BLE Data/Control limits — see
[`airgradient-ota`](../../../components/airgradient-ota/README.md)). The product
sets `CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU = 512` and provisions the NimBLE mbuf
pool in `sdkconfig.defaults` for the larger MTU (at MTU 512 the BLE Data
Write-Command payload is `MTU − 3 = 509` bytes). Wi-Fi check timing lives in
[`CloudService::Config`](cloud_service.md#config); the orchestrator retains the
2 s BLE poll (`OTA_BLE_POLL_INTERVAL_MS`).

## Testability

Cloud scheduling and event delivery are tested in
[`go_cloud.tests.cpp`](../tests/go_cloud.tests.cpp). The **trigger, quiesce, and
outcome handling in the orchestrator** are exercised against a stubbed
`OtaService`, at the same boundary as `WifiService` / `BleService`. `run_ble()` / `run_wifi_check()`
are thin shells over NimBLE / ESP-IDF (compiled out under `TEST_HOST`) and are
HIL-verified. The by-value `EspOtaImageWriter` + `OtaBleService` members
construct from their host shells, which the orchestrator/app test targets link.
The watchdog-feed cadence is HIL-verified, not host-asserted.
