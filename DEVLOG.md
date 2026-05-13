# Lumen — Dev Log
**Project:** Lumen (tangible object for negotiated screen-work interruption)
**Repo:** github.com/fammad/lumen
**Author:** Fuad Mammadov, KTH IMT MSc

---

## April 5 — Concept

Started thinking about Lumen as a physical object. Brainstormed what the interaction should feel like, what problem it was actually solving, and whether it was worth building at all. No code yet.

## April 10 — Logic and structure

Started investigating the detection logic. Imported variables, mapped out what the system needed to track across frames. Still no working code — mostly understanding what MediaPipe gives you and what you have to compute yourself.

## April 15 — Environment, repo, first working detection

Set up the venv environment, created the GitHub repo, added dependencies. Python 3.12 pinned, mediapipe fixed at 0.10.14 because 3.13 breaks the `mp.solutions` API.

Built `test_facemesh.py` first to confirm MediaPipe face mesh was actually running before touching eye-specific logic. Once that worked, moved to `eye_landmarks.py` — isolated the 6 landmark indices per eye and drew them on the webcam feed. Then wrote `blink_detector.py` with the EAR calculation. By end of day, EAR was printing live to screen.

Hardware is still pending. Everything so far is software only.

## April 21 — Blink counter

Added blink event detection on top of the EAR values. Took three iterations.

First version triggered on average EAR across both eyes. Head turns caused false counts because the far eye's landmarks compress at angle and EAR drops without a real blink.

Second version required both eyes to be below threshold simultaneously. Fixed head-turn artifacts and filtered winks.

Third version added a minimum open duration — 3 consecutive above-threshold frames required before a reopen is confirmed. Squinting was oscillating below and above threshold repeatedly and counting each recovery as a blink. This stopped it.

Result: 10 deliberate blinks, count showed 10. Open-eye EAR baseline is 0.33 in evening home lighting. Threshold is set at 0.21. That gap will likely need re-tuning under conference fluorescents in Vaasa.

ESP32-S3 not yet ordered. No hardware built.

## April 22 - Blink Rate per 60 second.

Added blink counting functionality to the code that can be used to count how many blink occured in 60 seconds.

First created new constants(how many second has to count) and variables (dequeu four double ended queue). The code append every time it sees new blink. If the blink is out of given WINDOW_SECOND it drops thhe frist blink_times[0] one and add this one if this code true while blink_times and now - blink_times[0] > WINDOW_SECONDS

Result: 2 minute user test to count manual eye blink with the system. It was accurate. But I realized my blink rate is around 5-9 per minute which is not typical.

## April 23 — Range and lighting test

Tested blink detection accuracy across distance and lighting conditions.

MacBook Pro built-in camera, 1920×1080 at 60 FPS.

Low light: ~99% accurate up to 120 cm. Beyond that, landmark confidence 
drops and EAR values become unstable.

Normal / daylight: ~99% accurate up to 150 cm. Same degradation pattern 
past that threshold — not a gradual drop, more like a cliff.

This matters for the NordiCHI demo: attendees will be sitting ~60–80 cm 
from the screen. Both lighting conditions cover that range comfortably. 
The threshold cliff past 150 cm is not a concern for the demo setup, but 
I'll note the range limit in the abstract's limitations paragraph.

EAR threshold is still 0.21. Haven't re-tuned for conference fluorescents 
yet — that's the Sprint 4 lighting test.


## April 28 — Rolling baseline engine

Built `BaselineEngine` class - owns "what's the healthy blink rate reference for this user right now?" Returns reference, personal_baseline, floor_active, current_rate, samples_collected via get_state().

Hybrid design: deque pre-filled with PRIOR=15 (Rosenfield healthy at-rest rate) so the baseline is meaningful from sample 1, no warm-up dead time. Reference = max(personal_baseline, FLOOR) where FLOOR=7 (Rosenfield strain threshold). The floor catches drift — without it, sustained strain pulls the rolling mean down with the user, and the system silently agrees with the deteriorating state instead of pushing back.

Considered session-scale blending of calibration baseline with rolling rate. Rejected — any weighted average with a low recent rate pulls reference down, reintroducing the drift bug. Escalation over time belongs to the focus-clock in Task 2.2, not the baseline.

Smoke test (3 scenarios):
- Fresh start: reference=15.0, samples_collected=0 (deque full of priors)
- 30 healthy samples (~15): reference=15.0, floor_active=False
- 30 strained samples (~5): reference=7, personal_baseline=4.9, floor_active=True ← floor catching drift

What I learned: deque(iterable, maxlen=N) is the right structure for FIFO rolling windows. Pre-filling with a literature prior is Bayesian-style initialization. Always trace mechanically (sum=147, /30=4.9) before reaching for narrative ("the system decided"). And \n is a newline; /n is two characters.


## April 29 — Risk Scoring and Tweak

Some ideas for risk score:
20 minutes is when time-pressure alone maxes out its contribution to the risk score. After 20 minutes of continuous focus, the clock has nothing more to say — but the clock alone never crosses the BREAK threshold. Strain in the blink rate must also be present. This separates duration from strain: long focus is concerning, but only concerning enough to escalate the dome when accompanied by physiological evidence.



## April 30 Risk scoring engine

Built `RiskEngine` class in `risk_engine.py` — owns "how concerned 
should the dome be right now?" Consumes baseline state dict from 
BaselineEngine. Single stored attribute: `last_break_time`. 
Everything else computed fresh per call.

Formula:
- blink_risk = max(0, (reference - current_rate) / reference)
- focus_risk = min(1.0, minutes_without_break / 20)
- risk_score = 0.5 * blink_risk + 0.5 * focus_risk
- CALM < 0.3 / ATTENTION < 0.6 / BREAK ≥ 0.6

Key design decisions:
- FOCUS_SATURATE_MIN = 20: motivated by AOA 20-20-20 rule and 
  work-block research (Cirillo 2006, Mark 2008). Tunable constant,
  not a derived value.
- Focus alone caps at 0.5 risk (0.5 weight). Cannot trigger BREAK 
  without blink strain also present. Time is a concern, not a sentence.
- mark_break() resets focus clock only. Blink strain persists across 
  breaks — intentional. A break addresses time, not physiology.
- Time injected as parameter (not time.time() internally) — 
  enables testing without waiting real seconds.

Smoke test (5 scenarios):
- Fresh healthy: score=0.0, CALM ✓
- Strained 5min: score=0.458, ATTENTION ✓  
- Healthy 60min: score=0.5, ATTENTION ✓ (clock alone never triggers BREAK)
- Strained 30min: score=0.833, BREAK ✓
- After mark_break: focus_risk=0.0, still ATTENTION (blink strain 
  persists) ✓

Considered resetting baseline after breaks by injecting healthy 
values into deque. Rejected — same drift bug from opposite direction. 
If user's blink rate is genuinely 5/min post-break, showing CALM 
would be dishonest. Baseline reflects reality; it doesn't reset 
to optimism.



## May 1 - State machine and mock visualizer

Added state.py with DomeState enum: CALM, ATTENTION, BREAK, BREATHING, PAUSED. 
Single source of truth for dome states — every downstream module will import 
from here instead of comparing strings. Pre-filled integers 1–5, standard 
library Enum, no dependencies.

Built mock_visualizer.py — tkinter window with animated circle driven by 
real-clock sine wave. Brightness and cycle duration defined per state in 
three dicts (STATE_COLORS, STATE_CYCLE, STATE_BRIGHTNESS). Keyboard bindings 
1–5 for manual state switching. Two bugs found and fixed: tkinter not 
bundled with Homebrew Python 3.12 (fixed with brew install python-tk@3.12), 
and three methods accidentally dedented out of MockVisualizer class 
(indentation fix). 

Initial color palette was too saturated for daylight use — conference 
fluorescents and desk ambient light won't produce the dark-room contrast 
the original values assumed. Retuned to warm yellowish-white for CALM, 
clear orange for ATTENTION, deep red for BREAK, bright green for BREATHING. 
Brightness floors raised across all states so colors remain readable in 
ambient light. BREAK amplitude compressed (0.25–0.65) to preserve the 
withdrawal behavior against McFarlane framing.


## May 2 — F1 validation

Ran blink detection accuracy test across 6 conditions: three distances 
(1m, 1.5m, 2m) in direct sunlight, one movement stress test at 1m, 
and two distances under table lamp only.

Results: F1 ≥ 0.90 for all conditions within the demo operating envelope 
(≤1.5m). Sharp degradation at 2m in low light (F1=0.57) — landmark 
confidence drops below reliable tracking threshold at that distance. 
No re-tune needed; EAR threshold stays at 0.21.

Demo setup uses 60–80cm distance. All tested conditions at that range 
perform at F1 ≥ 0.92. Head-turn stress test at 1m returned F1=0.92; 
some false positives from landmark compression may be present in that 
figure but were not counted separately.

Limitation documented in README. Will report F1=0.92 (head-turn condition, 
most conservative passing result) as the headline validation figure in the 
abstract — honest and adversarial rather than cherry-picking the 1.0 result.