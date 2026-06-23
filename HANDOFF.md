# HANDOFF — Rocket League Acessibilidade Visual

Documento auto-suficiente para outra sessão/pessoa continuar **sem redescobrir tudo**. Data: jun/2026.

## 1. Resumo do projeto

Recursos e guias para uma pessoa PCD com limitação visual jogar Rocket League com mais legibilidade
(enxergar bola/carros/campo). Entrega principal = **documentação de ajustes seguros** (jogo + Windows +
GPU + monitor). Entrega secundária = **trilha de treino offline** (mods de contraste). O mod C++ original
(injeção de DLL) foi **arquivado** como código-fonte, sem binários.

## 2. Objetivo de acessibilidade

Reduzir barreira visual — **acesso, não vantagem**. Contraste, legibilidade, menos poluição visual, baixo
impacto em FPS, segurança da conta, instruções simples e linguagem não capacitista.

## 3. Usuário-alvo

Pessoa PCD (baixa visão / daltonismo / cegueira parcial) — origem: um amigo do mantenedor. PC/Windows, RL
via Steam ou Epic. Pode não ser técnico → guias passo a passo. A **conta não pode correr risco de ban**.

## 4. O que já foi feito (no passado)

- Mod **RL Ball Mod** (pasta local original do mod, fora deste repositório, sem git): proxy `winmm.dll` + MinHook em
  `ProcessEvent` (UE3) + escrita em memória + hook D3D11. Trocava textura da bola e escurecia o céu. Tinha
  instalador `.bat`, um Mod Manager (Python/Tkinter), 2 mockups HTML e um `RL-Mod-Manager.exe` (PyInstaller).
- Texturas: `gg_lag_team` (skin troll, = `ball_custom`), `joaninha` (foto pessoal), `high_contrast_ball`
  (verde+vermelho — **ruim para daltonismo**).

## 5. Onde o projeto estava parado

- Troca de textura da bola **não comprovada** (caminho D3D11 com PNG vazio; caminho UE3 estado desconhecido).
- Offsets presos à build **Season 22 v2.67** → quebram a cada update.
- `LEIA-ME.txt` anunciava "Funciona em Online" (**falso/perigoso** pós-EAC).
- Sem git, sem README da trilha segura, repo poluído (binários duplicados, ~70 screenshots, zips).

## 6. Quais arquivos existem (neste repo novo)

```
rocket-league-accessibility/
├─ README.md                 # trilha segura como caminho principal
├─ HANDOFF.md ROADMAP.md CHANGELOG.md LICENSE .gitignore
├─ docs/
│  ├─ contexto.md  pesquisa-rocket-league.md  acessibilidade-visual.md
│  ├─ setup-online-seguro.md  setup-offline-treino.md  performance.md  riscos-e-limitacoes.md
├─ configs/
│  ├─ presets-visuais/README.md          # valores prontos (jogo/GPU/monitor/Windows)
│  └─ exemplos/config.exemplo.json        # config unificado do mod legado (offline)
├─ assets/texturas/                        # candidatas colorblind-safe (amarelo/ciano) + README
└─ legacy/rl-ball-mod/                      # SÓ código-fonte do mod antigo (sem binários/instalador)
   ├─ src/ lib/ build.bat config/ mod-manager/ texturas-historicas/ README.md
```

Fonte original (com binários, screenshots, instalador) permanece na pasta local do mod, fora deste
repositório — **não** foi migrada inteira de propósito.

## 7. Decisões técnicas tomadas

- **Trilha segura (online) = fora do processo do jogo** (nativo + driver + Windows + monitor). É a entrega
  principal e cobre a maioria da necessidade. Zero risco de EAC.
- **Mod injetado = só offline (EAC-off)**, código-fonte apenas, sem binários, sem instalador, sem automação
  de menu online.
- **Não shippar** `winmm.dll`/`.exe`/`INSTALAR.bat` (risco de cadeia de suprimento + bypass empacotado).
- **Texturas:** candidatas colorblind-safe sólidas (amarelo/ciano), por construção; históricas só com aviso.
- **`night_mode` default OFF** (escurecer reduz contraste). Config unificado numa fonte única.

## 8. Pesquisas externas feitas (com fontes)

Fan-out de 6 frentes + verificação adversarial (detalhe e links em
[docs/pesquisa-rocket-league.md](docs/pesquisa-rocket-league.md)): atualizações do RL, EAC, BakkesMod,
acessibilidade nativa, GPU/SO, precedente da comunidade.

## 9-10. O que mudou no RL e impacto

- **EAC obrigatório no online desde 28/04/2026** (Season 22). Split EAC-on (online, sem mods) / EAC-off
  (offline, mods rodam). Mod **só existe offline**.
- **BakkesMod** descontinuado e revivido com apoio Psyonix, **só non-EAC**.
- **Psyonix internaliza features de mod** (hitbox viz, MMR, treino, team colors).
- **Engine → UE6** (não UE5), deploy ~2028: tende a quebrar mods de injeção UE3.

## 11. O que é seguro para ONLINE

Color Blind Mode + High Contrast Nameplates (nativo); preset visual de limpeza; FOV/Distance/UI Scale/Ball
Arrow; HDR Paper White/Contrast; NVIDIA Digital Vibrance / AMD Custom Color+RIS / Intel GCC; Filtros de Cor
+ Lupa do Windows 11; OSD do monitor. **Pré-requisito: Borderless.**

## 12. O que é só OFFLINE/treino

BakkesMod (plugin Color Blind Color Changer / texture mods); mod legado deste repo; ReShade (preferir
Digital Vibrance). NVIDIA Freestyle: só validar offline (inconsistente com EAC).

## 13. Riscos conhecidos

Ban só se **burlar o EAC** para usar mod online (propaga entre plataformas Epic). Com EAC on o mod é
bloqueado (não injeta), não banido na hora. Offline = sem risco. Binário não assinado = cadeia de
suprimento. Offsets quebram a cada update → crash. Detalhe em
[docs/riscos-e-limitacoes.md](docs/riscos-e-limitacoes.md).

## 14. Pendências (priorizadas)

1. **[P0] Validar a trilha segura com o usuário-alvo** (testar os passos com a pessoa PCD; coletar quais
   ajustes realmente ajudam) — é a entrega que mais importa.
2. **[P1] Antes/depois visual** (1-2 screenshots de contraste com Color Blind Mode + saturação) para o README.
3. **[P1] Validar in-game** as texturas candidatas (amarelo/ciano) no modo EAC-off; medir se ajudam.
4. **[P2] Ampliar acessibilidade:** filtros de cor macOS/SteamOS; áudio posicional como pista; HUD/legendas.
5. **[P2] Mod legado (se for mexer):** desligar `g_logMode`, corrigir stutter cíclico, remover render morto,
   confirmar se a troca de bola funciona offline. Só se houver demanda real — tende a quebrar no UE6.
6. **[P3] Build reproduzível** do legado (CMake / g++ do PATH) — hoje o `build.bat` assume o WinLibs do WinGet num caminho fixo.

## 15. Próximos passos exatos

1. Abrir o PR no repositório do Felipe (ver §"Como subir" no fim).
2. Sentar com o usuário-alvo e rodar [docs/setup-online-seguro.md](docs/setup-online-seguro.md) na ordem do
   perfil dele; anotar o que funcionou no [CHANGELOG.md](CHANGELOG.md).
3. Tirar o antes/depois e colocar no README.

## 16. Como rodar/testar

- **Trilha segura:** não tem build. Seguir [docs/setup-online-seguro.md](docs/setup-online-seguro.md). Teste
  = a pessoa enxerga melhor a bola/campo, com FPS estável, em ranqueada/treino.
- **Mod legado (offline):** ver [legacy/rl-ball-mod/README.md](legacy/rl-ball-mod/README.md); requer MinGW,
  ajustar `build.bat`. **Só EAC-off.**

## 17. Como medir performance

PresentMon/CapFrameX (1% low), overlay do Steam. Critério: opção que derrube o 1% low de forma perceptível
não vale em hardware fraco — preferir alternativa de custo zero. Ver [docs/performance.md](docs/performance.md).

## 18. Critérios de aceite

- [x] Projeto antigo localizado e mapeado.
- [x] Estrutura limpa do repo criada.
- [x] README útil, trilha segura como caminho principal.
- [x] HANDOFF completo.
- [x] Pesquisa do RL atual documentada com fontes.
- [x] Online vs offline separados e classificados.
- [x] Objetivo de acessibilidade explícito; linguagem não capacitista.
- [x] Alterações antigas registradas; limitações honestas.
- [x] Pendências priorizadas + próximos passos.
- [ ] **Validação com o usuário-alvo real** (P0 — depende de sessão presencial).
- [ ] Antes/depois visual no README.

## 19. Checklist para a próxima sessão

- [ ] PR aberto e revisado pelo Felipe.
- [ ] Passos da trilha segura testados com a pessoa PCD; ajustes que ajudaram anotados no CHANGELOG.
- [ ] 1-2 imagens antes/depois no README.
- [ ] (Se houver demanda) texturas candidatas validadas in-game (EAC-off).
- [ ] (Opcional) ampliar para macOS/SteamOS e áudio posicional.

---

### Como subir (estado atual)

GitHub logado como `renanfdc`. Repo destino pretendido: conta do Felipe (`fmodesto30`). Restrição: colaborador
não cria repo novo na conta de outro usuário — só o Felipe cria `fmodesto30/<nome>`, ou se transfere um repo
de `renanfdc`. Plano: este projeto vira commit limpo numa branch e abre **PR** para o repo combinado. Ver a
decisão final no PR/descrição.
