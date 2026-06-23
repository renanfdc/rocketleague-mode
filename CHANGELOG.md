# Changelog

Formato baseado em [Keep a Changelog](https://keepachangelog.com/).

## [0.1.0] — 2026-06-23

Primeira organização do projeto a partir do mod legado `rl-ball-mod`.

### Added
- Estrutura limpa do repositório (`docs/`, `configs/`, `assets/`, `legacy/`).
- **Trilha online-segura** como caminho principal: [docs/setup-online-seguro.md](docs/setup-online-seguro.md)
  (config nativa do RL + Filtros do Windows + saturação de driver + OSD do monitor), passo a passo por perfil.
- **Trilha offline/treino (EAC-off):** [docs/setup-offline-treino.md](docs/setup-offline-treino.md).
- Pesquisa do RL atual com fontes: [docs/pesquisa-rocket-league.md](docs/pesquisa-rocket-league.md)
  (EAC obrigatório no online desde 28/04/2026, BakkesMod non-EAC, migração para UE6).
- Guia de acessibilidade por perfil ([docs/acessibilidade-visual.md](docs/acessibilidade-visual.md)),
  performance ([docs/performance.md](docs/performance.md)) e riscos
  ([docs/riscos-e-limitacoes.md](docs/riscos-e-limitacoes.md)).
- Texturas candidatas **colorblind-safe** (amarelo/ciano sólidos) em `assets/texturas/`.
- `HANDOFF.md`, `ROADMAP.md`, `LICENSE` (MIT), `.gitignore`.
- Mod legado preservado em `legacy/rl-ball-mod/` **como código-fonte**.

### Changed
- Mod reposicionado de "funciona online" para **offline/EAC-off apenas**.
- `night_mode` default **OFF** (escurecer reduz contraste — ruim para baixa visão).
- Config unificado numa fonte única ([configs/exemplos/config.exemplo.json](configs/exemplos/config.exemplo.json));
  os 4 configs divergentes do projeto antigo foram consolidados.

### Removed (não distribuído de propósito)
- Binários não assinados (`winmm.dll`, `version.dll`, `RL-Mod-Manager.exe`) — risco de cadeia de suprimento.
- Instalador automático (`INSTALAR.bat`/`install.bat`/`uninstall.bat`) — copiava o proxy DLL sem gate de
  EAC-off (bypass empacotado).
- Automação de menu online (`auto_test`/SendInput) repudiada na documentação.
- ~70 screenshots de dev e zips do projeto antigo.

### Security / Compliance
- Removida a afirmação falsa "Funciona em Online" do `LEIA-ME.txt` antigo; documentado que o online era
  objetivo de design do código original (deriva de "CustomBallOnline") e está explicitamente repudiado.
- `high_contrast_ball.png` (verde+vermelho, ruim para daltonismo) rebaixado a histórico, com aviso.

### Pendente (ver ROADMAP)
- Validação com o usuário-alvo (P0); antes/depois visual; validação in-game das texturas.
