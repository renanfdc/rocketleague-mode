# Presets visuais (valores de referência)

Valores prontos para copiar. Os presets **online-seguros** (jogo + driver + monitor) não dependem de mod e
valem em ranqueada. O preset do mod legado é **só offline**. Passo a passo em
[../../docs/setup-online-seguro.md](../../docs/setup-online-seguro.md).

## Preset NATIVO do Rocket League — "limpeza + contraste" (online-seguro)

`Settings > Gameplay`
- Color Blind Mode: **On**
- High Contrast Nameplates: **On**
- Ball Cam Indicator: **On** · Ball Arrow: **On**

`Settings > Camera`
- Camera Shake: **Off**
- FOV: **110** (mais campo) ou **90** (elementos maiores) — ao conforto
- Distance: **270-300** (menor = bola maior) · Height: **110** · Angle: **-4** · Stiffness: **0.55**
- UI Scale: **0.8-1.0** (HUD maior)

`Settings > Video > Advanced`
- Render Quality: **High Quality** · Render Detail: **Performance** (ou Custom)
- Bloom: **Off** · Light Shafts: **Off** · Dynamic Shadows: **Off** · Motion Blur: **Off**
- Weather Effects: **Off** · Ambient Occlusion: **Off** · Depth of Field: **Off**
- Window Mode: **Borderless** (pré-requisito de filtros Windows/driver)

`Settings > Video` (só monitor HDR): HDR Paper White e HDR Contrast ao conforto.

## Preset de DRIVER (online-seguro) — escolha a sua GPU

- **NVIDIA:** Dynamic Vibrance / Vibração Digital **55-75%**.
- **AMD:** Adrenalin > Custom Color: Saturação **+10 a +25%**; + Radeon Image Sharpening **On**.
- **Intel:** GCC > Color: Saturação/Contraste levemente acima do padrão (perfil global).

## Preset de MONITOR (OSD, online-seguro)

- Modo de imagem: **Custom/User/Standard**
- Gamma: **2.0-2.2** · Black Equalizer / Shadow Boost: **médio (~10-15)**
- Contraste/Saturação: levemente acima do padrão · Low Blue Light: opcional (só um caminho, não com Night Light)

## Preset de Filtro do Windows 11 (online-seguro)

- `Win+Ctrl+C` para alternar · `Configurações > Acessibilidade > Filtros de cor`
- Deuteranopia / Protanopia / Tritanopia conforme a condição.

## Preset do MOD LEGADO (OFFLINE / EAC-OFF apenas)

Arquivo de exemplo: [../exemplos/config.exemplo.json](../exemplos/config.exemplo.json)

| Chave | Valor recomendado | Por quê |
|---|---|---|
| `enabled` | `true` | liga o mod |
| `texture_file` | `bola-amarelo-solido.png` | colorblind-safe (ver assets/texturas) |
| `night_mode` | **`false`** | escurecer **reduz** contraste → ruim p/ baixa visão |
| `apply_to_dissolve` | `true` | aplica a textura também no efeito de "dissolve" da bola (chave real lida por `config.cpp`) |
| `auto_test` | **`false`** | navegação automática de menu (incl. online) — manter desligado |
| `log_level` | `info` | — |

> A textura citada precisa estar em `BallMod/textures/` na pasta do jogo. **Nunca** rodar o mod com o EAC
> ligado — ver [../../docs/riscos-e-limitacoes.md](../../docs/riscos-e-limitacoes.md).
