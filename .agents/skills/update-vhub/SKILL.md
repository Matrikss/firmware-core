---
name: update-vhub
description: Assess and update product VHUB manual release-verification cases for firmware changes, PRs, or commit ranges. Use for VHUB maintenance and coverage reviews, not automated firmware test implementation.
---

# Update VHUB

Maintain the affected products' `vhub/*.vhub.json` templates with practical,
reproducible manual cases. Use the shared
[VHUB authoring guide](../../../vhub/README.md) as the source for format,
writing rules, examples, revision handling, and validation. Read it before
assessing coverage, including review-only tasks; keep those rules there rather
than duplicating them in this skill.

## Understand the Change

Establish the requested scope: a PR, commit range, local diff, described behavior,
or explicit template review. Use supplied inputs and available repository context;
ask only when the intended change or target cannot be established. A review-only
request produces findings without edits.

Inspect the actual diff, relevant implementation, product wiring, and nearby
documentation. A PR description is context, not sufficient evidence of behavior.
For shared-component changes, identify each affected shipping product before
choosing templates. Do not assume another product has the same menus, interfaces,
or hardware as the first product inspected.

Identify the before-and-after observable behavior and its sources. Distinguish
intended requirements from incidental implementation details. If the code and
requirements disagree, report the mismatch instead of silently turning a likely
bug into an expected result.

## Assess Coverage

Read the target templates and inspect relevant host tests, device integration
tests, and script assertions. Apply the guide's automation criteria to determine
what still needs manual verification. Match that behavior to existing cases,
including cross-category coverage. Decide whether to update an existing scenario,
add a distinct scenario, remove redundant coverage, or leave coverage unchanged.
Internal-only changes and behavior already adequately covered may require no edit.

Keep overlapping changes from multiple commits or PRs together in the assessment.
Preserve coverage outside the requested scope. Before editing, briefly state the
affected products, cases to change, and any unresolved behavior or tooling question.
Proceed within the user's authorized scope; ask for clarification only when the
missing answer affects correctness.

## Update Cases

Use the structured case format defined in the shared guide for every product.
The guide is the format authority; existing templates are sources of coverage
and product context, not alternative formatting conventions.

Write from the tester's perspective. Verify menu paths, request payloads, timing
limits, and observation methods against the affected product. Do not invent a
client, test fixture, log string, or deadline to make a case appear executable.
Resolve material gaps or report the affected procedure as not ready for execution.

Keep IDs for continuing scenarios. For deliberate splits, merges, or removals,
record replacement case IDs or automated tests and their required execution in
the change summary. Update the template revision using the guide's convention
only when template content changes. Preserve unrelated edits and avoid formatting
churn.

## Validate

Validate the changed JSON and use a compatible repository schema validator if
available. Follow the guide's structural and procedural checks even when JSON
parses successfully. Confirm references resolve and inspect the final diff for
missing coverage or accidental changes to other products.

For documentation-only or template-only edits, run the relevant documentation
and template checks. If the task also changes firmware, complete the repository's
required firmware verification separately. Template validation does not execute
the manual cases. Do not flash devices, claim hardware results, or record a release
as passed through this maintenance workflow.

## Report

Report affected files and case IDs, the observable behavior covered, validation
performed, and any unresolved assumptions or checks requiring QA execution.
Explain significant coverage splits or removals. If no template edit was needed,
explain why the behavior is internal-only or already covered. Do not imply that
tests passed merely because their definitions were updated.
