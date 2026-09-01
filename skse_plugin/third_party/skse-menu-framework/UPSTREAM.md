# SKSE Menu Framework consumer API

`SKSEMenuFramework.h` and `LICENSE` are vendored verbatim from the official
[`SKSE-Menu-Framework-3-API`](https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API)
consumer repository at commit `1dcb70179076aae4ab626f43c5baab2735ca5877`.

- `SKSEMenuFramework.h` SHA-256:
  `48416E8220CA777E2FFFC2EF2BAF21F699AB2E6C409D437F44EEC5E311C3524C`
- `LICENSE` is LGPL-2.1 and has SHA-256:
  `7FFE1954587C77DFBA1CF8EB9B2EA743671FA6E63F9E7A2F258119D42E14EEFE`

The consumer header is byte-identical to `resources/SKSEMenuFramework.h` in
the official runtime repository,
[`SKSE-Menu-Framework-3`](https://github.com/QTR-Modding/SKSE-Menu-Framework-3),
at commit `928e01ab459822a8d233ab99f0419ea1de23c775`.

Spell Hotbar 2 includes the header but does not link or redistribute Menu
Framework. Calls resolve dynamically from the separately installed runtime at
`Data/SKSE/Plugins/SKSEMenuFramework.dll`.

SMF discovers consumer fonts only under `Data/SKSE/Plugins/Fonts`. Optional
FOMOD TTFs still install there so the host can discover them. This guest does
not call `SKSEMenuFramework::PushFont`; HUD text uses the host default ImGui
face. The private `ImFont*` pointers stay null.
