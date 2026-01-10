# Changelog

## 0.1.2
- Added f-strings with `f"..."` interpolation and `{{`/`}}` escapes.
- API router logging is configurable via `router({ logging: true })`; middleware uses `@Use`.
- Server responses JSON-serialize object/list bodies (including response `body`).
