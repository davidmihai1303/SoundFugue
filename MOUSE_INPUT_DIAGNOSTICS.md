# Intermittent mouse-click failure: diagnostic steps

Date: 18 September 2026

Status: investigation notes. The cause has not been confirmed. The logging below is proposed diagnostic work, not a fix already applied to the game.

## 1. Symptoms reported so far

- Some launches behave normally; other launches have unreliable clicks from the beginning.
- Once a run is affected, restarting sometimes produces a working run.
- Double-clicking is more likely to produce an attack than a single click.
- Isolated clicks can fail after waiting more than a second, with no movement keys held and the game window apparently focused.
- A/D movement and Space jumping work normally.
- The issue has been reported with both an external mouse and the MacBook trackpad, including physical trackpad presses.
- The visible symptom is a missing attack animation. We have not yet established whether the click is missing, the attack is rejected, or the animation is wrong.

## 2. What the code already tells us

The input path is:

```text
macOS / SFML mouse event
    -> Game::processEvents sets hasClicked
    -> World::update passes InputState to Player
    -> Player::attackingLogic calls attack()
    -> Player::attack accepts or rejects the request
    -> Player::animationLogic selects the attack animation
    -> Player::draw draws the selected sprite
```

Relevant files are `src/game/Game.cpp`, `src/game/World.cpp`, `src/game/InputState.hpp`, and `src/entities/Player.cpp`.

- Input is passed to the player before its update. The earlier input-order problem is already fixed.
- `hasClicked` is reset once at the start of event processing. A left-button press sets it to true. A release clears `clickDown`, not `hasClicked`, so a press and release in the same frame do not automatically lose the click.
- The attack cooldown is 0.5 seconds. A click during cooldown is discarded, rather than queued. This explains rapid-click failures but does not explain isolated failures after more than a second.
- Both attack clocks are explicitly reset in `Entity`'s constructor. In the installed SFML implementation, `reset()` stops a clock at zero. Comparing that stopped clock against `sf::Time::Zero` is intentional; it is not a floating-point equality problem.
- A/D and Space use `sf::Keyboard::isKeyPressed`, which can read keyboard state without window focus. Mouse attacks use window events. Working keyboard movement therefore does not establish that mouse events reach the window.
- Changing direction or jumping can cancel an attack. Reproduce initially without either action.
- The terrain resolver and ground-support query are not yet connected to player movement. Their headless tests cannot verify mouse-event delivery or attack animation.

## 3. Establish a repeatable comparison

1. Confirm the executable and working directory in the CLion run configuration. There are project copies at `Documents/2D-Platformer` and `Documents/SoundFugue`; edit and build the same copy that CLion launches.
2. If diagnostic output is added, print a unique marker once at the beginning of `Game::run`, such as `INPUT_DIAGNOSTICS enabled`, to verify that the running binary contains it.
3. Let the player land, stop moving, and leave the cursor over the game content.
4. Try five single clicks, approximately two seconds apart. Record how many attacks appear.
5. Try a double-click separately. Record whether one or both button presses appear in the event log.
6. Record the FPS shown in the title and any startup errors in the console.
7. Capture one working launch and one failing launch using the same procedure. Save the failing run's output before restarting.

Use temporary prints or debugger logpoints that do not suspend execution. Ordinary breakpoints change timing: the attack clocks continue measuring elapsed time while execution is paused. Avoid logging every frame at the current 540 FPS limit; log input events, attack decisions, and animation changes.

## 4. First check: does SFML deliver the mouse press?

In `Game::processEvents`, put this at the beginning of the existing `MouseButtonPressed` branch, before the test for `sf::Mouse::Button::Left`:

```cpp
std::cout << "[INPUT] mouse press: button="
          << static_cast<int>(button->button)
          << ", focused=" << m_window.hasFocus()
          << ", position=(" << button->position.x
          << ", " << button->position.y << ")"
          << std::endl;
```

`Game.cpp` already includes `<iostream>`. `std::endl` flushes this occasional message so buffered output is less likely to be mistaken for a missing event.

In this SFML version, left is button 0 and right is button 1. Logging before the left-button filter is important: otherwise an unexpected button value looks like no event at all.

Also consider event-specific messages for `MouseButtonReleased`, `FocusGained`, and `FocusLost`. Include the button number for releases. The existing release branch lacks braces, so add braces if inserting statements there; preserve the original release handling inside that branch.

Interpretation:

| Observation in a failing run | Next investigation |
| --- | --- |
| Startup diagnostic marker never appears | Confirm build target, executable path, and which project copy was edited. |
| Failed single click produces no press message, but a double-click does | Investigate event delivery before the attack code. |
| Press message appears with a button other than 0 | Investigate button mapping or modifiers before the game's left-button filter. |
| Press message reports focus false | Investigate startup activation/focus and compare with a working run. |
| Every failed click produces a left-button press message | Follow the event through `InputState` and the attack gate. |

`hasFocus()` reports SFML's view of focus. Record it as evidence; it does not by itself explain why a native event was or was not delivered.

## 5. If the press arrives: trace the handoff and attack decision

Add messages only when a click or attack request occurs:

1. After `m_inputState.hasClicked = true` in `Game::processEvents`: record that the left click was stored.
2. In `World::update`, when the incoming `inputState.hasClicked` is true: record that input is being forwarded before the entity update.
3. Inside `Player::attackingLogic`'s `if (m_inputState.hasClicked)`: record that the player is requesting an attack.
4. At the start of `Player::attack`: record both attack clocks' elapsed values, their running states, and `m_isAttacking`.
5. Inside the existing acceptance condition, immediately after `m_isAttacking = true`: record `ATTACK ACCEPTED`.

If using prints in `Player.cpp`, explicitly include `<iostream>` there.

The current acceptance condition is:

```cpp
m_cooldownAttackClock.getElapsedTime() == sf::Time::Zero &&
m_activeAttackClock.getElapsedTime() <= m_activeAttackTime
```

Keep the condition unchanged during the first diagnostic pass. A request message followed by no acceptance message indicates rejection at this gate. Clock readings logged before the condition are approximate snapshots; the clocks can advance between the log and the actual comparison.

After more than a second without attacking, normal updates should have reset both completed timers. If an isolated request is rejected, capture the values from that failing run before changing timer logic.

## 6. If the attack is accepted: inspect state and animation

Trace what happens after the acceptance message:

- Record when `m_isAttacking` changes back to false, with the reason: attack duration elapsed, jump, or facing-direction change.
- Verify that `Player::animationLogic` selects `m_animationToDraw = 2` while attacking.
- Verify that `Player::draw` reaches its attack-sprite branch.
- Record attack frame-index changes, `m_attacking_elapsedTime`, and the sprite texture rectangle. Avoid a print on every draw.
- Check that the attack texture loaded successfully and has nonzero dimensions. Capture any texture-loading error, the working directory, and the texture path.
- Compare FPS in good and bad runs. Attack duration uses an elapsed-time clock, while animation advances from frame `dt`, which is clamped in `Game::run`. Long frame stalls can truncate visible animation progress without implying a missing mouse event.

There is a separate animation issue worth examining only if attacks are being accepted: `attack()` restarts the active-attack clock but does not explicitly restart the animation frame and animation timer. Standing/walking logic resets those counters, but does not immediately reset the attack sprite's texture rectangle. A retrigger during an active attack can therefore continue the existing animation instead of visibly restarting it. This has not been established as the cause of the intermittent startup failure.

## 7. If mouse presses are missing: compare startup and focus behavior

Perform one experiment at a time and keep the event log running:

1. In a failing run, switch to another application and back with Command-Tab. Then test isolated clicks again. Compare focus events and `hasFocus()` before and after.
2. Compare CLion Run, CLion Debug, and launching the same executable from Terminal. Preserve the same working directory so relative resource paths do not create a separate problem.
3. Record whether modifier keys were held when the program launched. Release them before the click comparison.
4. Compare the external mouse and physical trackpad press in the same failing run. This has already been reported on both; repeat only to correlate it with the raw event log.
5. If needed, temporarily observe transitions from `sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)` alongside window events. If real-time state changes but no press event appears, the two input paths disagree. Polling can miss very short presses, so use a deliberate press and release for this experiment. Do not replace the attack input mechanism with polling as an unverified fix.
6. If the failure remains confined to event delivery, reproduce it in a separate minimal SFML program with only a window, an event loop, and mouse/focus logging. This isolates the platform/backend behavior from game state, assets, and combat logic.

Do not change SFML versions, force focus every frame, or change cooldown rules merely to see whether the symptom disappears. First identify which stage fails, then try one targeted change and compare several launches.

## 8. Related report: a lead, not a confirmed diagnosis

A [KOReader SDL3 issue](https://github.com/koreader/koreader-base/issues/2290) reports a similar macOS pattern: single clicks fail for an entire session, double-clicks get through, and restarting sometimes restores normal behavior. Its investigation points toward startup/focus handling.

SoundFugue uses SFML, not SDL. The SDL-specific `SDL_MOUSE_FOCUS_CLICKTHROUGH` workaround cannot be copied into this project, and this report does not establish an SFML defect. It supports checking raw events and focus before assuming the attack code is responsible.

SFML's [keyboard documentation](https://www.sfml-dev.org/documentation/3.1.0/namespacesf_1_1Keyboard.html) explains that real-time keyboard queries can work without window focus. Its [mouse documentation](https://www.sfml-dev.org/documentation/3.1.0/namespacesf_1_1Mouse.html) distinguishes real-time button queries from window events.

## 9. Record the result of each run

```text
Date/time:
Project copy and executable path:
Launch method and working directory:
Diagnostic startup marker present:
macOS / SFML versions:
Device:
FPS:
Working or failing from launch:
Single clicks attempted / visible attacks:
Raw left-button press messages:
Player attack-request messages:
Attack-accepted messages:
Reported focus and focus transitions:
Double-click result:
Effect of switching away and back:
Relevant timer / animation values:
Startup errors:
```

Once the failing stage is identified, fix that stage and repeat the same isolated-click procedure across several launches. Keep useful reproduction notes and remove temporary diagnostic output afterward. The collision tests should still pass, but successful GUI input requires this separate runtime check.
