# ROADMAP

Prioridades por necessidade real do usuário-alvo (PCD baixa visão), não por sofisticação técnica.

## P0 — validar com o usuário

- [ ] Rodar [docs/setup-online-seguro.md](docs/setup-online-seguro.md) **com a pessoa PCD**, na ordem do
      perfil dela. Anotar o que de fato ajudou (e o que não) no [CHANGELOG.md](CHANGELOG.md). Esta é a
      entrega que mais importa — tudo o mais é suporte a ela.

## P1 — tornar o guia inquestionável

- [ ] **Antes/depois visual** no README (1-2 imagens: bola/campo sem ajuste vs com Color Blind Mode +
      saturação). É o que falta para alguém julgar se vale.
- [ ] Validar **in-game (EAC-off)** as texturas candidatas (`assets/texturas/` amarelo/ciano) e registrar
      qual ajuda mais; trocar a recomendação por uma textura comprovada.
- [ ] Guia rápido de **1 página** ("cartão") por perfil para imprimir/compartilhar com não técnicos.

## P2 — ampliar o alcance da acessibilidade

- [ ] Filtros de cor de **macOS** e **SteamOS/Steam Deck** (mesmo princípio "fora do jogo = seguro").
- [ ] **Áudio posicional** do RL como pista não-visual (baixa visão severa): documentar e treinar.
- [ ] Enquadrar Camera Shake/Motion Blur Off como acessibilidade **fotossensível/vestibular** (perfil distinto).
- [ ] **HUD/legendas:** UI Scale, Safe Zone, tamanho/cor de texto de chat para baixa visão.

## P3 — mod legado (só se houver demanda; tende a quebrar no UE6)

- [ ] Confirmar se a **troca de textura da bola funciona offline** (hoje não comprovada).
- [ ] Performance: desligar `g_logMode`, corrigir o `g_srvCache.clear()` cíclico, remover `NightMode::Render`
      morto (ou restaurar todo o pipeline state). Ver [docs/performance.md](docs/performance.md).
- [ ] **Build reproduzível** (CMake ou `g++` do PATH) — hoje `build.bat` assume o WinLibs do WinGet num caminho fixo.
- [ ] `night_mode` permanente OFF; remover automação de menu do código.

## Fora de escopo / descartado

- Usar mod injetado **online** — risco de ban; não há caminho seguro.
- ReShade / NVIDIA Freestyle como solução de cor — preferir Digital Vibrance (sem injeção).
- Realce de cosmético (Ball Trail/boost) como acessibilidade — não é ajuste de visibilidade.
- Distribuir binários/instalador — risco de cadeia de suprimento.

## Horizonte

- **UE6 (~2028):** a migração de engine tende a quebrar mods de injeção UE3. A trilha nativa + camadas
  externas é a que sobrevive — reforça priorizar P0-P2 sobre P3.
