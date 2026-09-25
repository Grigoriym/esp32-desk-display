# Test fixtures

- `weather_ok.json`: real Open-Meteo response, captured 2026-09-25 with the
  exact URL from `main/weather.c`.
- `bvg_ok.json`: **hand-written** in the transport.rest / hafas-client
  departures format (both wrapper mirrors were down when the tests were
  written). Covers a delayed trip, a cancelled one (`"when": null`), a
  " (Berlin)" suffix and a trip after midnight. Replace with a real capture
  (URL from `main/bvg.c`, `bvg_secrets.h` IDs) when the wrapper answers, or
  with VBB responses once roadmap task 4b switches APIs.
