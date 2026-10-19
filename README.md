# D.A MAQAM ANALYZER — UI V3 source candidate

Built on the user-supplied STABLE JUCE source. Contains an implemented dark/gold JUCE editor, a selectable **reference** library filtered by tradition, Learn/Stop, pitch history, nearest equal-tempered note and cents deviation, three heuristic candidate matches, and copyable tuning *guidance*. The audio remains pass-through.

## Not implemented / not validated
- The original approved photorealistic oud artwork is **not bundled** (use a properly licensed real photograph before release).
- Complete scholarly maqam/dastgah/mugham catalog and seyir-aware classifier.
- Accurate microtonal target tuning export or direct Auto-Tune integration.
- Statistical confidence calibration, pitch-algorithm accuracy benchmarks, host QA, and Windows compilation validation.
- This is not a finished commercial release. The match score is not a probability.

## Build
Upload extracted contents to GitHub root, including hidden `.github/workflows/build.yml`; run **Build Windows VST3** under Actions. Preserve your original STABLE branch. Check compile logs and test in Studio One. No build has been run in this environment.
