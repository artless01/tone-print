# Jeff Buckley / QuadraVerb Tone Recipe

This pass is tuned around the common Jeff Buckley QuadraVerb story: a bright, dynamic guitar signal feeding an Alesis QuadraVerb-style long hall, usually discussed as a modified "Taj Mahal" preset. It is a musical approximation for Nick Toneprint, not an exact preset dump or endorsement.

## References

- Ground Guitar: [Jeff Buckley's Alesis Quadraverb](https://www.groundguitar.com/jeff-buckley-gear/jeff-buckleys-alesis-quadraverb/)
- Reverb News: [How Jeff Buckley Built His Ethereal "Grace" Guitar Sound](https://reverb.com/news/video-how-jeff-buckley-built-his-ethereal-grace-guitar-sound-potent-pairings)
- Mixdown: [Gear Rundown: Jeff Buckley](https://mixdownmag.com.au/features/rig-rundown-jeff-buckley/)

## Preset Starting Points

### Taj Hall DI

Use this for the cleanest Buckley-style direction.

- Guitar: bridge or middle single-coil, or a bright hollowbody setting.
- Front end: clean amp/DI, low gain, no heavy compression before the plugin.
- Plugin: `Taj Hall DI`.
- First tweaks: `Mix` between 0.45 and 0.60, `Verb Mix` between 0.70 and 0.85, `Decay` between 0.88 and 0.96.
- Keep `Shimmer` at 0 for the most QuadraVerb-like read.
- Raise `PreDelay` if the attack needs more air before the bloom.

### Grace Rack Bloom

Use this when the guitar should push harder and smear more.

- Guitar: single-coil bridge/middle or a slightly gritty amp edge.
- Plugin: `Grace Rack Bloom`.
- First tweaks: lower `Drive` for glass, raise `Tone` for more cut, raise `Afterimage` if the tail should hang under bends.
- Keep `Output` conservative because long reverb tails stack quickly.

## DSP Notes

- `Verb Mix`, `Decay`, `Size`, and `PreDelay` carry the rack-hall illusion.
- `Damping` is set medium-low so the tail stays bright without becoming a modern glossy shimmer.
- `Afterimage` is used lightly as the "notes overlap and blur into the next phrase" layer.
- `Drift Send` feeds the slightly moving slap path into the reverb tank for width and instability.
- `Shimmer` is mostly avoided because the target is a 90s rack hall, not a modern octave ambient verb.

## Next Iterations

- A/B against Nick playing the opening chord shapes through a clean DI.
- Add a dedicated `Rack Age` control if the tail needs more grain, noise, or early-digital darkness.
- Add optional high-cut/low-cut controls before the reverb tank for faster tone matching.
