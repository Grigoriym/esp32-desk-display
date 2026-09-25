# Test fixtures

- `weather_ok.json`: real Open-Meteo response, captured 2026-09-25 with the
  exact URL from `main/weather.c`.
- `bvg_real.json`: real transport.rest response, captured 2026-09-25 10:29
  with the URL from `main/bvg.c` (U Cottbusser Platz, U5 towards Hbf):
  6 departures every 10 min, no delays or cancellations.
- `bvg_ok.json`: **hand-written** in the same format (the wrapper was down
  when the tests were first written); kept because it covers what the real
  capture doesn't: a delayed trip, a cancelled one (`"when": null`), a
  " (Berlin)" suffix and a trip after midnight. Replace both with VBB
  responses once roadmap task 4b switches APIs.
