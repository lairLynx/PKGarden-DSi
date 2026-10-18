#pragma once

// Startup splash: the lairLynx Studios logo and credit on the top screen, the
// SDK credit on the bottom screen. Same look as the other lairLynx Studios apps
// (copied from NDSi-Shop / StarDSi).

#ifdef __cplusplus
extern "C" {
#endif

// Draws the splash and fades it in. Call first, before anything else touches the video.
void splashBegin(void);

// Holds the splash until it has been up for a moment (any key or a touch skips
// ahead), then fades it out to black. The screens stay black afterwards: set up your
// own graphics, then call screensFadeIn().
void splashEnd(void);

// Fades both screens from black to normal brightness.
void screensFadeIn(void);

#ifdef __cplusplus
}
#endif
