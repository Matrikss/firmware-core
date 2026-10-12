# VHUB Verification Templates

VHUB is a self-hosted workspace for manually validating AirGradient products.
Import a released JSON procedure, start a validation, and record one status and
result notes per applicable test case and variant.

This directory contains product-specific manual release-verification templates
consumed by VHUB. This guide defines the shared authoring format and writing
rules for maintainers and agents across the firmware monorepo.

## Scope

- Maintain one `*.vhub.json` per shipping product; exclude the reference product.
- Include observable outcomes that still need human verification, such as
  display quality, touch response, sound, or faults automation cannot reproduce.
- Inspect host tests, device integration tests, and scripts before adding or
  retaining cases. Do not repeat host-covered parsing, encoding, calculations,
  or deterministic logic checks manually.
- Omit duplicate device integration checks only when execution is required for
  the release candidate and passing results cover the applicable hardware and
  configuration. Optional runs and skipped checks do not replace manual coverage.
- Verify actual assertions prove the outcome. A response-format check that
  accepts failure does not prove successful operation.
- When using a script in a manual case, specify the additional human observations.
  Decoding output is an observation tool, not a separate manual test.
- Share cases across interchangeable sensors; note sensor-specific differences
  within the case. Require separate executions only for different expected
  behavior or explicit release-plan requirements.
- Keep product procedures in its cases or linked documentation. Assess every
  affected product when shared components change.
- When automation replaces a case, record the replacement coverage and required
  execution in the change summary.

```text
vhub/
├── README.md
├── Go.vhub.json
└── FutureProduct.vhub.json
```

## Template Structure

All products use the structured case format defined in this guide. Each template
contains the following fields, with individual cases in `tests`.

| Field | Purpose |
| --- | --- |
| `schema_version` | Declares the format understood by the importer. |
| `rev` | Records the revision associated with the template update. |
| `product` | Contains the product `slug`, `name`, and supported `variants`. |
| `tests` | Contains that product's test cases. |

### Case Fields

All fields below are required. Use empty arrays where explicitly allowed.

| Field | Type | Content |
| --- | --- | --- |
| `id` | String | Stable, unique identifier within the product template, such as `settings.display.unit-persistence`. |
| `title` | String | Short scenario name. |
| `objective` | String | What the case verifies and why it matters. |
| `category` | String | Main grouping. |
| `sub_category` | String | Grouping within the category. |
| `applicability` | String array | Nonempty list of applicable variants declared by the product. |
| `setup` | String array | Case-specific equipment, access, test inputs, and device prerequisites. One independently checkable prerequisite per entry; `[]` if none. Preparation actions belong in `steps`. |
| `steps` | Object array | Nonempty, ordered list of actions and expected results. |
| `cleanup` | String array | Restoration actions, including after an interrupted or failed run; `[]` if none. |
| `references` | String array | Supporting document links; `[]` if none. Use repository-root-relative paths or full URLs. |

### Step Fields

| Field | Type | Required | Content |
| --- | --- | --- | --- |
| `action` | String | Yes | What the tester does. |
| `expected_result` | String | Yes | Observable result that determines whether the step passes. |
| `evidence` | String array | No | Instructions for what to record or attach for this step. Omit when unnecessary. |

There is no separate case-level `expected_result`, `evidence`, or `notes` in the
case format. Expectations belong beside their actions. Evidence fields
contain collection instructions; actual results, device identifiers, firmware
versions, and uploaded evidence belong to the release execution record.

## Writing Rules

A tester should be able to execute a case without designing the experiment or
asking its author what counts as a pass.

| Write Cases That | Avoid |
| --- | --- |
| Verify one coherent scenario with a clear outcome. | Combining unrelated checks because they share a menu or component. |
| List each required resource or eligibility condition in its own `setup` entry. | Packing independent prerequisites into a sentence, hiding preparation actions in setup, or repeating them in both places. |
| Put preparation, baseline capture, and test actions in execution order. | Assuming another case left the device in a particular state. |
| Pair each action with its observable expectation. | Mixing instructions into expected results. |
| Specify concrete inputs, bounded waits, and observable timing start points. | “Works correctly,” “wait a while,” or “retry until it passes.” |
| Derive behavior and limits from the product's requirements and implementation. | Inventing deadlines, log messages, commands, or test tools. |
| Request evidence useful for establishing or investigating the result. | Requiring large log dumps for every visual check. |
| Restore recorded settings and remove only test-created data. | Implicit cleanup or deleting unrelated data. |
| Link exact tool procedures and keep acceptance criteria in the case. | Making QA inspect firmware source to determine a pass. |

### Setup, Steps, and Cleanup

`setup` answers “What does this case specifically require?” Examples include a
controllable network, a test account, or a client with a documented
capability. Name the tool and link its instructions when a special operation is
required, such as interrupting a transfer or decoding an export.

Put each independently checkable prerequisite in its own array entry. Do not
join separate conditions with semicolons or pack them into a paragraph. Keep
qualifications with the resource they describe, such as “A computer with
Bluetooth and Python 3.11 or newer.” Device eligibility, network access, and
expendable data are separate prerequisites.

Include only prerequisites the case needs. Do not repeat the product or variant
already declared by the template and `applicability`, or general release
preparation such as installing the release firmware and recording its version.
Do not add “an initialized device” to every case. Include a starting state only
when it affects this scenario's execution or result; steps that establish that
state make a matching setup entry unnecessary. Use `setup: []` when no specific
prerequisites are needed. Repeated entries must be justified by each case, not
copied as a standard checklist.

Make prerequisites verifiable: use a
documented battery range or observable charge state when it matters, rather
than “sufficient charge.” Do not invent a threshold for a case that does not
need one. Connecting equipment, selecting a mode, creating an evidence
directory, and replacing command placeholders belong in `steps`, not `setup`.

`steps` answers “What do I do?” Powering off, disconnecting a cable, selecting a
mode, recording a setting, and creating test data are actions. Write each action
once at the point it is performed. Preparation steps can have simple expected
results that confirm readiness for the next action.

`cleanup` answers “What must I restore?” Record original values before changing
them. Include restoration after failure or interruption, using only baselines
actually captured. If restoration fails, record the remaining state. Destructive
cases must identify expendable devices or data in setup; do not promise to
restore data the procedure intentionally erases.

### Scope and Evidence

Keep related actions together when they prove one outcome, such as changing a
setting and checking persistence after restart. Split cases when they need
different preparation or would be investigated and rerun independently. Do not
turn every button press or internal code branch into a separate case.

Use logs when they establish an otherwise hidden result or help diagnose a
failure. Exact log text should be a pass condition only when that text is itself
required or is the documented way to observe the behavior. Preserve functional
coverage without binding every case to incidental implementation wording.

For comparisons across interfaces, explain how to identify the same measurement
and account for conversion, rounding, and updates during capture. For faults or
timing-sensitive scenarios, supply a reproducible trigger and a bounded
observation period. If those are unavailable, resolve the procedure before
treating the case as ready for release execution.

An expected rejection passes a negative test. Missing prerequisites block a run;
they do not establish a pass. Required checks cannot be skipped in a passing
run. Record failures before retries, and retain the original result.

## Generic Examples

These examples illustrate writing patterns, not requirements for any shipping
product. `ExampleVariant`, command names, values, and time limits below are
fictional. Replace them with verified product details when authoring a case.

### Visible Output

This compact JSON example shows the complete case shape. It assumes a product
with a documented indicator diagnostic; no firmware implementation is implied.
It needs no special prerequisites, so `setup` is empty.

```json
{
  "id": "diagnostics.indicator.green",
  "title": "Indicator diagnostic displays green",
  "objective": "Verify that the indicator can produce the requested green output.",
  "category": "Diagnostics",
  "sub_category": "Indicators",
  "applicability": ["ExampleVariant"],
  "setup": [],
  "steps": [
    {"action": "Open Diagnostics and select Green Indicator.", "expected_result": "The indicator turns green within two seconds of selection."},
    {"action": "Select Exit.", "expected_result": "The diagnostic closes and the previous indicator behavior returns."}
  ],
  "cleanup": ["If interrupted, exit the diagnostic to restore normal operation."],
  "references": []
}
```

### Setting Persistence

Checking interval persistence does not also prove measurement cadence; that is
a separate scenario. Link the product's configuration and restart procedures in
`references` when adapting this example.

```json
{
  "id": "settings.reporting.interval-persistence",
  "title": "The reporting interval survives restart",
  "objective": "Verify that a saved reporting preference remains effective after restart.",
  "category": "Settings",
  "sub_category": "Reporting",
  "applicability": ["ExampleVariant"],
  "setup": [
    "Access to the device's documented configuration interface."
  ],
  "steps": [
    {
      "action": "Read and record the reporting interval.",
      "expected_result": "The configuration interface returns the current value."
    },
    {
      "action": "Set 30 seconds, or 60 seconds if the original value is already 30. Read the setting back for up to five seconds after the accepted write.",
      "expected_result": "The interface returns the chosen value within that five-second window."
    },
    {
      "action": "Select Restart and wait up to 30 seconds for the configuration interface to become available.",
      "expected_result": "The device restarts and the interface responds within 30 seconds of selecting Restart."
    },
    {
      "action": "Read the reporting interval again.",
      "expected_result": "It equals the value selected before restart.",
      "evidence": [
        "Record the original, selected, and post-restart interval values."
      ]
    }
  ],
  "cleanup": [
    "Restore the original reporting interval and confirm it with a read."
  ],
  "references": []
}
```

### Rejected Input and Recovery

This fictional product accepts integer intervals from 1 through 60 seconds.
Link the client's operation guide and the product's error contract in
`references` when adapting this example.

```json
{
  "id": "settings.reporting.invalid-interval-recovery",
  "title": "An unsupported interval is rejected without changing settings",
  "objective": "Verify that rejecting an invalid input preserves configuration and leaves the interface usable.",
  "category": "Settings",
  "sub_category": "Reporting",
  "applicability": ["ExampleVariant"],
  "setup": [
    "Access to the device's documented configuration client."
  ],
  "steps": [
    {
      "action": "Read and save the current configuration, including the reporting interval.",
      "expected_result": "The client receives a complete configuration snapshot."
    },
    {
      "action": "Submit reporting interval 0 through the configuration client.",
      "expected_result": "Within five seconds of submission, the client reports the documented invalid-value error.",
      "evidence": [
        "Save the rejected request and its response."
      ]
    },
    {
      "action": "Read the configuration and compare it with the saved snapshot.",
      "expected_result": "The interface responds and all configuration values are unchanged.",
      "evidence": [
        "Save both configuration snapshots and record any differences."
      ]
    },
    {
      "action": "Submit reporting interval 30, or 60 if the original interval is already 30, and read it back for up to five seconds after submission.",
      "expected_result": "The request succeeds and the selected value is returned within that five-second window."
    }
  ],
  "cleanup": [
    "Restore the original reporting interval, including if the invalid write unexpectedly changed it, and confirm the restored value with a read."
  ],
  "references": []
}
```

## Maintenance

Review the relevant product template before opening each pull request. Update it
only when firmware changes behavior that a tester can observe through the
device, hardware, serial logs, network interfaces, or server data.

Inspect existing coverage before adding cases. Update a case when its scenario
is unchanged; add a case for a distinct scenario. Preserve unrelated checks and
case IDs. When a split, merge, or removal is necessary, explain the old-to-new
coverage mapping in the change summary so reviewers can detect lost coverage.

Set `rev` to the current seven-character Git HEAD when template content changes:

```sh
git rev-parse --short=7 HEAD
```

This is the repository's current revision convention. Several uncommitted edits
at the same HEAD can share a value; `rev` alone is not proof of an immutable
snapshot. Do not change it for a review that leaves template content unchanged.

The agent workflow is in
[`update-vhub`](../.agents/skills/update-vhub/SKILL.md). It applies this guide to
requested firmware changes; the guide remains the shared source for case format
and writing rules.

## Validation

For a template update, validate JSON syntax and use a compatible schema validator
when one is available. This repository does not currently provide a VHUB schema
validator. Syntax alone does not validate the case structure or prove that a
hardware test passes.

Check unique IDs, required fields and types defined in this guide, applicability
against the product variants, nonempty actions and expectations, and reference
targets. Check that baseline capture precedes mutation and that cleanup covers
changes made by the procedure. Review the diff for lost or duplicated coverage.
Read every setup entry as a checklist item: it must express one relevant,
verifiable, case-specific prerequisite and contain no execution instructions.
Remove product/applicability repetition and general release-preparation items.
Check that the
steps establish temporary state and do not repeat setup as preparation work.

The author is responsible for delivering cases that QA can execute as written,
with confirmed prerequisites, available tools, and clear pass conditions.
Validate Markdown changes with the repository's pre-commit hooks.
