# Rocket League — Acessibilidade Visual

Recursos e guias para uma pessoa com **deficiência visual** (baixa visão, daltonismo, sensibilidade a
movimento) jogar **Rocket League** com mais conforto e legibilidade — enxergar melhor a **bola**, os carros
e o campo.

> **Isto é acessibilidade, não vantagem.** Nada aqui automatiza jogadas, altera física ou rede, nem é
> trapaça. O caminho principal **não toca no jogo** — são ajustes do próprio Rocket League, do driver de
> vídeo, do Windows e do monitor. **Seguro com o anti-cheat, zero risco de banimento.**

## Comece aqui (trilha ONLINE-segura)

A maior parte da necessidade se resolve **sem instalar nada**, e isso vale em **ranqueada/casual**:

1. **No jogo:** ligue **Color Blind Mode** + **High Contrast Nameplates**; desligue efeitos de fundo
   (Bloom, Light Shafts, Motion Blur, Weather, sombras) — *isso ainda aumenta o FPS*.
2. **Windows 11:** **Filtros de Cor** (`Win+Ctrl+C`) para daltonismo; **Lupa** para ampliar.
3. **Driver de vídeo:** aumentar **saturação** (NVIDIA Digital Vibrance / AMD Custom Color / Intel GCC).
4. **Monitor:** **gamma 2.0-2.2** + **Black Equalizer** no OSD.

> **Pré-requisito** dos passos 2-3: rodar o jogo em **Borderless/Windowed** (em fullscreen exclusivo os
> filtros do Windows e a cor do driver são ignorados — é o erro nº 1).

**→ Passo a passo completo, com a ordem por perfil:** [docs/setup-online-seguro.md](docs/setup-online-seguro.md)

## Documentação

| Doc | Conteúdo |
|---|---|
| [docs/contexto.md](docs/contexto.md) | O que é o projeto, para quem, e o histórico. |
| [docs/setup-online-seguro.md](docs/setup-online-seguro.md) | **Trilha principal**: passo a passo seguro (jogo + Windows + GPU + monitor). |
| [docs/acessibilidade-visual.md](docs/acessibilidade-visual.md) | Guia **por perfil** (daltonismo / baixa visão / fotofobia) e princípios. |
| [docs/setup-offline-treino.md](docs/setup-offline-treino.md) | Trilha **offline (EAC-off)**: BakkesMod e mod de contraste para treino. |
| [docs/pesquisa-rocket-league.md](docs/pesquisa-rocket-league.md) | O que mudou no RL (EAC, BakkesMod, UE6) — com fontes. |
| [docs/performance.md](docs/performance.md) | Custo de FPS/input-lag de cada opção. |
| [docs/riscos-e-limitacoes.md](docs/riscos-e-limitacoes.md) | EAC, política de ban, limites honestos. |
| [HANDOFF.md](HANDOFF.md) | Estado completo para continuar o projeto. |
| [ROADMAP.md](ROADMAP.md) | Próximos passos priorizados. |

## O que mudou no Rocket League (resumo)

Desde **28/04/2026** o **Easy Anti-Cheat (EAC)** é obrigatório no online de PC. O jogo se partiu em dois:

- **EAC-on (online):** ranqueada/casual/torneios — **sem mods**. Aqui só vale a trilha segura (nativo + GPU
  + Windows + monitor).
- **EAC-off (offline):** free play/treino/LAN/replay — **mods rodam**. Aqui dá para usar contraste extra
  para **treinar a leitura da bola**.

O **BakkesMod** foi descontinuado e revivido com apoio da Psyonix, **só em modo non-EAC**. Detalhes e
fontes em [docs/pesquisa-rocket-league.md](docs/pesquisa-rocket-league.md).

## Treino offline (opcional)

Para calibrar contraste/textura da bola **offline**, ver [docs/setup-offline-treino.md](docs/setup-offline-treino.md).
Texturas candidatas **colorblind-safe** em [assets/texturas/](assets/texturas/).

## Mod legado (arquivado)

O projeto começou como um mod C++ (injeção de DLL) que troca a textura da bola e escurece o céu. Está
preservado em [legacy/rl-ball-mod/](legacy/rl-ball-mod/) **apenas como código-fonte**, **sem binários e sem
instalador**, e **só roda offline (EAC-off)** — ver o aviso lá. Não tem caminho seguro para o online.

## Aviso

Mexer com mods que injetam no jogo (incluindo o mod legado) **só no modo sem EAC (offline)**. **Nunca**
tente usar no online — burlar o EAC pode **banir a conta**. Os ajustes da trilha principal (jogo/Windows/
GPU/monitor) **não injetam** e são seguros. Use por sua conta e risco.

## Licença

[MIT](LICENSE).
