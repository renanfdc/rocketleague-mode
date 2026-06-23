# Performance — custo de cada trilha

Acessibilidade não pode custar a jogabilidade. Para uma pessoa PCD, FPS estável e baixo input lag fazem
parte de conseguir jogar. Esta tabela classifica cada opção pelo custo, além do valor de acessibilidade.

> Regra geral: **o que age fora do processo do jogo (driver, monitor) custa quase nada de FPS**; o que
> compõe imagem no Windows (filtros/lupa) custa um pouco; o que **injeta** no jogo (mod, ReShade) custa
> frametime e pode dar stutter/crash.

## Trilha ONLINE-segura

| Opção | Custo de FPS / input lag | Nota |
|---|---|---|
| **Preset visual nativo** (desligar Bloom/Light Shafts/Shadows/Motion Blur/Weather + Render Detail Performance) | **Negativo — AUMENTA FPS** | Única trilha que **melhora** performance. Priorize em hardware fraco. |
| Color Blind Mode / High Contrast Nameplates / Ball Arrow | Zero | Lógica nativa, sem custo gráfico. |
| FOV / Distance / UI Scale | Zero | Mudança de câmera/HUD. |
| HDR Paper White / Contrast | Zero | Só com display HDR. |
| **NVIDIA Digital Vibrance / AMD Custom Color / Intel GCC** | ~Zero | Cor na saída do driver. Sem hook no jogo. |
| **OSD do monitor** (gamma/black equalizer/saturação) | Zero | No hardware, fora do PC. |
| **Filtros de Cor do Windows 11** | **Baixo-médio** | Composição no DWM (Borderless) + ~1 frame de latência de composição vs fullscreen exclusivo. Perceptível em iGPU/laptop. |
| **Lupa/Magnifier do Windows** | **Médio-alto** | Re-amostragem da tela inteira via DWM. Pode derrubar FPS em hardware modesto. Use só quando precisar. |
| Rodar em **Borderless** (pré-requisito dos filtros) | **Baixo** | Perde fullscreen exclusivo: +~1 frame de latência de apresentação do DWM, possível leve queda de FPS em GPU limitada. |

> **Trade-off do Borderless:** ele é necessário para filtros do Windows e cor do driver pegarem, mas custa
> um pouco de latência. Quem usa **só** Color Blind nativo + OSD do monitor pode ficar em **Fullscreen** e
> economizar esse frame.

## Trilha OFFLINE / treino (mod injetado)

O mod do repo (`legacy/rl-ball-mod/`) tem custo **alto** e problemas de performance reais (achados na
auditoria do código), mesmo offline:

| Problema | Onde | Efeito |
|---|---|---|
| Hook de `Present` + `PSSetShaderResources` por frame (vtable D3D11) | `tex_hook.cpp` | Custo de frametime em todo frame e em cada bind de textura. |
| `g_srvCache.clear()` a cada 3600 frames (~60s) | `tex_hook.cpp:692` | **Micro-stutter cíclico a cada ~1 min** (toda textura volta ao caminho lento). |
| `g_logMode = true` por padrão (modo descoberta) | `tex_hook.cpp:35` | `fprintf` síncrono por SRV nos primeiros ~300 frames → **engasgo pesado no load**. |
| Varredura de `GObjects` (100k+) na game thread | `ball_hooks.cpp` | **Hitch de vários ms** ao spawnar a bola / trocar de mapa. |
| `NightMode::Render` por frame mesmo "desabilitado" | `tex_hook.cpp:704` | Restaura só RTV/DSV (não blend/shaders/topology) → pode **corromper o render** do jogo. |
| Compressor BC1 caseiro + 3-4 criações de night-sky que **vazam VRAM** | `tex_hook.cpp` | Pico de CPU e **leak de VRAM** no init. |
| Match de textura só por dimensão/formato | `tex_hook.cpp` | Troca **qualquer** textura 640x640 BC1 (não só a bola) → binds redundantes. |

**Compatibilidade entre máquinas:** offsets/slots presos a uma build (vtable D3D11 [8], ProcessEvent slot
67, RLSDK Season 22 v2.67). Em GPU/driver/resolução diferentes ou build nova do RL → **CRASH** do processo,
não degradação suave. O fallback de pattern-scan varre `0x10000..0x7FFFFFFFFFFF` e **congela** o init por segundos.

**Se for usar mesmo assim (offline):** desligar `g_logMode` (→ `false`), aumentar muito o intervalo do
`g_srvCache.clear()` (ou invalidação seletiva), e remover a chamada de `NightMode::Render` por frame. Ver
pendências em [`../legacy/rl-ball-mod/README.md`](../legacy/rl-ball-mod/README.md).

## Como medir

- **FPS / frametime:** overlay do Steam, `Ctrl+Shift+R` (RL tem display de perf nativo em algumas versões),
  ou PresentMon / CapFrameX (mede o 1% low, que é o que dói no stutter).
- **Input lag:** comparar Fullscreen exclusivo vs Borderless com o mesmo cenário; sentir/medir com câmera de
  alta taxa se for rigoroso.
- **Critério:** uma opção de acessibilidade que derrube o **1% low** de forma perceptível não vale para
  hardware fraco — prefira a alternativa de custo zero (driver/monitor) que entrega o mesmo efeito de cor.
