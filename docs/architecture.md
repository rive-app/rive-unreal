# Rive Unreal plugin architecture

How the plugin is put together, and why the less obvious parts work the way
they do. Code comments stay short and point here; this is where the detail
lives.

Sections marked TODO are placeholders for parts of the plugin this document
doesn't cover yet.

## Modules

TODO: what each module owns and how they depend on each other.

| Module | Role |
|---|---|
| `Rive` | Runtime: files, artboards, state machines, view models, the UMG widget |
| `RiveRenderer` | Rendering on RHI, and the command builder that talks to the command server |
| `RiveShaders` | Shader types and registration |
| `RiveEditor` | Asset import, factories, details customizations |
| `RiveEditorNodes` | Blueprint nodes for artboards and view models |
| `RiveStats` | Stat groups |

## Threading and the command server

The Rive runtime runs behind a command server. Game-thread code never touches
a `rive::StateMachineInstance` directly: it sends work to the server through
`FRiveCommandBuilder` (`RiveRenderer/Public/RiveCommandBuilder.h`), and the
server runs it on the render thread.

There are three ways to send work, and the choice matters for both ordering
and cost:

| Call | When it runs | Cost on the game thread |
|---|---|---|
| `RunOnce` | Buffered, sent with the rest of the frame's commands in `Execute` | None beyond adding to an array |
| `RunOnceImmediate` | Queued on the command queue straight away, in order with other queue commands | One uncontended lock |
| `RunOnServerAndWait` (`RiveStateMachine.cpp`) | Queued immediately, then the game thread blocks until the render thread has processed it, with a 100 ms timeout | A game/render sync point: the game thread can stall until the render thread catches up, which may be most of a frame |

Rules of thumb:

- Anything that must stay in order with an immediate call (another focus
  call, a gamepad batch, a pointer event) uses `RunOnceImmediate`. Mixing
  `RunOnce` and `RunOnceImmediate` for related work lets a buffered command
  run after a later immediate one.
- Avoid `RunOnServerAndWait` on paths that run per frame or per auto-repeat.
  When the game thread needs an answer, prefer a cached value written by the
  server (see [Focus state cache](#focus-state-cache)) or a result posted
  back to the game thread (see [Navigation](#navigation)).

TODO: draw commands, the render target updater, and how `Execute` is driven.

## Input

### Pointer

TODO: hit testing, fit and alignment mapping, button masks.

The press and release round trips (`PointerDown`, `PointerUp`) still use
`RunOnServerAndWait`, so the widget can build an honest `FReply` from whether
something in the artboard took the press.

### Keyboard and text

TODO.

`KeyInput` and `TextInput` reach the focused node and its ancestors, and also
wait on the server for whether anything handled them.

### Gamepad

`URiveWidget` turns Unreal gamepad keys and analog values into the runtime's
W3C standard gamepad batches (`FRiveStateMachine::GamepadInput`):

- Buttons map to W3C button slots 0 to 15, except the triggers (6 and 7),
  which come from the trigger axes below. Unreal has no standard key for the
  home button (16).
- Sticks map to axes 0 to 3, with Y negated: Unreal reads positive up, W3C
  positive down.
- Triggers come from the trigger axes and fill both their button slot (as
  pressure) and axes 4 and 5. The digital trigger keys are ignored so a press
  isn't reported twice.
- The first event from a device announces it with a `connected` record, and
  values that haven't changed aren't sent again.
- Batches are fire and forget, sent with `RunOnceImmediate` so a press keeps
  its order with focus moves.

Gamepad events reach state machine listeners by bubbling up from the focused
node, and reach every script with a gamepad handler regardless of focus.

The runtime doesn't report whether a listener took a gamepad event, so
`URiveWidget` marks a button press handled when the artboard has focus, and
leaves it unhandled otherwise so the game can use it (B backing out of a
screen, for example). Navigation keys are always left unhandled, so Slate can
turn them into navigation.

When the widget loses focus it releases every held button and recentres the
sticks (`ReleaseGamepads`). The release of a held button goes to whatever
took focus, often the very press that moved it, so without this the artboard
would keep the button down.

## Focus

### Slate focus and Rive focus

There are two layers of focus:

- **Slate focus** decides which widget receives key, gamepad and navigation
  events. A `URiveWidget` takes it from a press on something in the artboard,
  from UMG navigation arriving, or from `SetFocus`.
- **Rive focus** is the artboard's own: which node inside it is focused.
  Keyboard and gamepad listeners only hear events through a focused node.

The widget keeps the two in step:

- Losing Slate focus clears Rive focus (`NativeOnFocusLost`), so a text input
  stops its caret.
- Gaining Slate focus enters the artboard's focus tree at its first stop
  (`NativeOnFocusReceived`), so listeners work whichever way the widget got
  focus back. A press that hit a node still wins: its focus request applies on
  the next advance, after the entry.
- Swapping the artboard while the widget holds focus (`SetArtboard`,
  `SetRiveDescriptor`) enters the new artboard the same way, because no Slate
  focus change happens to trigger it. The new state machine is created on the
  command queue before the entry is sent, so the entry lands on it.

`URiveWidget` defaults to `Visible` rather than UMG's `SelfHitTestInvisible`.
Slate only puts hit-test-visible widgets in its hittest grid, and the grid is
where navigation looks for the next widget to focus, so without this a pad
could leave the artboard but never return.

### Focus state cache

`FRiveStateMachine::HasFocus` reads an atomic rather than asking the server,
so it never waits. The server-side focus calls (`FocusFirst`, `ClearFocus`,
`MoveFocus`) write it, and a `RunOnce` after each advance refreshes it,
because listeners apply their focus changes (a press focusing a node, a
traversal action) during the advance, not when the event arrives.

The cache can be a frame behind, so one press can be judged against the
previous frame's focus.

Decisions that must not be wrong stay on the server. `NativeOnFocusReceived`
calls `FocusFirst` without asking `HasFocus` first, and `FocusFirst` does
nothing when something is already focused. Asking the cache first would miss a
`ClearFocus` queued earlier in the same frame, skip entry, and leave the
widget with nothing focused once that clear ran.

### Tab running off the end

Running off either end of the tab order clears Rive focus before the move
reports failure. If Slate then has nowhere to send focus (no neighbour, or a
Stop rule), the widget keeps Slate focus with nothing focused inside it:
listeners go quiet and every later move is refused.

`MoveFocusOnServer` remembers the focused node before a Next or Previous move
and puts it back if the move fails and leaves nothing focused. If Slate does
move on, `NativeOnFocusLost` clears it as usual. The node hears a blur and
then a focus on the same advance, so a file that animates on focus may replay
that animation at a Tab edge.

### Navigation

Arrows, Tab, the d-pad and the left stick (by default; Slate's navigation
config decides which keys and axes navigate) arrive at
`URiveWidget::NativeOnNavigation` once the key or stick event is left
unhandled. They move focus inside the artboard first, using the runtime's
`focusLeft`, `focusRight`, `focusUp`, `focusDown`, `focusNext` and
`focusPrevious`, and the widget leaves for a UMG neighbour only when the
artboard has nothing further that way.

#### Why navigation is deferred

Slate wants the answer, move inside or leave, as `NativeOnNavigation`'s
return value. Only the command server knows it, and asking synchronously
(`RunOnServerAndWait`) stalls the game thread until the render thread catches
up: a game/render sync point on every navigation event, including each
auto-repeat while the d-pad or stick is held.

So the answer is not waited for:

1. `NativeOnNavigation` sends the move to the artboard without waiting and
   returns `FNavigationReply::Stop()`. Slate keeps focus on the widget for
   now. Stop is what any widget answering Stop already causes: no
   destination, no move.
2. The command server makes the move and posts the result back to the game
   thread (`FRiveStateMachine::MoveFocus`).
3. If the artboard moved, there is nothing more to do. If it had nowhere to
   go, `URiveWidget::FinishNavigation` replays the original navigation
   through `FSlateApplication::ProcessReply`, the same entry point the key
   press itself went through (`SWidget::OnKeyDown` returns a reply carrying
   the navigation). Slate then walks the focus path from the widget up,
   calling `OnNavigation` on each widget, and acts on the first reply that
   isn't Escape. During that replay, `bReplayingNavigation` makes
   `NativeOnNavigation` answer with the widget's own UMG rules
   (`InDefaultReply`) instead of asking the artboard again.

#### Why replay rather than search

Replaying through Slate is what keeps normal Slate behaviour. All of these
apply exactly as if the widget had escaped at once:

- the Navigation rules set on the Rive widget in the designer (Explicit,
  Wrap, Stop, Custom);
- rules and `OnNavigation` overrides on parent widgets;
- CommonUI routing and viewport handling;
- the `Slate.NavigationDebugging` view.

`FSlateApplication::NavigateFromWidget` would be simpler, but it searches
with an Escape rule from every level and skips all of that.

Leaving happens a frame or two after the press, once the server has run the
move.

#### What the replay guards against

The world can change while the answer is in flight, so `FinishNavigation`
checks for:

- **Focus having moved on.** `ProcessReply` navigates from the user's current
  focus, not from a path passed in, so a replay after the player clicked or
  navigated elsewhere would move focus from the wrong widget. The replay only
  happens while the widget still holds that user's focus.
- **Repeats stacking up.** A held d-pad sends the next navigation before the
  last answer arrives. At an edge every one of them fails, and replaying each
  would jump past several widgets. Only the answer to the latest navigation
  (`NavigationSequence`) can replay.
- **The widget going away.** The server callback holds a weak pointer.
- **Who and how.** The user index and the navigation genesis are replayed as
  they arrived. Slate treats controller navigation differently from keyboard
  (`GSlateEnableGamepadEditorNavigation`, for one).
- **Re-entry.** The replay calls back into `NativeOnNavigation`
  synchronously. `bReplayingNavigation` is scoped to exactly that call.

`HasFocus` is a cache, so in the frame focus arrives it can still say nothing
is focused, and a navigation then leaves at once without trying the artboard.
That costs at most one press.

## Future work

### Directional focus entry, exit and restore

The `proposed/directional-focus-api` branch proposes runtime APIs that would
replace the plugin's simpler behaviour. Its own docs live under
`packages/runtime_unreal/docs/focus_entry/` on that branch
(`README.md`, `FOCUS_RESTORE.md`).

| Today | With the proposal | Where it plugs in |
|---|---|---|
| Entering always focuses the first stop | `FocusManager::focusFrom(source, direction)`: the stop nearest the widget focus came from. The source is the previous widget's rect (`NativeOnFocusChanging` has its path), mapped into artboard space the way pointer events are. Slate doesn't say which direction moved focus, so the proposal records the last navigation key. | `URiveWidget::NativeOnFocusReceived`, beside `FRiveStateMachine::FocusFirst` |
| Returning by SetFocus or window reactivation also starts at the first stop | `FocusManager::resumeFocus()` returns to the node `suspendFocus` remembered | `URiveWidget::NativeOnFocusReceived` |
| Losing focus forgets the focused node | `FocusManager::suspendFocus()` blurs the same way, so the caret still stops, but remembers the node | `URiveWidget::NativeOnFocusLost`, in place of `ClearFocus` |
| Leaving searches from the whole widget's rect, so on a large artboard focus can leave for a neighbour level with the artboard's centre rather than with the focused item | Search from the focused item's on-screen rect with `FHittestGrid::FindNextFocusableWidget`, and return the result as an explicit `FNavigationReply` | `URiveWidget::FinishNavigation`, in place of the plain replay |

`FocusFirst` would stay as the fallback when neither `focusFrom` nor
`resumeFocus` finds a stop.
