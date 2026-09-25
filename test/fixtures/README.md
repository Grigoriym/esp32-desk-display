# Test fixtures

- `weather_ok.json`: real Open-Meteo response, captured 2026-09-25 13:09 with the
  exact URL from `main/weather.c` (dry: rain chance at most 10% over the 12 h).
- `air_ok.json`: real Open-Meteo air-quality response, captured 2026-09-25
  14:00 with the exact `AIR_URL` from `main/weather.c` (off season: pollen
  all ~0, only the AQI says much).
- `alerts_none.json`: real Bright Sky `/alerts` response, captured
  2026-09-25 with the exact `ALERTS_URL` from `main/weather.c`: no warnings
  out (none anywhere in Germany that day).
- `alerts_warnings.json`: **hand-written** from the Bright Sky schema
  (`api.brightsky.dev/openapi.json`) and the English DWD labels in
  dwdparse's CAP test data (`heavy rain`, `wind gusts`): two started
  warnings, an upcoming severe one and a `status: test` message. Replace
  with a real capture the next time Berlin has a warning.
- `holidays_2026.json`: real Nager.Date response for 2026/DE, captured
  2026-09-25 (19 entries, 10 apply in Berlin).
- `bvg_real.json`: real transport.rest response, captured 2026-09-25 10:29
  with the URL from `main/bvg.c` (U Cottbusser Platz, U5 towards Hbf):
  6 departures every 10 min, no delays or cancellations.
- `bvg_ok.json`: **hand-written** in the same format (the wrapper was down
  when the tests were first written); kept because it covers what the real
  capture doesn't: a delayed trip, a cancelled one (`"when": null`), a
  " (Berlin)" suffix and a trip after midnight. Replace both with VBB
  responses once roadmap task 4b switches APIs.
