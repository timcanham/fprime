# CmdSequencer Directives — Review Actions

Reviews of the sequence-directive feature (`executeDirective`, `jumpToLabel`,
`performCmd_Step`, `cmdResponseIn_handler`, and the new member variables).

| Review | Source | Findings |
| --- | --- | --- |
| Part A — C/C++ design | `.github/skills/fprime-cpp-design/SKILL.md` (CPP-1 … CPP-37) | 3 must-fix, 2 could-fix, 3 suggestions |
| Part B — Correctness | `.github/agents/correctness-review.agent.md` | 5 must-fix, 2 could-fix |
| Part C — Security | `.github/agents/security-review.agent.md` | 1 must-fix, 3 suggestions, 1 future work |

**CI safety: Go** — no outstanding category-8 `**must fix**`.

## Recommended order

**Do B1 first** — it is also C1, the one security must-fix, and the single
edit (making the directive continuation a loop instead of a self-call) closes
an uplink-reachable FSW crash. Then the rest of Part B: two more defects
poison or misreport sequence state. Part A is conformance work that does not
change behavior; Part C's remaining items are observability and CI hygiene.

Within Part A, do action A1 first: findings CPP-19, CPP-24, CPP-30, and part
of CPP-10 all live inside the block it extracts, so four of the eight
collapse into that one edit.

---

## Part A — C/C++ Design (CPP rules)

---

### A1. Extract the duplicated label-parse block

- [ ] Add a private helper `deserializeLabel(Fw::ExternalSerializeBuffer&, Fw::String&)`
- [ ] Replace the three inlined copies with calls to it

**Rule:** CPP-33 `cpp-inlined-utility` — **could fix**

**Sites:** `CmdSequencerImpl.cpp:666-690` (JCF), `:747-771` (JCS), `:848-861` (`jumpToLabel`)

The block — deserialize length, bounds-check against 20, deserialize chars
with `OMIT_LENGTH`, null-terminate — is triplicated verbatim. It depends on
no component state and operates on general types.

---

### A2. Justify or remove the casts

- [ ] `const_cast<U8*>(...getBuffAddr())` at `CmdSequencerImpl.cpp:621`
- [ ] `const_cast<U8*>(...getBuffAddr())` at `CmdSequencerImpl.cpp:836`
- [ ] `reinterpret_cast<U8*>(labelBuf)` at `:684`, `:765`, `:856` — these disappear if A5 is done

**Rule:** CPP-10 `cpp-reinterpret-or-const-cast-unjustified` — **must fix**

Five casts on a flight path with no inline justification. SKILL §3 upgrades
unjustified `const_cast` / `reinterpret_cast` on flight code to must-fix.

The `const_cast` pair is the more serious shape: it strips const off a record
buffer to hand to `ExternalSerializeBuffer`. Either use a const-correct
deserializer entry point, or add a comment stating the buffer is not mutated
and why the API forces non-const.

---

### A3. Make the new event emissions traceable

- [ ] Add a distinct event for directive jumps, carrying the target label
- [ ] Stop emitting `CS_SequenceCanceled` at `:699` and `:779`
- [ ] Give `CS_InvalidMode` an FPP enum argument identifying the call site

**Rule:** CPP-36 `cpp-event-not-uniquely-traceable` — **must fix**

Two problems:

`CS_SequenceCanceled(fileName)` now fires from 4 sites (`:221`, `:232`,
**`:699`**, **`:779`**) with an indistinguishable argument set. Worse, at
699/779 the event is factually wrong — the sequence *jumped*, it wasn't
canceled. An operator cannot tell a JCF jump from a real cancel.

`CS_InvalidMode()` takes **no arguments** and now fires from 6 sites (`:96`,
`:163`, `:438`, `:488`, **`:661`**, **`:742`**). At 661/742 it means "JCF/JCS
before any command ran" — unrecoverable from the log alone.

Sites 221/232/96/163/438/488 are preexisting → **future work**. The four new
emissions are in scope.

---

### A4. Initialize the new locals

- [ ] `directiveId` at `:625` and `:840`
- [ ] `labelLen` at `:667`, `:748`, `:848`
- [ ] `labelBuf` at `:676`, `:757`, `:854`
- [ ] `exitStatus` at `:717`
- [ ] `mode` at `:797`

**Rule:** CPP-19 `cpp-uninitialized-variable` — **must fix**

Each is written by `deserializeTo` before use, so there is no UB today — but
CPP-19 is unconditional and SKILL §3 forbids downgrading outside test code.
One-line fix each:

```cpp
U8 directiveId{};
```

Most of these vanish with A1.

---

### A5. Use `Fw::String` for the label buffer

- [ ] Replace `char labelBuf[21]` at `:676`, `:757`, `:854`

**Rule:** CPP-24 `cpp-char-pointer-where-fw-string-fits` — **suggestion**

The buffer plus manual null-termination is converted to `Fw::String` at `:695`
and `:776` anyway. Deserialize into an `Fw::String` (or an `Fw::ExternalString`
over the fixed buffer) and drop the hand-rolled terminator. This also removes
the three `reinterpret_cast` sites in A2.

---

### A6. Name the label-size constants

- [ ] Add constants to the `Record::Constants` enum
- [ ] Replace literals at `:676-678`, `:757-759`, `:850-854`

**Rule:** CPP-30 `cpp-magic-number-replacing-constant` — **could fix**

The 20-char label limit and its 21-byte buffer are bare literals at six
points, with the `+1` relationship implicit. The `Record::Constants` enum next
door is the idiomatic home:

```cpp
enum Constants { MAX_LABEL_SIZE = 20, LABEL_BUFFER_SIZE = MAX_LABEL_SIZE + 1 };
```

---

### A7. Return a status enum from `executeDirective`

- [ ] Replace the `bool` return at `CmdSequencerImpl.hpp:671`

**Rule:** CPP-35 `cpp-bool-status-where-enum-fits` — **suggestion**

The `bool` collapses six distinct outcomes: malformed record, unknown directive
ID, directive before any command, label not found, bad exit status, bad error
mode. The caller at `CmdSequencerImpl.cpp:529` can only `performCmd_Cancel()`.
An FPP `DirectiveStatus` enum would let the cause reach the ground.

Not upgraded to must-fix because each cause currently logs its own event
before returning.

---

### A8. Bound the label search loop

- [ ] Convert the `while` at `CmdSequencerImpl.cpp:830` to a bounded `for`

**Rule:** CPP-34 `cpp-while-loop-for-counted-iteration` — **suggestion**

`while (this->m_sequence->hasMoreRecords())` walks the record list with no
visible upper bound; `nextRecord` returns `void`, so termination rests
entirely on `hasMoreRecords` being monotonic. CPP-27 / JPL requires a provable
bound. `getHeader().m_numRecords` is available — a `for` over it makes the
bound explicit and the three `continue` paths safe by construction.

---

## Part B — Correctness

All seven are confirmed against the source: each has a concrete triggering
record sequence, a traced path to the wrong outcome, and no upstream guard
that prevents it. Four (B1, B2, B3, B4) were introduced by the
JCF/JCS-after-command redesign; B5 is inherent to the redesign's state
model; B6 and B7 are pre-existing shapes that the redesign made reachable.

`m_errorPendingAbort` is the thread running through B2, B3, B4 and B7 — a
single flag cleared in three places and not in the fourth. Consider fixing
them as one change.

---

### B1. Directive jump loop recurses without bound

- [ ] Clear `m_lastCmdExecuted` / `m_lastCmdStatus` after a JCF or JCS jump
- [ ] Convert the directive continuation at `CmdSequencerImpl.cpp:535` from recursion to iteration

**Class:** `correctness-nontermination` — **must fix**

`performCmd_Step` (`:532-535`) calls **itself** to advance past a directive.
`m_lastCmdStatus` is never cleared once a JCF/JCS has consumed it, so a
backward jump that reaches its own JCF again with no command in between
recurses forever.

Trigger — three records, no malformed input required:

```text
RELATIVE  <command that returns anything but OK>
LABEL     "RETRY"
JCF       "RETRY"
```

Trace: command fails → `cmdResponseIn_handler:340` stores
`m_lastCmdStatus = <failure>` → `performCmd_Step` → `LABEL` is a no-op →
`performCmd_Step` → `JCF` sees `m_lastCmdStatus != OK` → `jumpToLabel("RETRY")`
rewinds the deserializer to just past `LABEL "RETRY"` → returns `true` →
`:535` recurses → reads `JCF "RETRY"` again → `m_lastCmdStatus` is *still* the
failure → jumps again. No frame ever returns.

Consequence: stack exhaustion, then FSW abort. Occurs under both error modes
(with ERROR_MODE ON the first JCF clears `m_errorPendingAbort`, so the guard
at `:644` does not stop the second pass).

The recursion is also unbounded in the benign case: a run of *N* consecutive
directives costs *N* stack frames, so a long `LABEL` block is a stack-depth
hazard independent of B1's loop. Both go away if the continuation becomes a
loop over records rather than a self-call.

---

### B2. `m_errorPendingAbort` survives into the next sequence

- [ ] Add `this->m_errorPendingAbort = false;` to `sequenceComplete` (`CmdSequencerImpl.cpp:551-579`)

**Class:** `correctness-state-machine` — **must fix**

`sequenceComplete` clears `m_executedCount`, `m_lastCmdExecuted`,
`m_lastCmdStatus` and `m_errorMode` (`:557-565`) — but not
`m_errorPendingAbort`. Only `performCmd_Cancel` (`:312`) and the constructor
clear it.

Trigger: a sequence whose **last record is a failing command** — legal,
because `validateRecords` (`FPrimeSequence.cpp:303`) does not require a
terminating `END_OF_SEQUENCE` record, and `CS_STEP_cmdHandler:443` documents
sequences without one as a supported case.

Trace: command fails with ERROR_MODE ON → `m_errorPendingAbort = true`
(`:349`) → `cmdResponseIn_handler:355` finds `not hasMoreRecords()` →
`sequenceComplete()` → flag stays `true` with no sequence running.

Consequence: the **next** sequence run is killed at its first non-directive
record by the guard at `:508`, emitting `CS_SequenceCanceled` with no event
explaining the cause. One poisoned sequence per occurrence; `CS_RUN` of a
valid file appears to fail for no reason.

```cpp
// in sequenceComplete, alongside the existing resets
this->m_errorMode = true;
this->m_errorPendingAbort = false;
```

---

### B3. A failed final command reports sequence success

- [ ] In `cmdResponseIn_handler`, take the error exit when a failure leaves no more records

**Class:** `correctness-state-machine` — **must fix** (regression)

Same trigger as B2 — last record is a failing command, no `END_OF_SEQUENCE`.
Both the AUTO branch (`:355-358`) and the MANUAL branch (`:364-367`) call
bare `sequenceComplete()`, which defaults to `Fw::CmdResponse::OK`. So
`seqDone_out` and `cmdResponse_out` both report **OK**, and
`CS_SequenceComplete` is logged, for a sequence whose last command failed
with ERROR_MODE ON.

Before the redesign this path called `performCmd_Cancel()` and reported
`EXECUTION_ERROR`. Consequence: ground and any upstream sequencer see a
clean completion; a blocking `CS_RUN` returns success. Failure is invisible
except for the separate `CS_CommandError` event.

```cpp
if (not this->m_sequence->hasMoreRecords()) {
    this->m_runMode = STOPPED;
    if (this->m_errorPendingAbort) {
        this->performCmd_Cancel();      // reports EXECUTION_ERROR, clears the flag
    } else {
        this->sequenceComplete();       // ERROR_MODE OFF: failure was accepted
    }
}
```

---

### B4. Non-JCF directive after a failed command cancels twice

- [ ] Make the guard at `CmdSequencerImpl.cpp:643-649` signal the caller instead of canceling itself

**Class:** `correctness-state-machine` — **must fix**

`executeDirective`'s pending-abort guard calls `performCmd_Cancel()` *and*
returns `false` (`:646-648`). Its only caller, `performCmd_Step:529-531`,
treats `false` as "abort" and calls `performCmd_Cancel()` again.

Trigger: ERROR_MODE ON (the default), a failing command, and any non-`JCF`
directive next — `LABEL`, `JCS`, `EXIT`, or `ERROR_MODE`:

```text
RELATIVE  <command that fails>
LABEL     "CLEANUP"
```

Consequence: `seqDone_out(0, 0, 0, EXECUTION_ERROR)` is invoked **twice** for
one sequence run (`:315` has only an `isConnected` guard). A downstream
sequencer or counter sees two completions for one sequence. `cmdResponse_out`
is spared only incidentally, because the first call leaves `m_blockState ==
NO_BLOCK` and `m_join_waiting == false`.

Fix: drop the `performCmd_Cancel()` from the guard and let the caller own the
cancel — or, with A7, return a dedicated status the caller dispatches on.

---

### B5. JCF/JCS reached by a jump tests a stale command status

- [ ] Clear `m_lastCmdExecuted` / `m_lastCmdStatus` once a JCF/JCS jump is taken

**Class:** `correctness-state-machine` — **must fix**

`m_lastCmdStatus` is written only by `cmdResponseIn_handler:340` and cleared
only in `performCmd_Cancel` / `sequenceComplete`. It is therefore sticky
across jumps: a JCF/JCS reached by control flow rather than by falling out of
a command still evaluates the status of whatever command last ran.

Trigger: an error handler entered by a jump that itself begins with a
conditional directive:

```text
RELATIVE  <command A, fails>
JCF       "HANDLER"
...
LABEL     "HANDLER"
JCS       "DONE"          <-- tests command A, several records back
```

Consequence: the wrong branch is taken. A handler cannot distinguish "no
command has run since I was entered" from "the command before my caller
succeeded", so recovery logic silently takes the success path. This is also
what makes B1's loop unbounded, so the same fix addresses both.

`docs/sdd.md:309` deliberately allows *consecutive* JCF/JCS after one command
to all test that command — so clear on **jump taken**, not on every
evaluation, to preserve the documented behavior.

---

### B6. `m_executedCount` freezes on a failed command

- [ ] Advance `m_executedCount` on the failure path in `cmdResponseIn_handler:345`

**Class:** `correctness-other` — **could fix**

`commandComplete` (`:581-587`) increments `m_executedCount` and is called
only on the success path. The failure path calls
`commandError(this->m_executedCount, ...)` and increments nothing.

Trigger: ERROR_MODE OFF with commands A (fails) and B (succeeds). A's
`CS_CommandError` reports index 0; B's `CS_CommandComplete` also reports
index 0.

Consequence: after the first failure, every subsequent `CS_CommandComplete`,
`CS_CommandError` and `CS_SequenceTimeout` reports a record index low by the
number of prior failures, and `CS_RecordInvalid` in `executeDirective`
inherits the same skew. The operator cannot map events back to records.
Diagnostics only — no control-flow effect, hence could-fix. Before the
redesign a failure ended the sequence, so the counter never had to survive
one.

---

### B7. `CS_STEP` reports OK after aborting the sequence

- [ ] Check `m_runMode` before the response at `CmdSequencerImpl.cpp:454`

**Class:** `correctness-other` — **could fix**

Trigger: MANUAL step mode, ERROR_MODE ON, a command fails →
`m_errorPendingAbort = true` and the handler waits (`:364`). The operator
sends `CS_STEP` → `performCmd_Step` hits the guard at `:508` →
`performCmd_Cancel()` → `m_runMode = STOPPED`.

Back in `CS_STEP_cmdHandler`, `:451` suppresses `CS_CmdStepped` because
`m_runMode == STOPPED`, but `:454` still sends
`cmdResponse_out(..., Fw::CmdResponse::OK)`.

Consequence: `CS_STEP` acknowledges success while it in fact terminated the
sequence. The pre-existing shape is unchanged, but the guard at `:508` gave
it a new way to be reached; before the redesign a failing command in MANUAL
mode had already canceled the sequence, so no `CS_STEP` followed.

---

## Part C — Security

**Source classification.** `Svc::CmdSequencer` is named in
`fprime-ground-input-tracing` §1 twice over — as a detector component whose
scanned payload is ground-input, and as a consumer of uplinked file content.
So **every byte of a sequence record, including all directive payloads, is
`ground-input` at zero hops.** High confidence; no topology trace needed.

**What the directive code gets right.** Categories 1, 3, 4 and 6 produced no
findings, which is worth stating rather than leaving implicit:

- Every `deserializeTo` in `executeDirective` and `jumpToLabel` has its status
  checked before the value is used.
- Every ground-supplied scalar is range-checked before use: `directiveId <=
  ERROR_MODE` (`:634`), `labelLen <= 20` (`:678`, `:759`, `:850`), `exitStatus
  <= 1` (`:726`), `mode <= 1` (`:806`).
- No `memcpy`, no allocation, and no queue push takes a ground-controlled
  size. The only ground-sized copy is the label read, bounded above.

The one memory-write on a ground-controlled index, `labelBuf[readSize] =
'\0'`, is in-bounds: `readSize <= 20` is enforced before the call, and
`deserializeTo` only ever lowers it. `char labelBuf[21]` — max index 20.

---

### C1. Ground-controlled recursion depth — uplink-triggerable stack exhaustion

- [ ] Convert the directive continuation at `CmdSequencerImpl.cpp:535` to iteration

**Class:** `general-vulnerability/unbounded-recursion` — **must fix**

*Concurrence with **B1** — same site-key, same fix. Recorded here for the
ground-reachability classification and the DoS consequence, not as a second
defect.*

`performCmd_Step` recurses into itself once per consecutive directive
(`:532-535`). The recursion depth is set by the number of consecutive
`SEQUENCE_DIRECTIVE` records in the uplinked file — a `ground-input` count
with **no bound check**, which is category 2's shape applied to stack rather
than heap.

Quantified for the Ref deployment, which allocates a 5 KB sequence buffer
(`TestDeploymentsProject/Ref/Top/RefTopology.cpp:60`): a minimal `LABEL`
record is 15 bytes — descriptor 1 + time tag 8 + record size 4 + payload 2 —
so a single valid 5 KB sequence file yields **~341 records**, hence ~341
nested `performCmd_Step` → `executeDirective` frame pairs. Each pair carries
an `ExternalSerializeBuffer`, a `char[21]`, and an `Fw::Time`.

Three distinct ways ground reaches this, in increasing severity:

1. **A well-formed sequence with a long directive run** — no malformed input,
   no loop. Depth scales with the sequence buffer size.
2. **A well-formed retry loop** — because each taken jump also consumes a
   frame that is never popped, a legitimate `LABEL`/`JCF` retry loop grows the
   stack linearly in *iterations executed*, not in file size. A sequence
   author writing an ordinary bounded retry crashes the FSW.
3. **Unbounded** — the `LABEL "R"` / `JCF "R"` construction in B1 recurses
   forever.

Consequence: stack overflow in the sequencer task, then FSW abort. I could not
resolve the sequencer task's stack budget (`Default::STACK_SIZE` is not
defined within read scope), so the exact file size that overflows is
deployment-specific — but cases 2 and 3 exceed any finite budget.
*Low confidence on the numeric threshold only; the unboundedness is certain.*
Per `maintainer-lookup`, security-overseer `@bitWarrior` should confirm the
per-deployment stack budget.

Converting `:535` to a loop fixes all three, and is the same edit B1 needs.

---

### C2. `jumpToLabel` discards malformed uplinked directives silently

- [ ] Emit `CS_RecordInvalid` at the three `continue` sites (`:843`, `:851`, `:859`)

**Class:** `ground-validation-gap` — **suggestion**

`jumpToLabel` skips any directive it cannot parse — bad directive ID (`:843`),
bad or oversized label length (`:851`), failed label read (`:859`) — with a
bare `continue` and **no event**. Every other rejection of malformed ground
input in this component logs `CS_RecordInvalid`.

Consequence: a malformed `LABEL` record is invisible to the ground. The jump
either silently resolves to a *different* label further on or silently fails
the whole sequence with only a generic `CS_CommandError`, and the operator has
no evidence which record was rejected or why. Malformed uplinked content
should never be discarded without a trace.

Related, same function: `jumpToLabel` returns the **first** matching label and
nothing at load time rejects duplicate `LABEL` names, so a sequence with two
`LABEL "X"` records always jumps to the first. Worth a validation at load.

```cpp
if (status != Fw::FW_SERIALIZE_OK || labelLen > 20) {
    this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(),
                                          this->m_executedCount, labelLen);
    continue;
}
```

---

### C3. Label search rescans from record 0 on every jump

- [ ] Build a label index at load, or search forward from the current position

**Class:** `general-vulnerability/unbounded-loop` — **suggestion**

`jumpToLabel:827` calls `reset()` and rescans the whole record list on every
jump, synchronously on the sequencer's thread. Work is O(records) per jump and
O(records²) over a sequence that jumps often — with the record count
ground-controlled.

The loop does terminate: `validateRecords` (`FPrimeSequence.cpp:303-331`)
proves every record deserializes and consumes the buffer exactly, so
`hasMoreRecords()` is monotonic. At Ref's 5 KB buffer the worst case is ~341²
≈ 116 K record deserializations — milliseconds, not a denial of service on its
own, which is why this is a suggestion and not a must-fix. It is listed
because it is a ground-scaled cost on a blocking path: `CS_RUN` in `BLOCK`
mode holds the caller's command response for the duration.

---

### C4. Integration test uplinks to a fixed path outside the working tree

- [ ] Replace `/tmp/ref_test_seq*.bin` with a unique per-run destination
      (`Svc/CmdSequencer/test/int/test_cmd_sequencer.py:169-172` and its 7 reuses)

**Class:** `ci-test-runtime-policy-violation` — **suggestion**

The new integration test uplinks to hard-coded `/tmp/ref_test_seq.bin` and
`/tmp/ref_test_seq_wait.bin`. Per `ci-test-runtime-policy`, writing outside
the working tree at a predictable path is a category-8 trigger: on a shared
runner the destination is world-writable and pre-creatable, and two concurrent
CI runs collide on the same filename.

Does **not** force `CI safety: No-Go` — it is a `**suggestion**`, not a
`**must fix**`, and the rule gates on outstanding category-8 must-fixes only.

Clean in the same file, worth noting so it is not "fixed" into something
worse: the `fprime-seqgen` invocations at `:140-165` use `subprocess.run` with
a **list** argument and no `shell=True`. That is the correct form; keep it.

---

### C5. Shell-interpreted `system()` in the unit-test path

- [ ] Replace with `Os::FileSystem::removeFile` in
      `test/ut/SequenceFiles/File.cpp:95` and
      `test/ut/SequenceFiles/AMPCS/CRCs.cpp`

**Class:** `ci-test-runtime-policy-violation` — **future work**

`File::remove()` builds `"rm -f " + name` and calls `system()`, spawning a
shell during every unit-test run — a category-8 primitive.

Tagged `**future work**`, not `**must fix**`: the branch only reformatted
these lines (`git diff` shows the `system()` call present on both sides), so
the behavior is neither introduced nor widened, and the concatenated path is a
compile-time constant, so no untrusted value reaches the shell. Replacing it
removes the shell from the test path entirely.

---

## Working-tree risk — not part of this change

**Local credential files are not ignored by git.** `.auth_key` and
`.mylocalauthcache` (repo root and `TestDeploymentsProject/`) are matched by
**no** `.gitignore` rule — `git check-ignore` returns nothing for any of them.
While they exist, `git add -A` or `git commit -a` would stage credentials into
a commit. They are local GenAI-auth artifacts, not content of this branch, so
they are outside the review's scope.

They are absent from the working tree as of this writing — a `git clean` run
removed them (along with an earlier copy of this file). The auth helper
recreates them on use, and they are still unignored when it does, so the
exposure returns.

*(Contents never read — `~/.claude/settings.json` denies reads on both
patterns. Only ignore status was checked.)*

Fix locally, without touching the shared `.gitignore`, since these are
user-specific tool artifacts. Note that `.gitignore` would not protect this
file or those from `git clean -xdf`, which deletes ignored files too:

```bash
printf '.auth_key\n.mylocalauthcache\n' >> .git/info/exclude
```

---

## Ruled out

Examined and confirmed **not** defects — recorded so they are not re-filed:

- **`jumpToLabel` walking off the end of the record list.** `nextRecord`
  `FW_ASSERT`s on a deserialize failure (`FPrimeSequence.cpp:~120`), but
  `validateRecords` (`:303-331`) already deserializes every record and
  rejects any trailing bytes on every `loadFile`, so the walk in
  `CmdSequencerImpl.cpp:830` cannot reach malformed data.
- **`labelBuf[readSize] = '\0'` out of bounds.** `readSize` is bounded to 20
  and the array is `char labelBuf[21]`; max index written is 20. Correct.
- **`FW_ASSERT(false, directive)` in `executeDirective`'s `default`.**
  Unreachable: `:634` rejects `directiveId > ERROR_MODE` and the enum is
  dense from 0.
- **`deserializeTo` mutating `readSize`.** Documented in-out parameter
  (`Fw/Types/Serializable.hpp:455-491`); the subsequent bound check uses the
  post-call value, which is the intended contract.

Every `FW_ASSERT` reachable from the directive code was traced to a source
class and cleared — no ground- or hardware-reachable assert exists on this
path:

- **`FW_ASSERT(false, directive)`** (`CmdSequencerImpl.cpp:817`). `directive`
  is `ground-input`, but `:634` rejects `directiveId > ERROR_MODE` and
  `DirectiveId` is dense over 0…4, so every surviving value has a `case`.
  Unreachable.
- **`nextRecord`'s `FW_ASSERT(status == FW_SERIALIZE_OK)`** reached through the
  *new* caller `jumpToLabel:832`. The new caller does widen its reach, so this
  was checked closely: `validateRecords` deserializes all `numRecords` records
  from offset 0 and rejects any trailing bytes on every `loadFile`, and
  `jumpToLabel` replays that identical walk after `reset()`. Cannot fire.
- **`copyCommand`'s `FW_ASSERT` on `setBuffLen(recordSize)`**
  (`FPrimeSequence.cpp:298`) with `recordSize` ground-controlled.
  `deserializeRecordSize` (`:277-289`) rejects `recordSize` exceeding both the
  bytes remaining and `FW_COM_BUFFER_MAX_SIZE` first. The `recordSize +
  sizeof(FwPacketDescriptorType)` addition cannot overflow U32 because the
  remaining-bytes check runs first and short-circuits.
- **`extractCRC`'s `FW_ASSERT(buffSize >= crcSize)`** — preceded by an explicit
  `if (buffSize < crcSize) return false;`. Redundant, not reachable.
- **`jumpToLabel:824` `FW_ASSERT(m_sequence != nullptr)`** — `internal-state`,
  set at load; `jumpToLabel` only runs while executing a loaded sequence.

Also cleared: **`setBuffLen(record.m_command.getSize())` at `:621-623` and
`:836-838` does not expose uninitialized memory.** `getSize()` is the valid
serialized length (it replaces the deprecated `getBuffLength()`), not the
`ComBuffer` capacity, so `dirBuf` spans only bytes `copyCommand` actually
wrote. The call is redundant, not unsafe.

---

## Routed elsewhere

Outside both rule sets, but worth tracking:

**Dead state → maintainability review.** `m_jcfActive` / `m_jcfTarget` /
`m_jcsActive` / `m_jcsTarget` (`CmdSequencerImpl.hpp:736-742`) are marked
"NO LONGER USED (kept for compatibility)" but still written in
`performCmd_Cancel` (`:304-308`). Write-only state. Also conflicts with the
repo's own no-backwards-compat-shims guidance.

**Pending-abort adjacency → design review.** The guard at
`CmdSequencerImpl.cpp:644` aborts unless the *immediately* next directive is
`JCF`. A `LABEL` or `ERROR_MODE` record between a failed command and its
`JCF` therefore kills the sequence. `docs/sdd.md:309` ("multiple consecutive
JCF … the first matching directive will jump") reads as if intervening
directives were tolerated. This is a design question — which records may
separate a command from its handler — rather than a coding defect; B4 covers
the mechanical fault on the same path.
